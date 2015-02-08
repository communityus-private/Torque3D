//-----------------------------------------------------------------------------
// Copyright (c) 2015 Andrew Mac
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "T3D/offlineLPV/offlineLPV.h"

#include "scene/sceneManager.h"
#include "scene/sceneRenderState.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/sim/debugDraw.h"
#include "util/tempAlloc.h"
#include "materials/materialDefinition.h"
#include "materials/materialManager.h"
#include "materials/materialFeatureTypes.h"
#include "materials/matInstance.h"
#include "console/consoleTypes.h"
#include "core/stream/bitStream.h"
#include "gfx/gfxDevice.h"
#include "console/console.h"
#include "console/engineAPI.h"
#include "gfx/gfxTextureHandle.h"
#include "scene/sceneContainer.h"
#include "math/mRandom.h"
#include "T3D/lightBase.h"
#include "lighting/shadowMap/lightShadowMap.h"
#include "renderInstance/renderPassManager.h"
#include "renderInstance/renderBinManager.h"
#include "gfx/gfxTextureManager.h"
#include "materials/shaderData.h"
#include "gfx/sim/gfxStateBlockData.h"
#include "gfx/util/screenspace.h"
#include "postFx/postEffectCommon.h"

#include "math/mPolyhedron.impl.h"

IMPLEMENT_CO_NETOBJECT_V1( OfflineLPV );

ConsoleDocClass( OfflineLPV,
   "@brief An invisible shape that allow objects within it to be lit with static global illumination.\n\n"

   "OfflineLPV is used to add static global illumination to an area of the scene. It's results are generated\n\n"
   "at runtime based on the settings configured in the editor. \n\n"

   "@ingroup enviroMisc"
);

GFX_ImplementTextureProfile( LPVProfile,
                              GFXTextureProfile::DiffuseMap,
                              GFXTextureProfile::PreserveSize | GFXTextureProfile::KeepBitmap,
                              GFXTextureProfile::NONE );

//-----------------------------------------------------------------------------

OfflineLPV::OfflineLPV()
   : mTransformDirty( true ),
     mSilhouetteExtractor( mPolyhedron )
{
   VECTOR_SET_ASSOCIATION( mWSPoints );
   VECTOR_SET_ASSOCIATION( mVolumeQueryList );

   mNetFlags.set( Ghostable | ScopeAlways );
   mObjScale.set( 1.f, 1.f, 1.f );
   mObjBox.set(
      Point3F( -0.5f, -0.5f, -0.5f ),
      Point3F( 0.5f, 0.5f, 0.5f )
   );

   mObjToWorld.identity();
   mWorldToObj.identity();

   mRegenVolume = false;
   mInjectLights = false;
   mPropagateLights = false;
   mShowLightGrid = false;
   mExportGrid = false;
   mShowPropagatedLightGrid = false;
   mPropagationStage = 0;

   mLPVTexture = NULL;
   mLightInfoTarget = NULL;
   mPrepassTarget = NULL;
   mShader = NULL;
   mShaderConsts = NULL;
   mRenderTarget = NULL;
   mPropagatedLightGrid = &mPropagatedLightGridA;

   // Setup Empty Geometry Grid.
   for ( U32 x = 0; x < 10; x++ )
   {
      for ( U32 y = 0; y < 10; y++ )
      {
         for ( U32 z = 0; z < 10; z++ )
            mGeometryGrid[x][y][z] = false;
      }
   }

   resetWorldBox();

   RenderPassManager::getRenderBinSignal().notify( this, &OfflineLPV::_handleBinEvent );
}

OfflineLPV::~OfflineLPV()
{
   mLPVTexture = NULL;
   mLightInfoTarget = NULL;
   mPrepassTarget = NULL;
   mShader = NULL;
   mShaderConsts = NULL;
   mRenderTarget = NULL;
}

