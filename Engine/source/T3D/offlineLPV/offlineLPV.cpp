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
#include "materials/processedMaterial.h"
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
#include "gfx/gfxTextureHandle.h"
#include "postFx/postEffectCommon.h"
#include "core/stream/fileStream.h"
#include "core/fileObject.h"
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

   mVoxelSize           = 1.0f;
   mRegenVolume         = false;
   mShowVoxels          = false;
   mInjectLights        = false;
   mShowDirectLight     = false;
   mPropagationStage    = 0;
   mPropagateLights     = false;
   mShowPropagated      = false;
   mExportPropagated    = false;
   mExportDirectLight   = false;
   mRenderReflection    = false;
   mFileName            = "";
   mSaveResults         = false;
   mLoadResults         = false;

   mGeometryGrid           = NULL;
   mLightGrid              = NULL;
   mPropagatedLightGrid    = NULL;
   mPropagatedLightGridA   = NULL;
   mPropagatedLightGridB   = NULL;
   mPropagatedTexture      = NULL;
   mDirectLightTexture     = NULL;
   mLightInfoTarget        = NULL;
   mPrepassTarget          = NULL;
   mPropagatedShader       = NULL;
   mPropagatedShaderConsts = NULL;
   mReflectShader          = NULL;
   mReflectShaderConsts    = NULL;
   mRenderTarget           = NULL;

   resetWorldBox();

   RenderPassManager::getRenderBinSignal().notify( this, &OfflineLPV::_handleBinEvent );
}

OfflineLPV::~OfflineLPV()
{
   SAFE_DELETE(mGeometryGrid);
   SAFE_DELETE(mLightGrid);
   SAFE_DELETE(mPropagatedLightGridA);
   SAFE_DELETE(mPropagatedLightGridB);

   mPropagatedTexture      = NULL;
   mDirectLightTexture     = NULL;
   mLightInfoTarget        = NULL;
   mPrepassTarget          = NULL;
   mPropagatedShader       = NULL;
   mPropagatedShaderConsts = NULL;
   mReflectShader          = NULL;
   mReflectShaderConsts    = NULL;
   mRenderTarget           = NULL;
}