void OfflineLPV::initPersistFields()
{
   addProtectedField( "regenVolume", TypeBool, Offset( mRegenVolume, OfflineLPV ),
         &_setRegenVolume, &defaultProtectedGetFn, "HACK: flip this to regen volume." );

   addProtectedField( "injectLights", TypeBool, Offset( mInjectLights, OfflineLPV ),
         &_setInjectLights, &defaultProtectedGetFn, "HACK: flip this to regen volume." );

   addProtectedField( "propagateLights", TypeBool, Offset( mPropagateLights, OfflineLPV ),
         &_setPropagateLights, &defaultProtectedGetFn, "HACK: flip this to regen volume." );

   addProtectedField( "exportGrid", TypeBool, Offset( mExportGrid, OfflineLPV ),
         &_setExportGrid, &defaultProtectedGetFn, "HACK: flip this to regen volume." );

   addField("showLightGrid", TypeBool, Offset(mShowLightGrid, OfflineLPV), "");
   addField("showPropagatedLightGrid", TypeBool, Offset(mShowPropagatedLightGrid, OfflineLPV), "");

   Parent::initPersistFields();
}

//-----------------------------------------------------------------------------

void OfflineLPV::consoleInit()
{
   // Disable rendering of occlusion volumes by default.
   getStaticClassRep()->mIsRenderEnabled = false;
}

//-----------------------------------------------------------------------------

bool OfflineLPV::onAdd()
{
   if( !Parent::onAdd() )
      return false;

   if ( isClientObject() && mLPVTexture.isNull() )  
   {
      // Initialize the volume texture with all black to begin with.
      U32 pos = 0;
      for(U32 x = 0; x < LPV_GRID_RESOLUTION; x++)
      {
         for(U32 y = 0; y < LPV_GRID_RESOLUTION; y++)
         {
            for(U32 z = 0; z < LPV_GRID_RESOLUTION; z++)
            {
               mLPVRawData[pos]     = 0;   // Blue
               mLPVRawData[pos + 1] = 0;   // Green
               mLPVRawData[pos + 2] = 0;   // Red
               mLPVRawData[pos + 3] = 0;   // Alpha
               pos += 4;
            }
         }
      }

      // TODO: _initShader() could fail.
      mLPVTexture.set(LPV_GRID_RESOLUTION, LPV_GRID_RESOLUTION, LPV_GRID_RESOLUTION, &mLPVRawData[0], GFXFormat::GFXFormatR8G8B8A8, &LPVProfile, "Light Propagation Grid");
      _initShader();
   }

   mRegenVolume = false;
   mInjectLights = false;
   mPropagateLights = false;
   mShowLightGrid = false;
   mExportGrid = false;
   mShowPropagatedLightGrid = false;

   // Set up the silhouette extractor.
   mSilhouetteExtractor = SilhouetteExtractorType( mPolyhedron );

   return true;
}

void OfflineLPV::onRemove()
{
   if ( isClientObject() )  
   {

   }
   Parent::onRemove();
}

//-----------------------------------------------------------------------------

// This renders the visualization of the volume in the editor.
void OfflineLPV::_renderObject( ObjectRenderInst* ri, SceneRenderState* state, BaseMatInstance* overrideMat )
{
   Parent::_renderObject( ri, state, overrideMat );

   // Render GRID!
   GFXDrawUtil *drawer = GFX->getDrawUtil();

   GFXStateBlockDesc desc;
   desc.setZReadWrite( true, true );
   desc.setBlend( false );
   desc.setFillModeSolid();
 
   Box3F worldBox = getWorldBox();
   Point3F bottom_corner = worldBox.minExtents;
   Point3F top_corner = worldBox.maxExtents;
   Point3F difference = (top_corner - bottom_corner) / LPV_GRID_RESOLUTION;

   // Geometry Grid Visualization.
   for ( U32 x = 0; x < LPV_GRID_RESOLUTION; x++ )
   {
      for ( U32 y = 0; y < LPV_GRID_RESOLUTION; y++ )
      {
         for ( U32 z = 0; z < LPV_GRID_RESOLUTION; z++ )
         {
            if ( mShowLightGrid )
            {
               if ( mShowPropagatedLightGrid )
               {
                  if ( mPropagatedLightGrid->data[x][y][z].alpha <= 0.0f ) continue;

                  Box3F new_box;
                  new_box.set(bottom_corner + Point3F(difference.x * x, difference.y * y, difference.z * z), 
                     bottom_corner + Point3F(difference.x * (x + 1), difference.y * (y + 1), difference.z * (z + 1)));
                  drawer->drawCube( desc, new_box, mPropagatedLightGrid->data[x][y][z] );
               } else {
                  if ( mLightGrid.data[x][y][z].alpha <= 0.0f ) continue;

                  Box3F new_box;
                  new_box.set(bottom_corner + Point3F(difference.x * x, difference.y * y, difference.z * z), 
                     bottom_corner + Point3F(difference.x * (x + 1), difference.y * (y + 1), difference.z * (z + 1)));
                  drawer->drawCube( desc, new_box, mLightGrid.data[x][y][z] );
               }
            } else {
               if ( !mGeometryGrid[x][y][z] ) continue;

               Box3F new_box;
               new_box.set(bottom_corner + Point3F(difference.x * x, difference.y * y, difference.z * z), 
                  bottom_corner + Point3F(difference.x * (x + 1), difference.y * (y + 1), difference.z * (z + 1)));
               drawer->drawCube( desc, new_box, ColorI( x * 7.9, y * 7.9, z * 7.9 ) );
            }
         }
      }
   }
}

//-----------------------------------------------------------------------------

void OfflineLPV::setTransform( const MatrixF& mat )
{
   Parent::setTransform( mat );
   mTransformDirty = true;
}

//-----------------------------------------------------------------------------

void OfflineLPV::buildSilhouette( const SceneCameraState& cameraState, Vector< Point3F >& outPoints )
{
   // Extract the silhouette of the polyhedron.  This works differently
   // depending on whether we project orthogonally or in perspective.

   TempAlloc< U32 > indices( mPolyhedron.getNumPoints() );
   U32 numPoints;

   if( cameraState.getFrustum().isOrtho() )
   {
      // Transform the view direction into object space.

      Point3F osViewDir;
      getWorldTransform().mulV( cameraState.getViewDirection(), &osViewDir );

      // And extract the silhouette.

      SilhouetteExtractorOrtho< PolyhedronType > extractor( mPolyhedron );
      numPoints = extractor.extractSilhouette( osViewDir, indices, indices.size );
   }
   else
   {
      // Create a transform to go from view space to object space.

      MatrixF camView( true );
      camView.scale( Point3F( 1.0f / getScale().x, 1.0f / getScale().y, 1.0f / getScale().z ) );
      camView.mul( getRenderWorldTransform() );
      camView.mul( cameraState.getViewWorldMatrix() );

      // Do a perspective-correct silhouette extraction.

      numPoints = mSilhouetteExtractor.extractSilhouette(
         camView,
         indices, indices.size );
   }

   // If we haven't yet, transform the polyhedron's points
   // to world space.

   if( mTransformDirty )
   {
      const U32 numPoints = mPolyhedron.getNumPoints();
      const PolyhedronType::PointType* points = getPolyhedron().getPoints();

      mWSPoints.setSize( numPoints );
      for( U32 i = 0; i < numPoints; ++ i )
      {
         Point3F p = points[ i ];
         p.convolve( getScale() );
         getTransform().mulP( p, &mWSPoints[ i ] );
      }

      mTransformDirty = false;
   }

   // Now store the points.

   outPoints.setSize( numPoints );
   for( U32 i = 0; i < numPoints; ++ i )
      outPoints[ i ] = mWSPoints[ indices[ i ] ];
}

//-----------------------------------------------------------------------------

U32 OfflineLPV::packUpdate( NetConnection *connection, U32 mask, BitStream *stream )
{
   U32 retMask = Parent::packUpdate( connection, mask, stream );

   // TODO: Should probably use a mask here rather than sending every update.

   stream->writeFlag(mShowLightGrid);
   stream->writeFlag(mShowPropagatedLightGrid);

   stream->writeFlag(mRegenVolume);
   if ( mRegenVolume )
      mRegenVolume = false;

   stream->writeFlag(mInjectLights);
   if ( mInjectLights )
      mInjectLights = false;

   stream->writeFlag(mPropagateLights);
   if ( mPropagateLights )
      mPropagateLights = false;

   stream->writeFlag(mExportGrid);
   if ( mExportGrid )
      mExportGrid = false;

   return retMask;  
}