void OfflineLPV::initPersistFields()
{
   //
   addGroup("OfflineLPV - General");

      addField("voxelSize", TypeF32, Offset(mVoxelSize, OfflineLPV), "");

      addProtectedField( "regenVolume", TypeBool, Offset( mRegenVolume, OfflineLPV ),
         &_setRegenVolume, &defaultProtectedGetFn, "Regenerate Voxel Grid", AbstractClassRep::FieldFlags::FIELD_ButtonInInspectors);

      addField("showVoxels", TypeBool, Offset(mShowVoxels, OfflineLPV), "");

      addProtectedField( "injectLights", TypeBool, Offset( mInjectLights, OfflineLPV ),
            &_setInjectLights, &defaultProtectedGetFn, "Inject scene lights into grid.", AbstractClassRep::FieldFlags::FIELD_ButtonInInspectors );

      addField("showDirectLight", TypeBool, Offset(mShowDirectLight, OfflineLPV), "");

   endGroup("OfflineLPV - General");

   //
   addGroup("OfflineLPV - Propagation");

      addProtectedField( "propagateLights", TypeBool, Offset( mPropagateLights, OfflineLPV ),
            &_setPropagateLights, &defaultProtectedGetFn, "Perform light propagation.", AbstractClassRep::FieldFlags::FIELD_ButtonInInspectors );

      addField("showPropagated", TypeBool, Offset(mShowPropagated, OfflineLPV), "Render propagated light.");

      addProtectedField( "exportPropagated", TypeBool, Offset( mExportPropagated, OfflineLPV ),
            &_setExportPropagated, &defaultProtectedGetFn, "Export propagated light grid to display.", AbstractClassRep::FieldFlags::FIELD_ButtonInInspectors );

   endGroup("OfflineLPV - Propagation");

   //
   addGroup("OfflineLPV - Reflection");

      addProtectedField( "exportDirectLight", TypeBool, Offset( mExportDirectLight, OfflineLPV ),
            &_setExportDirectLight, &defaultProtectedGetFn, "Export direct light grid for reflections.", AbstractClassRep::FieldFlags::FIELD_ButtonInInspectors );

      addField("renderReflection", TypeBool, Offset(mRenderReflection, OfflineLPV), "Render reflections.");

   endGroup("OfflineLPV - Reflection");

   //
   addGroup("OfflineLPV - Save/Load");

      addField( "fileName", TypeStringFilename, Offset( mFileName, OfflineLPV ), "Path of file to save and load results." );

      addProtectedField( "saveResults", TypeBool, Offset( mSaveResults, OfflineLPV ),
            &_setSaveResults, &defaultProtectedGetFn, "Save the results of the light propagation.", AbstractClassRep::FieldFlags::FIELD_ButtonInInspectors );

      addProtectedField( "loadResults", TypeBool, Offset( mLoadResults, OfflineLPV ),
            &_setLoadResults, &defaultProtectedGetFn, "Load the results of the light propagation.", AbstractClassRep::FieldFlags::FIELD_ButtonInInspectors );

   endGroup("OfflineLPV - Save/Load");

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

   if ( isClientObject() )  
      load();

   // None of these are actual settings to be saved.
   mRegenVolume         = false;
   mInjectLights        = false;
   mPropagateLights     = false;
   mExportPropagated    = false;
   mExportDirectLight   = false;
   mSaveResults         = false;
   mLoadResults         = false;
   mShowVoxels          = false;
   mShowDirectLight     = false;
   mShowPropagated      = false;

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

// Returns a Point3I with the voxel count in each direction.
Point3I OfflineLPV::getVoxelCount()
{
   Point3I voxelCount;
   Box3F worldBox = getWorldBox();

   voxelCount.x = worldBox.len_x() / mVoxelSize;
   voxelCount.y = worldBox.len_y() / mVoxelSize;
   voxelCount.z = worldBox.len_z() / mVoxelSize;

   return voxelCount;
}

// Returns a Point3F with the world space dimensions of a voxel.
Point3F OfflineLPV::getWorldSpaceVoxelSize()
{
   Box3F worldBox = getWorldBox();
   Point3F bottom_corner = worldBox.minExtents;
   Point3F top_corner = worldBox.maxExtents;
   Point3I voxelCount = getVoxelCount();
   Point3F difference = (top_corner - bottom_corner);
   difference.x = difference.x / voxelCount.x;
   difference.y = difference.y / voxelCount.y;
   difference.z = difference.z / voxelCount.z;
   return difference;
}

// Renders the visualization of the volume in the editor.
void OfflineLPV::_renderObject( ObjectRenderInst* ri, SceneRenderState* state, BaseMatInstance* overrideMat )
{
   Parent::_renderObject( ri, state, overrideMat );

   GFXDrawUtil *drawer = GFX->getDrawUtil();

   GFXStateBlockDesc desc;
   desc.setZReadWrite( true, true );
   desc.setBlend( false );
   desc.setFillModeSolid();
   
   Box3F worldBox = getWorldBox();
   Point3F bottom_corner = worldBox.minExtents;
   Point3F top_corner = worldBox.maxExtents;
   Point3F wsVoxelSize = getWorldSpaceVoxelSize();
   Point3I voxelCount = getVoxelCount();

   U32 pos = 0;
   for ( U32 z = 0; z < voxelCount.z; z++ )
   {
      for ( U32 y = 0; y < voxelCount.y; y++ )
      {
         for ( U32 x = 0; x < voxelCount.x; x++ )
         {
            // Propagated Light Grid Visualization.
            if ( mShowPropagated && mPropagatedLightGrid )
            {
               if ( mPropagatedLightGrid[pos].alpha > 0.0f )
               {
                  Box3F new_box;
                  new_box.set(bottom_corner + Point3F(wsVoxelSize.x * x, wsVoxelSize.y * y, wsVoxelSize.z * z), 
                     bottom_corner + Point3F(wsVoxelSize.x * (x + 1), wsVoxelSize.y * (y + 1), wsVoxelSize.z * (z + 1)));
                  drawer->drawCube( desc, new_box, mPropagatedLightGrid[pos] );
               }

               pos++;
               continue;
            }

            // Direct Light Grid Visualization.
            if ( mShowDirectLight && mLightGrid )
            {
               if ( mLightGrid[pos].alpha > 0.0f )
               {
                  Box3F new_box;
                  new_box.set(bottom_corner + Point3F(wsVoxelSize.x * x, wsVoxelSize.y * y, wsVoxelSize.z * z), 
                     bottom_corner + Point3F(wsVoxelSize.x * (x + 1), wsVoxelSize.y * (y + 1), wsVoxelSize.z * (z + 1)));
                  drawer->drawCube( desc, new_box, mLightGrid[pos] );
               }

               pos++;
               continue;
            }

            // Geometry Grid Visualization.
            if ( mShowVoxels && mGeometryGrid )
            {
               if ( mGeometryGrid[pos].alpha > 0.0f )
               {
                  Box3F new_box;
                  new_box.set(bottom_corner + Point3F(wsVoxelSize.x * x, wsVoxelSize.y * y, wsVoxelSize.z * z), 
                     bottom_corner + Point3F(wsVoxelSize.x * (x + 1), wsVoxelSize.y * (y + 1), wsVoxelSize.z * (z + 1)));
                  drawer->drawCube( desc, new_box, mGeometryGrid[pos] );
               }

               pos++;
               continue;
            }

            pos++;
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
   stream->write(mVoxelSize);
   stream->write(mFileName);
   stream->writeFlag(mShowVoxels);
   stream->writeFlag(mShowDirectLight);
   stream->writeFlag(mShowPropagated);
   stream->writeFlag(mRenderReflection);

   stream->writeFlag(mRegenVolume);
   if ( mRegenVolume )
      mRegenVolume = false;

   stream->writeFlag(mInjectLights);
   if ( mInjectLights )
      mInjectLights = false;

   stream->writeFlag(mPropagateLights);
   if ( mPropagateLights )
      mPropagateLights = false;

   stream->writeFlag(mExportPropagated);
   if ( mExportPropagated )
      mExportPropagated = false;

   stream->writeFlag(mExportDirectLight);
   if ( mExportDirectLight )
      mExportDirectLight = false;

   stream->writeFlag(mSaveResults);
   if ( mSaveResults )
      mSaveResults = false;

   stream->writeFlag(mLoadResults);
   if ( mLoadResults )
      mLoadResults = false;

   return retMask;  
}

void OfflineLPV::unpackUpdate( NetConnection *connection, BitStream *stream )
{
   Parent::unpackUpdate( connection, stream );

   stream->read(&mVoxelSize);
   stream->read(&mFileName);
   mShowVoxels       = stream->readFlag();
   mShowDirectLight  = stream->readFlag();
   mShowPropagated   = stream->readFlag();
   mRenderReflection = stream->readFlag();

   // Regen Volume Triggered?
   if ( stream->readFlag() )
      regenVolume();

   // Inject Lights Triggered?
   if ( stream->readFlag() )
      injectLights();

   // Propagate Lights Triggered?
   if ( stream->readFlag() )
   {
      Point3I voxelCount = getVoxelCount();

      if (!mPropagatedLightGridA)
         mPropagatedLightGridA = new ColorF[voxelCount.x * voxelCount.y * voxelCount.z];

      if (!mPropagatedLightGridB)
         mPropagatedLightGridB = new ColorF[voxelCount.x * voxelCount.y * voxelCount.z];

      switch(mPropagationStage)
      {
         case 0:
            mPropagatedLightGrid = mPropagatedLightGridA;
            propagateLights(mLightGrid, mPropagatedLightGrid, true);
            mPropagationStage = 1;
            break;

         case 1:
            mPropagatedLightGrid = mPropagatedLightGridB;
            propagateLights(mPropagatedLightGridA, mPropagatedLightGridB);
            mPropagationStage = 2;
            break;

         case 2:
            mPropagatedLightGrid = mPropagatedLightGridA;
            propagateLights(mPropagatedLightGridB, mPropagatedLightGridA);
            mPropagationStage = 1;
            break;
      }
   }

   // Export Propagated Light?
   if ( stream->readFlag() )
      exportPropagatedLight();

   // Export Direct Light?
   if ( stream->readFlag() )
      exportDirectLight();

   // Save Results?
   if ( stream->readFlag() )
      save();

   // Load Results?
   if ( stream->readFlag() )
      load();
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
   Box3F worldBox          = getWorldBox();
   Point3F bottom_corner   = worldBox.minExtents;
   Point3F top_corner      = worldBox.maxExtents;
   Point3F wsVoxelSize     = getWorldSpaceVoxelSize();
   Point3I voxelCount      = getVoxelCount();

   RayInfo rayInfo;
   rayInfo.generateTexCoord = true;
   SceneContainer* container = getContainer();
   if ( !container ) return;

   // Allocate our grids.
   SAFE_DELETE(mGeometryGrid);
   SAFE_DELETE(mLightGrid);
   SAFE_DELETE(mPropagatedLightGridA);
   SAFE_DELETE(mPropagatedLightGridB);
   mGeometryGrid           = new ColorF[voxelCount.x * voxelCount.y * voxelCount.z];
   mLightGrid              = new ColorF[voxelCount.x * voxelCount.y * voxelCount.z];
   mPropagatedLightGridA   = new ColorF[voxelCount.x * voxelCount.y * voxelCount.z];
   mPropagatedLightGridB   = new ColorF[voxelCount.x * voxelCount.y * voxelCount.z];

   // Initialize the volume textures (if they aren't already.)
   _initVolumeTextures(voxelCount);

   // Geometry Grid Visualization.
   U32 pos = 0;
   for ( U32 z = 0; z < voxelCount.z; z++ )
   {
      for ( U32 y = 0; y < voxelCount.y; y++ )
      {
         for ( U32 x = 0; x < voxelCount.x; x++ )
         {
            Point3F start = bottom_corner + Point3F(wsVoxelSize.x * x, wsVoxelSize.y * y, wsVoxelSize.z * z);
            Point3F end = bottom_corner + Point3F(wsVoxelSize.x * (x + 1), wsVoxelSize.y * (y + 1), wsVoxelSize.z * (z + 1));

            ColorF voxel_color = ColorF::ZERO;

            bool hit = container->castRay(start, end, STATIC_COLLISION_TYPEMASK, &rayInfo);
            if ( hit && rayInfo.material )
            {
               Material* mat = dynamic_cast<Material*>(rayInfo.material->getMaterial());
               if ( mat && rayInfo.texCoord.x != -1 && rayInfo.texCoord.y != -1 )
               {
                  Resource<GBitmap> diffuseTex = GBitmap::load( mat->mDiffuseMapFilename[0] );
                  if(diffuseTex != NULL)
                  {
                     U32 w = diffuseTex->getWidth();
                     U32 h = diffuseTex->getHeight();

                     ColorI result;
                     diffuseTex->getColor(rayInfo.texCoord.x * w, rayInfo.texCoord.y * h, result);
                  }
               }
               voxel_color = mat->mDiffuse[0];
            }

            mGeometryGrid[pos] = voxel_color;
            mLightGrid[pos] = ColorF::ZERO;
            mPropagationStage = 0;
            mPropagatedLightGridA[pos] = ColorF::ZERO;
            mPropagatedLightGridB[pos] = ColorF::ZERO;
            mPropagatedLightGrid = mPropagatedLightGridA;
            pos++;
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
   if ( !mGeometryGrid || !mLightGrid ) return;

   Box3F worldBox          = getWorldBox();
   Point3F bottom_corner   = worldBox.minExtents;
   Point3F top_corner      = worldBox.maxExtents;
   Point3F wsVoxelSize     = getWorldSpaceVoxelSize();
   Point3I voxelCount      = getVoxelCount();

   // Inject lights from the scene into the light grid.
   U32 pos = 0;
   for ( U32 z = 0; z < voxelCount.z; z++ )
   {
      for ( U32 y = 0; y < voxelCount.y; y++ )
      {
         for ( U32 x = 0; x < voxelCount.x; x++ )
         {
            if ( mGeometryGrid[pos].alpha > 0.0f )
            {
               Point3F point = bottom_corner + Point3F(wsVoxelSize.x * (x + 1), wsVoxelSize.y * (y + 1), wsVoxelSize.z * (z + 1));
               mLightGrid[pos] = calcLightColor(point) * mGeometryGrid[pos];
               mPropagationStage = 0;
            }
            pos++;
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

void OfflineLPV::propagateLights(ColorF* source, ColorF* dest, bool sampleFromGeometry)
{
   if ( !mGeometryGrid || !source || !dest ) return;

   Point3I voxelCount = getVoxelCount();

   // Geometry Grid Visualization.
   U32 pos = 0;
   for ( U32 z = 0; z < voxelCount.z; z++ )
   {
      for ( U32 y = 0; y < voxelCount.y; y++ )
      {
         for ( U32 x = 0; x < voxelCount.x; x++ )
         {
            /* Color Bleeding */
            ColorF blendedColor(0, 0, 0, 0);
            for ( S32 outerX = -1; outerX < 2; outerX++ )
            {
               for ( S32 outerY = -1; outerY < 2; outerY++ )
               {
                  for ( S32 outerZ = -1; outerZ < 2; outerZ++ )
                  {
                     if ( x + outerX < 0 || y + outerY < 0 || z + outerZ < 0 ) continue;
                     if ( x + outerX >= voxelCount.x || y + outerY >= voxelCount.y || z + outerZ >= voxelCount.z ) continue;

                     U32 sampleOffset = (voxelCount.x * voxelCount.y * (z + outerZ)) +
                        (voxelCount.x * (y + outerY)) + (x + outerX);

                     // Attempt at being geometry aware. The idea was to not sample from voxels with geometry in them after the first pass.
                     // First pass would let light bleed into the "air" and then the following passes would sample from "air light".
                     if ( !sampleFromGeometry && mGeometryGrid[sampleOffset].alpha > 0.0f ) continue;

                     blendedColor += (source[sampleOffset]);
                  }
               }
            }

            dest[pos] = blendedColor / 27;
            if ( dest[pos].alpha > 0 )
               dest[pos].alpha = 1.0f;
            pos++;
         }
      }
   }
}

//--- Export from CPU to GPU for rendering ---

bool OfflineLPV::_setExportPropagated( void *object, const char *index, const char *data )
{
   OfflineLPV* volume = reinterpret_cast< OfflineLPV* >( object );
   volume->mExportPropagated = true;
   return false;
}

// Exports mPropagatedLightGrid to the actual volume texture on the graphics card.
// Note: for some reason the volume texture insists on being BGRA.
void OfflineLPV::exportPropagatedLight(ColorF* altSource, Point3I* altSize)
{
   if ( !mPropagatedLightGrid && !altSource ) return;
   ColorF* source = altSource ? altSource : mPropagatedLightGrid;
   Point3I size = altSize ? *altSize : getVoxelCount();

   GFXLockedRect* locked_rect = mPropagatedTexture->lock();
   if ( locked_rect )
   {
      U8* buffer = new U8[size.x * size.y * size.z * 4];
      U32 pos = 0;
      U32 bufPos = 0;
      for(U32 z = 0; z < size.z; z++)
      {
         for(U32 y = 0; y < size.y; y++)
         {
            for(U32 x = 0; x < size.x; x++)
            {
               ColorI cell_color = source[pos];
               pos++;

               buffer[bufPos]     = cell_color.blue;    // Blue
               buffer[bufPos + 1] = cell_color.green;   // Green
               buffer[bufPos + 2] = cell_color.red;     // Red
               buffer[bufPos + 3] = 255;                // Alpha
               bufPos += 4;
            }
         }
      }
      dMemcpy(locked_rect->bits, buffer, size.x * size.y * size.z * 4 * sizeof(U8));
      mPropagatedTexture->unlock();
      SAFE_DELETE(buffer);
   }
}

bool OfflineLPV::_setExportDirectLight( void *object, const char *index, const char *data )
{
   OfflineLPV* volume = reinterpret_cast< OfflineLPV* >( object );
   volume->mExportDirectLight = true;
   return false;
}

// Exports mLightGrid to the actual volume texture on the graphics card.
// Note: for some reason the volume texture insists on being BGRA.
void OfflineLPV::exportDirectLight(ColorF* altSource, Point3I* altSize)
{
   if ( !mLightGrid && !altSource ) return;
   ColorF* source = altSource ? altSource : mLightGrid;
   Point3I size = altSize ? *altSize : getVoxelCount();

   GFXLockedRect* locked_rect = mDirectLightTexture->lock();
   if ( locked_rect )
   {
      U8* buffer = new U8[size.x * size.y * size.z * 4];
      U32 pos = 0;
      U32 bufPos = 0;
      for(U32 z = 0; z < size.z; z++)
      {
         for(U32 y = 0; y < size.y; y++)
         {
            for(U32 x = 0; x < size.x; x++)
            {
               ColorI cell_color = source[pos];
               pos++;

               buffer[bufPos]     = cell_color.blue;    // Blue
               buffer[bufPos + 1] = cell_color.green;   // Green
               buffer[bufPos + 2] = cell_color.red;     // Red
               buffer[bufPos + 3] = 255;                // Alpha
               bufPos += 4;
            }
         }
      }
      dMemcpy(locked_rect->bits, buffer, size.x * size.y * size.z * 4 * sizeof(U8));
      mDirectLightTexture->unlock();
      SAFE_DELETE(buffer);
   }
}

//--- Final Volume Rendering Into LightBuffer ---

void OfflineLPV::_initVolumeTextures(Point3I volumeSize)
{
   // We only perform this function if the voxel count changes in any direction.
   if ( mCachedVolumeSize == volumeSize )
      return;

   // Initialize the shaders if they're null.
   if ( mPropagatedShader.isNull() || mReflectShader.isNull() )
      _initShaders();

   // Initialize the volume texture with all black to begin with.
   U8* buffer = new U8[volumeSize.x * volumeSize.y * volumeSize.z * 4];
   U32 pos = 0;
   for(U32 x = 0; x < volumeSize.x; x++)
   {
      for(U32 y = 0; y < volumeSize.y; y++)
      {
         for(U32 z = 0; z < volumeSize.z; z++)
         {
            buffer[pos]     = 0;   // Blue
            buffer[pos + 1] = 0;   // Green
            buffer[pos + 2] = 0;   // Red
            buffer[pos + 3] = 0;   // Alpha
            pos += 4;
         }
      }
   }

   // 
   mPropagatedTexture.set(volumeSize.x, volumeSize.y, volumeSize.z, &buffer[0], GFXFormat::GFXFormatR8G8B8A8, &LPVProfile, "OfflineLPV_Propagated");
   mDirectLightTexture.set(volumeSize.x, volumeSize.y, volumeSize.z, &buffer[0], GFXFormat::GFXFormatR8G8B8A8, &LPVProfile, "OfflineLPV_DirectLight");

   // Clean up buffer.
   SAFE_DELETE(buffer);
}

bool OfflineLPV::_initShaders()
{
   mPropagatedShader       = NULL;
   mPropagatedShaderConsts = NULL;
   mReflectShader          = NULL;
   mReflectShaderConsts    = NULL;

   // Need depth from pre-pass, so get the macros
   Vector<GFXShaderMacro> macros;

   if ( !mPrepassTarget )
      mPrepassTarget = NamedTexTarget::find( "prepass" );

   if ( mPrepassTarget )
      mPrepassTarget->getShaderMacros( &macros );

   ShaderData *shaderData;

   // Load Propagated Display Shader
   if ( !Sim::findObject( "OfflineLPVPropagatedShaderData", shaderData ) )
   {
      Con::warnf( "OfflineLPV::_initShader - failed to locate shader OfflineLPVPropagatedShaderData!" );
      return false;
   }

   mPropagatedShader = shaderData->getShader( macros );
   if ( !mPropagatedShader )
      return false;
   
   mPropagatedShaderConsts = mPropagatedShader->allocConstBuffer();
   mEyePosWorldPropSC      = mPropagatedShader->getShaderConstHandle( "$eyePosWorld" );
   mRTParamsPropSC         = mPropagatedShader->getShaderConstHandle( "$rtParams0" );
   mVolumeStartPropSC      = mPropagatedShader->getShaderConstHandle( "$volumeStart" );
   mVolumeSizePropSC       = mPropagatedShader->getShaderConstHandle( "$volumeSize" );

   // Load Reflection Shader
   if ( !Sim::findObject( "OfflineLPVReflectShaderData", shaderData ) )
   {
      Con::warnf( "OfflineLPV::_initShader - failed to locate shader OfflineLPVReflectShaderData!" );
      return false;
   }

   mReflectShader = shaderData->getShader( macros );
   if ( !mReflectShader )
      return false;

   mReflectShaderConsts    = mReflectShader->allocConstBuffer();
   mInverseViewReflectSC   = mReflectShader->getShaderConstHandle( "$invViewMat" );
   mEyePosWorldReflectSC   = mReflectShader->getShaderConstHandle( "$eyePosWorld" );
   mRTParamsReflectSC      = mReflectShader->getShaderConstHandle( "$rtParams0" );
   mVolumeStartReflectSC   = mReflectShader->getShaderConstHandle( "$volumeStart" );
   mVolumeSizeReflectSC    = mReflectShader->getShaderConstHandle( "$volumeSize" );

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
      _renderPropagated(sceneState);

      if ( mRenderReflection )
         _renderReflect(sceneState);
   }
}

void OfflineLPV::_renderPropagated(const SceneRenderState* state)
{
   if ( !mPropagatedTexture || !isClientObject() ) 
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
   mPropagatedShaderConsts->setSafe( mEyePosWorldPropSC, camPos );

   // Volume position, used to calculate UV sampling.
   Box3F worldBox = getWorldBox();
   Point3F bottom_corner = worldBox.minExtents;
   Point3F top_corner = worldBox.maxExtents;
   Point3F volume_size = (top_corner - bottom_corner);
   mPropagatedShaderConsts->setSafe(mVolumeStartPropSC, bottom_corner);
   mPropagatedShaderConsts->setSafe(mVolumeSizePropSC, volume_size);

   // Render Target Parameters.
   const Point3I &targetSz = texObject->getSize();
   const RectI &targetVp = mLightInfoTarget->getViewport();
   Point4F rtParams;
   ScreenSpace::RenderTargetParameters(targetSz, targetVp, rtParams);
   mPropagatedShaderConsts->setSafe(mRTParamsPropSC, rtParams);

   GFX->pushActiveRenderTarget();
   GFX->setActiveRenderTarget( mRenderTarget );
   GFX->setVertexBuffer( vb );
   GFX->setStateBlockByDesc( desc );
   GFX->setShader(mPropagatedShader);
   GFX->setShaderConstBuffer(mPropagatedShaderConsts);

   // Setup Textures
   GFX->setTexture(0, mPropagatedTexture);
   GFX->setTexture(1, prepassTexObject);

   // Draw the screenspace quad.
   GFX->drawPrimitive( GFXTriangleFan, 0, 2 );

   // Clean Up
   mRenderTarget->resolve();
   GFX->popActiveRenderTarget();
}

void OfflineLPV::_renderReflect(const SceneRenderState* state)
{
   if ( !mDirectLightTexture || !isClientObject() ) 
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
   mReflectShaderConsts->setSafe( mEyePosWorldReflectSC, camPos );

   MatrixF inverseViewMatrix = state->getWorldViewMatrix();
   inverseViewMatrix.fullInverse();
   inverseViewMatrix.transpose();
   mReflectShaderConsts->setSafe( mInverseViewReflectSC, inverseViewMatrix );

   // Volume position, used to calculate UV sampling.
   Box3F worldBox = getWorldBox();
   Point3F bottom_corner = worldBox.minExtents;
   Point3F top_corner = worldBox.maxExtents;
   Point3F volume_size = (top_corner - bottom_corner);
   mReflectShaderConsts->setSafe(mVolumeStartReflectSC, bottom_corner);
   mReflectShaderConsts->setSafe(mVolumeSizeReflectSC, volume_size);

   // Render Target Parameters.
   const Point3I &targetSz = texObject->getSize();
   const RectI &targetVp = mLightInfoTarget->getViewport();
   Point4F rtParams;
   ScreenSpace::RenderTargetParameters(targetSz, targetVp, rtParams);
   mReflectShaderConsts->setSafe(mRTParamsReflectSC, rtParams);

   GFX->pushActiveRenderTarget();
   GFX->setActiveRenderTarget( mRenderTarget );
   GFX->setVertexBuffer( vb );
   GFX->setStateBlockByDesc( desc );
   GFX->setShader(mReflectShader);
   GFX->setShaderConstBuffer(mReflectShaderConsts);

   // Setup Textures
   GFX->setTexture(0, mDirectLightTexture);
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

//--- Final Volume Saving & Loading ---

bool OfflineLPV::_setSaveResults( void *object, const char *index, const char *data )
{
   OfflineLPV* volume = reinterpret_cast< OfflineLPV* >( object );
   volume->mSaveResults = true;
   return false;
}

bool OfflineLPV::save()
{
   if ( mFileName.length() == 0 || !mLightGrid || !mPropagatedLightGrid ) return false;

   FileStream dtsStream;
   if (dtsStream.open(mFileName, Torque::FS::File::Write))
   {
      Point3I voxelCount = getVoxelCount();

      // Resolution
      dtsStream.write(voxelCount.x);
      dtsStream.write(voxelCount.y);
      dtsStream.write(voxelCount.z);

      U32 pos = 0;
      for(U32 z = 0; z < voxelCount.z; z++)
      {
         for(U32 y = 0; y < voxelCount.y; y++)
         {
            for(U32 x = 0; x < voxelCount.x; x++)
            {
               // Direct Light
               dtsStream.write(mLightGrid[pos].red);
               dtsStream.write(mLightGrid[pos].green);
               dtsStream.write(mLightGrid[pos].blue);
               dtsStream.write(mLightGrid[pos].alpha);

               // Propagated Light
               dtsStream.write(mPropagatedLightGrid[pos].red);
               dtsStream.write(mPropagatedLightGrid[pos].green);
               dtsStream.write(mPropagatedLightGrid[pos].blue);
               dtsStream.write(mPropagatedLightGrid[pos].alpha);

               pos++;
            }
         }
      }

      return true;
   }

   return false;
}

bool OfflineLPV::_setLoadResults( void *object, const char *index, const char *data )
{
   OfflineLPV* volume = reinterpret_cast< OfflineLPV* >( object );
   volume->mLoadResults = true;
   return false;
}

bool OfflineLPV::load()
{
   if ( mFileName.length() == 0 ) return false;

   FileStream dtsStream;
   if (dtsStream.open(mFileName, Torque::FS::File::Read))
   {
      // Resolution
      Point3I size;
      dtsStream.read(&size.x);
      dtsStream.read(&size.y);
      dtsStream.read(&size.z);

      ColorF* directLightGrid = new ColorF[size.x * size.y * size.z];
      ColorF* propagatedLightGrid = new ColorF[size.x * size.y * size.z];

      U32 pos = 0;
      for(U32 z = 0; z < size.z; z++)
      {
         for(U32 y = 0; y < size.y; y++)
         {
            for(U32 x = 0; x < size.x; x++)
            {
               // Direct Light
               dtsStream.read(&directLightGrid[pos].red);
               dtsStream.read(&directLightGrid[pos].green);
               dtsStream.read(&directLightGrid[pos].blue);
               dtsStream.read(&directLightGrid[pos].alpha);

               // Propagated Light
               dtsStream.read(&propagatedLightGrid[pos].red);
               dtsStream.read(&propagatedLightGrid[pos].green);
               dtsStream.read(&propagatedLightGrid[pos].blue);
               dtsStream.read(&propagatedLightGrid[pos].alpha);

               pos++;
            }
         }
      }

      // Export loaded results.
      _initVolumeTextures(size);
      exportPropagatedLight(propagatedLightGrid, &size);
      exportDirectLight(directLightGrid, &size);

      SAFE_DELETE(propagatedLightGrid);
      SAFE_DELETE(directLightGrid);

      return true;
   }

   return false;
}