void OfflineLPV::unpackUpdate( NetConnection *connection, BitStream *stream )
{
   Parent::unpackUpdate( connection, stream );

   mShowLightGrid = stream->readFlag();
   mShowPropagatedLightGrid = stream->readFlag();

   // Regen Volume Triggered?
   if ( stream->readFlag() )
   {
      regenVolume();
   }

   // Inject Lights Triggered?
   if ( stream->readFlag() )
   {
      injectLights();
   }

   // Propagate Lights Triggered?
   if ( stream->readFlag() )
   {
      switch(mPropagationStage)
      {
         case 0:
            mPropagatedLightGrid = &mPropagatedLightGridA;
            propagateLights(&mLightGrid, mPropagatedLightGrid, true);
            mPropagationStage = 1;
            break;
         case 1:
            mPropagatedLightGrid = &mPropagatedLightGridB;
            propagateLights(&mPropagatedLightGridA, &mPropagatedLightGridB);
            mPropagationStage = 2;
            break;
         case 2:
            mPropagatedLightGrid = &mPropagatedLightGridA;
            propagateLights(&mPropagatedLightGridB, &mPropagatedLightGridA);
            mPropagationStage = 1;
            break;
      }
   }

   // Propagate Lights Triggered?
   if ( stream->readFlag() )
   {
      exportGrid();
   }
}

//-----------------------------------------------------------------------------

void OfflineLPV::inspectPostApply()
{
   Parent::inspectPostApply();
   setMaskBits(U32(-1));
}

//--- Scene Voxelization ---

bool OfflineLPV::_setRegenVolume( void *object, const char *index, const char *data )
{
   OfflineLPV* volume = reinterpret_cast< OfflineLPV* >( object );
   volume->mRegenVolume = true;
   return false;
}

void OfflineLPV::regenVolume()
{
   Con::printf("Regenerating Geometry Volume!");

   Box3F worldBox = getWorldBox();
   Point3F bottom_corner = worldBox.minExtents;
   Point3F top_corner = worldBox.maxExtents;
   Point3F difference = (top_corner - bottom_corner) / LPV_GRID_RESOLUTION;

   RayInfo rayInfo;
   SceneContainer* container = getContainer();
   if ( !container ) return;

   // Geometry Grid Visualization.
   for ( U32 x = 0; x < LPV_GRID_RESOLUTION; x++ )
   {
      for ( U32 y = 0; y < LPV_GRID_RESOLUTION; y++ )
      {
         for ( U32 z = 0; z < LPV_GRID_RESOLUTION; z++ )
         {
            Point3F start = bottom_corner + Point3F(difference.x * x, difference.y * y, difference.z * z);
            Point3F end = bottom_corner + Point3F(difference.x * (x + 1), difference.y * (y + 1), difference.z * (z + 1));

            bool hit = container->castRay(start, end, STATIC_COLLISION_TYPEMASK, &rayInfo);
            if ( rayInfo.material )
            {
               const char* name = rayInfo.material->getMaterial()->getName();
               //Con::printf("Material Found In Voxel: %s", name);
            }
            mGeometryGrid[x][y][z] = hit;
            mLightGrid.data[x][y][z] = ColorF::ZERO;
            mPropagationStage = 0;
            mPropagatedLightGridA.data[x][y][z] = ColorF::ZERO;
            mPropagatedLightGridB.data[x][y][z] = ColorF::ZERO;
         }
      }
   }
}

//--- Light Injection ---

bool OfflineLPV::_setInjectLights( void *object, const char *index, const char *data )
{
   OfflineLPV* volume = reinterpret_cast< OfflineLPV* >( object );
   volume->mInjectLights = true;
   return false;
}

void OfflineLPV::injectLights()
{
   Con::printf("Injecting Lights!");

   Box3F worldBox = getWorldBox();
   Point3F bottom_corner = worldBox.minExtents;
   Point3F top_corner = worldBox.maxExtents;
   Point3F half_difference = ((top_corner - bottom_corner) / LPV_GRID_RESOLUTION);

   RayInfo rayInfo;
   SceneContainer* container = getContainer();

   indirectLightSources.clear();

   // Geometry Grid Visualization.
   for ( U32 x = 0; x < LPV_GRID_RESOLUTION; x++ )
   {
      for ( U32 y = 0; y < LPV_GRID_RESOLUTION; y++ )
      {
         for ( U32 z = 0; z < LPV_GRID_RESOLUTION; z++ )
         {
            if ( !mGeometryGrid[x][y][z] ) continue;

            Point3F point = bottom_corner + Point3F(half_difference.x * (x + 1), half_difference.y * (y + 1), half_difference.z * (z + 1));
            mLightGrid.data[x][y][z] = calcLightColor(point);

            IndirectLightSource l;
            l.color = mLightGrid.data[x][y][z];
            l.position = point;
            indirectLightSources.push_back(l);
            mPropagationStage = 0;
         }
      }
   }
}

ColorF OfflineLPV::calcLightColor(Point3F position)
{
   PROFILE_SCOPE( OfflineLPV_calcLightColor );

   ColorF result = ColorF::ZERO;

   if ( !LIGHTMGR )
      return result;

   RayInfo rayInfo;
   SceneContainer* container = getContainer();
   if ( !container ) return result;

   Vector<SceneObject*> lightObjects;
   const U32 lightMask = LightObjectType;
   container->findObjectList( lightMask, &lightObjects );

   Vector<SceneObject*>::iterator iter = lightObjects.begin();
   for ( ; iter != lightObjects.end(); iter++ )
   {
      // Get the light.
      SceneObject *lightObject = (*iter);

      bool hit = container->castRay(position, lightObject->getPosition(), STATIC_COLLISION_TYPEMASK, &rayInfo);
      if ( hit )
      {
         // OBSTRUCTED!
         continue;
      }
      else
      {
         LightBase* lb = dynamic_cast<LightBase*>(lightObject);
         if ( !lb ) continue;

         LightInfo* info = lb->getLight();
         F32 atten = getAttenuation(info, position);
         if ( atten <= 0 ) continue;

         result += info->getColor() * atten;
         //Con::printf("Attenuation: %f Result Total: %f %f %f %f", atten, result.red, result.green, result.blue, result.alpha);
      }
   }

   return result;
}

F32 OfflineLPV::getAttenuation(LightInfo* lightInfo, Point3F position)
{
   // Get the attenuation falloff ratio and normalize it.
   Point3F attenRatio = lightInfo->getExtended<ShadowMapParams>()->attenuationRatio;
   F32 total = attenRatio.x + attenRatio.y + attenRatio.z;
   if ( total > 0.0f )
      attenRatio /= total;

   const F32 radius = lightInfo->getRange().x;
   const F32 dist = Point3F(lightInfo->getPosition() - position).len();

   Point2F attenParams( ( 1.0f / radius ) * attenRatio.y,
                        ( 1.0f / ( radius * radius ) ) * attenRatio.z );

   return 1.0 - mDot( attenParams, Point2F( dist, dist * dist ) );
}

//--- Light Propagation ---

bool OfflineLPV::_setPropagateLights( void *object, const char *index, const char *data )
{
   OfflineLPV* volume = reinterpret_cast< OfflineLPV* >( object );
   volume->mPropagateLights = true;
   return false;
}

void OfflineLPV::propagateLights(ColorVoxelGrid* source, ColorVoxelGrid* dest, bool sampleFromGeometry)
{
   Con::printf("Propagating Lights!");

   Box3F worldBox = getWorldBox();
   Point3F bottom_corner = worldBox.minExtents;
   Point3F top_corner = worldBox.maxExtents;
   Point3F half_difference = ((top_corner - bottom_corner) / LPV_GRID_RESOLUTION) / 2;

   Point3F difference = ((top_corner - bottom_corner) / LPV_GRID_RESOLUTION);
   F32 distance = 1 + difference.len(); // if not offset by one then a distance < 1 increases light power? :(
   F32 attenuation = 1 / (distance * distance);
   //Con::printf("Distance: %f Attentuation Factor: %f", distance, attenuation);

   RayInfo rayInfo;
   SceneContainer* container = getContainer();

   // Geometry Grid Visualization.
   for ( U32 x = 0; x < LPV_GRID_RESOLUTION; x++ )
   {
      for ( U32 y = 0; y < LPV_GRID_RESOLUTION; y++ )
      {
         for ( U32 z = 0; z < LPV_GRID_RESOLUTION; z++ )
         {
            /* Color based on indirect light sources (simple tracing)
            if ( !mGeometryGrid[x][y][z] ) continue;
            if ( mLightGrid[x][y][z].alpha > 0 ) continue;

            Point3F point = bottom_corner + Point3F(half_difference.x * (x + 1), half_difference.y * (y + 1), half_difference.z * (z + 1));
            mPropagatedLightGrid[x][y][z] = calcIndirectLightColor(point);
            ---------------- */

            /* -- OR -- */

            /* Color Bleeding */
            ColorF blendedColor(0, 0, 0, 0);
            for ( S32 outerX = -1; outerX < 2; outerX++ )
            {
               for ( S32 outerY = -1; outerY < 2; outerY++ )
               {
                  for ( S32 outerZ = -1; outerZ < 2; outerZ++ )
                  {
                     if ( x + outerX < 0 || y + outerY < 0 || z + outerZ < 0 ) continue;
                     if ( x + outerX >= LPV_GRID_RESOLUTION || y + outerY >= LPV_GRID_RESOLUTION || z + outerZ >= LPV_GRID_RESOLUTION ) continue;

                     // Attempt at being geometry aware. The idea was to not sample from voxels with geometry in them after the first pass.
                     // First pass would let light bleed into the "air" and then the following passes would sample from "air light".
                     if ( !sampleFromGeometry && mGeometryGrid[x + outerX][y + outerY][z + outerZ] ) continue;

                     blendedColor += (source->data[x + outerX][y + outerY][z + outerZ]);// * attenuation;
                  }
               }
            }

            dest->data[x][y][z] = blendedColor / 27;
            if ( dest->data[x][y][z].alpha > 0 )
               dest->data[x][y][z].alpha = 1.0f;
            /* ---------------- */
         }
      }
   }
}

ColorF OfflineLPV::calcIndirectLightColor(Point3F position)
{
   PROFILE_SCOPE( OfflineLPV_calcIndirectLightColor );

   ColorF result = ColorF::ZERO;

   if ( !LIGHTMGR )
      return result;

   RayInfo rayInfo;
   SceneContainer* container = getContainer();
   if ( !container ) return result;

   for (U32 i = 0; i < indirectLightSources.size(); ++i )
   {
      IndirectLightSource* light = &indirectLightSources[i];

      bool hit = container->castRay(position, light->position, STATIC_COLLISION_TYPEMASK, &rayInfo);
      if ( hit )
      {
         // OBSTRUCTED!
         continue;
      }
      else
      {
         const F32 dist = Point3F(light->position - position).len();
         result += light->color * (1 / (dist * dist));
      }
   }

   return result;
}

//--- Export from CPU to GPU for rendering ---

bool OfflineLPV::_setExportGrid( void *object, const char *index, const char *data )
{
   OfflineLPV* volume = reinterpret_cast< OfflineLPV* >( object );
   volume->mExportGrid = true;
   return false;
}

// Exports mPropagatedLightGrid to the actual volume texture on the graphics card.
// Note: for some reason the volume texture insists on being BGRA.
void OfflineLPV::exportGrid()
{
   GFXLockedRect* locked_rect = mLPVTexture->lock();
   if ( locked_rect )
   {
      U32 pos = 0;
      for(U32 x = 0; x < LPV_GRID_RESOLUTION; x++)
      {
         for(U32 y = 0; y < LPV_GRID_RESOLUTION; y++)
         {
            for(U32 z = 0; z < LPV_GRID_RESOLUTION; z++)
            {
               ColorI cell_color = mPropagatedLightGrid->data[x][y][z];
               mLPVRawData[pos]     = cell_color.blue;    // Blue
               mLPVRawData[pos + 1] = cell_color.green;   // Green
               mLPVRawData[pos + 2] = cell_color.red;     // Red
               mLPVRawData[pos + 3] = 255;                // Alpha
               pos += 4;
            }
         }
      }
      dMemcpy(locked_rect->bits, mLPVRawData, LPV_GRID_RESOLUTION * LPV_GRID_RESOLUTION * LPV_GRID_RESOLUTION * 4 * sizeof(U8));
      mLPVTexture->unlock();
   }
}

//--- Final Volume Rendering Into LightBuffer ---

bool OfflineLPV::_initShader()
{
   mShader = NULL;
   mShaderConsts = NULL;

   ShaderData *shaderData;
   if ( !Sim::findObject( "OfflineLPVShaderData", shaderData ) )
   {
      Con::warnf( "OfflineLPV::_initShader - failed to locate shader OfflineLPVShaderData!" );
      return false;
   }
   
   // Need depth from pre-pass, so get the macros
   Vector<GFXShaderMacro> macros;

   if ( !mPrepassTarget )
      mPrepassTarget = NamedTexTarget::find( "prepass" );

   if ( mPrepassTarget )
      mPrepassTarget->getShaderMacros( &macros );

   mShader = shaderData->getShader( macros );

   if ( !mShader )
      return false;

   mShaderConsts = mShader->allocConstBuffer();
   mEyePosWorldSC = mShader->getShaderConstHandle( "$eyePosWorld" );
   mRTParamsSC = mShader->getShaderConstHandle( "$rtParams0" );

   mVolumeStartSC = mShader->getShaderConstHandle( "$volumeStart" );
   mVolumeSizeSC = mShader->getShaderConstHandle( "$volumeSize" );

   return true;
}

void OfflineLPV::_handleBinEvent(   RenderBinManager *bin,                           
                  const SceneRenderState* sceneState,
                  bool isBinStart )
{

   // We require a bin name to process effects... without
   // it we can skip the bin entirely.
   String binName( bin->getName() );
   if ( binName.isEmpty() )
      return;

   if ( !isBinStart && binName.equal("AL_LightBinMgr") )
   {
      _renderLPV(sceneState);
   }
}

void OfflineLPV::_renderLPV(const SceneRenderState* state)
{
   if ( !mLPVTexture || !isClientObject() ) 
      return;

   // -- Setup Render Target --
   if ( !mRenderTarget )
      mRenderTarget = GFX->allocRenderToTextureTarget();
         
   if ( !mRenderTarget ) return;

   if ( !mLightInfoTarget )
      mLightInfoTarget = NamedTexTarget::find( "lightinfo" );

   GFXTextureObject *texObject = mLightInfoTarget->getTexture();
   if ( !texObject ) return;

   mRenderTarget->attachTexture( GFXTextureTarget::Color0, texObject );

   // We also need to sample from the depth buffer.
   if ( !mPrepassTarget )
      mPrepassTarget = NamedTexTarget::find( "prepass" );

   GFXTextureObject *prepassTexObject = mPrepassTarget->getTexture();
   if ( !prepassTexObject ) return;

   // -- Setup screenspace quad to render (postfx) --
   Frustum frustum = state->getCameraFrustum();
   GFXVertexBufferHandle<PFXVertex> vb;
   _updateScreenGeometry( frustum, &vb );

   // -- State Block --
   GFXStateBlockDesc desc;
   desc.setZReadWrite( false, false );
   desc.setBlend( true, GFXBlendOne, GFXBlendOne );
   desc.setFillModeSolid();

   // Point sampling is useful for debugging to see the exact color values in each cell.
   //desc.samplers[0] = GFXSamplerStateDesc::getClampPoint();
   //desc.samplersDefined = true;

   // Camera position, used to calculate World Space position from depth buffer.
   const Point3F &camPos = state->getCameraPosition();
   mShaderConsts->setSafe( mEyePosWorldSC, camPos );

   // Volume position, used to calculate UV sampling.
   Box3F worldBox = getWorldBox();
   Point3F bottom_corner = worldBox.minExtents;
   Point3F top_corner = worldBox.maxExtents;
   Point3F volume_size = (top_corner - bottom_corner);
   mShaderConsts->setSafe(mVolumeStartSC, bottom_corner);
   mShaderConsts->setSafe(mVolumeSizeSC, volume_size);

   // Render Target Parameters.
   const Point3I &targetSz = texObject->getSize();
   const RectI &targetVp = mLightInfoTarget->getViewport();
   Point4F rtParams;
   ScreenSpace::RenderTargetParameters(targetSz, targetVp, rtParams);
   mShaderConsts->setSafe(mRTParamsSC, rtParams);

   GFX->pushActiveRenderTarget();
   GFX->setActiveRenderTarget( mRenderTarget );
   GFX->setVertexBuffer( vb );
   GFX->setStateBlockByDesc( desc );
   GFX->setShader(mShader);
   GFX->setShaderConstBuffer(mShaderConsts);

   // Setup Textures
   GFX->setTexture(0, mLPVTexture);
   GFX->setTexture(1, prepassTexObject);

   // Draw the screenspace quad.
   GFX->drawPrimitive( GFXTriangleFan, 0, 2 );

   // Clean Up
   mRenderTarget->resolve();
   GFX->popActiveRenderTarget();
}

void OfflineLPV::_updateScreenGeometry(   const Frustum &frustum,
                                          GFXVertexBufferHandle<PFXVertex> *outVB )
{
   outVB->set( GFX, 4, GFXBufferTypeVolatile );

   const Point3F *frustumPoints = frustum.getPoints();
   const Point3F& cameraPos = frustum.getPosition();

   // Perform a camera offset.  We need to manually perform this offset on the postFx's
   // polygon, which is at the far plane.
   const Point2F& projOffset = frustum.getProjectionOffset();
   Point3F cameraOffsetPos = cameraPos;
   if(!projOffset.isZero())
   {
      // First we need to calculate the offset at the near plane.  The projOffset
      // given above can be thought of a percent as it ranges from 0..1 (or 0..-1).
      F32 nearOffset = frustum.getNearRight() * projOffset.x;

      // Now given the near plane distance from the camera we can solve the right
      // triangle and calcuate the SIN theta for the offset at the near plane.
      // SIN theta = x/y
      F32 sinTheta = nearOffset / frustum.getNearDist();

      // Finally, we can calcuate the offset at the far plane, which is where our sun (or vector)
      // light's polygon is drawn.
      F32 farOffset = frustum.getFarDist() * sinTheta;

      // We can now apply this far plane offset to the far plane itself, which then compensates
      // for the project offset.
      MatrixF camTrans = frustum.getTransform();
      VectorF offset = camTrans.getRightVector();
      offset *= farOffset;
      cameraOffsetPos += offset;
   }

   PFXVertex *vert = outVB->lock();

   vert->point.set( -1.0, -1.0, 0.0 );
   vert->texCoord.set( 0.0f, 1.0f );
   vert->wsEyeRay = frustumPoints[Frustum::FarBottomLeft] - cameraOffsetPos;
   vert++;

   vert->point.set( -1.0, 1.0, 0.0 );
   vert->texCoord.set( 0.0f, 0.0f );
   vert->wsEyeRay = frustumPoints[Frustum::FarTopLeft] - cameraOffsetPos;
   vert++;

   vert->point.set( 1.0, 1.0, 0.0 );
   vert->texCoord.set( 1.0f, 0.0f );
   vert->wsEyeRay = frustumPoints[Frustum::FarTopRight] - cameraOffsetPos;
   vert++;

   vert->point.set( 1.0, -1.0, 0.0 );
   vert->texCoord.set( 1.0f, 1.0f );
   vert->wsEyeRay = frustumPoints[Frustum::FarBottomRight] - cameraOffsetPos;
   vert++;

   outVB->unlock();
}