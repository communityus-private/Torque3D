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

#include "math/mPolyhedron.impl.h"

//#define DEBUG_DRAW

IMPLEMENT_CO_NETOBJECT_V1( OfflineLPV );

ConsoleDocClass( OfflineLPV,
   "@brief An invisible shape that allow objects within it to have an accumulation map.\n\n"

   "OfflineLPV is used to add additional realism to a scene. It's main use is in outdoor scenes "
   " where objects could benefit from overlaying environment accumulation textures such as sand, snow, etc.\n\n"

   "Objects within the volume must have accumulation enabled in their material. \n\n"

   "@ingroup enviroMisc"
);

static const Point3F cubePoints[8] = 
{
   Point3F(-1, -1, -1), Point3F(-1, -1,  1), Point3F(-1,  1, -1), Point3F(-1,  1,  1),
   Point3F( 1, -1, -1), Point3F( 1, -1,  1), Point3F( 1,  1, -1), Point3F( 1,  1,  1)
};

static const U32 cubeFaces[6][4] = 
{
   { 0, 4, 6, 2 }, { 0, 2, 3, 1 }, { 0, 1, 5, 4 },
   { 3, 2, 6, 7 }, { 7, 6, 4, 5 }, { 3, 7, 5, 1 }
};

static const Point2F cubeUVs[6][4] =
{
   {Point2F(1, 0), Point2F(1, 1), Point2F(0, 1), Point2F(0, 0)}, // Top
   {Point2F(1, 1), Point2F(0, 1), Point2F(0, 0), Point2F(1, 0)},
   {Point2F(0, 1), Point2F(0, 0), Point2F(1, 0), Point2F(1, 1)},
   {Point2F(1, 0), Point2F(1, 1), Point2F(0, 1), Point2F(0, 0)},
   {Point2F(1, 0), Point2F(1, 1), Point2F(0, 1), Point2F(0, 0)},
   {Point2F(1, 0), Point2F(1, 1), Point2F(0, 1), Point2F(0, 0)} // Bottom
};

GFX_ImplementTextureProfile( LPVProfile,
                              GFXTextureProfile::DiffuseMap,
                              GFXTextureProfile::NoMipmap | GFXTextureProfile::Dynamic,
                              GFXTextureProfile::NONE );

//-----------------------------------------------------------------------------

OfflineLPV::OfflineLPV()
   : mTransformDirty( true ),
     mSilhouetteExtractor( mPolyhedron )
{
   VECTOR_SET_ASSOCIATION( mWSPoints );
   VECTOR_SET_ASSOCIATION( mVolumeQueryList );

   //mObjectFlags.set( VisualOccluderFlag );
   
   mNetFlags.set( Ghostable | ScopeAlways );
   mObjScale.set( 1.f, 1.f, 1.f );
   mObjBox.set(
      Point3F( -0.5f, -0.5f, -0.5f ),
      Point3F( 0.5f, 0.5f, 0.5f )
   );

   mObjToWorld.identity();
   mWorldToObj.identity();

   // Accumulation Texture.
   //mTextureName = "";
   //mAccuTexture = NULL;

   mRegenVolume = false;
   mInjectLights = false;
   mPropagateLights = false;
   mShowLightGrid = false;
   mShowPropagatedLightGrid = false;

   mLightInfoTarget = NULL;

   // Generate Random Geometry Grid
   for ( U32 x = 0; x < 10; x++ )
   {
      for ( U32 y = 0; y < 10; y++ )
      {
         for ( U32 z = 0; z < 10; z++ )
         {
            mGeometryGrid[x][y][z] = ( gRandGen.randI(0, 10) < 5 );
         }
      }
   }

   resetWorldBox();

   RenderPassManager::getRenderBinSignal().notify( this, &OfflineLPV::_handleBinEvent );
}

OfflineLPV::~OfflineLPV()
{
   //mAccuTexture = NULL;
}

void OfflineLPV::initPersistFields()
{
   addProtectedField( "regenVolume", TypeBool, Offset( mRegenVolume, OfflineLPV ),
         &_setRegenVolume, &defaultProtectedGetFn, "HACK: flip this to regen volume." );

   addProtectedField( "injectLights", TypeBool, Offset( mInjectLights, OfflineLPV ),
         &_setInjectLights, &defaultProtectedGetFn, "HACK: flip this to regen volume." );

   addProtectedField( "propagateLights", TypeBool, Offset( mPropagateLights, OfflineLPV ),
         &_setPropagateLights, &defaultProtectedGetFn, "HACK: flip this to regen volume." );

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

   // Prepare some client side things.
   if ( isClientObject() )  
   {
      //smAccuVolumes.push_back(this);
      //refreshVolumes();
   }

   // Set up the silhouette extractor.
   mSilhouetteExtractor = SilhouetteExtractorType( mPolyhedron );

   U32 pos = 0;
   for(U32 x = 0; x < LPV_GRID_RESOLUTION; x++)
   {
      for(U32 y = 0; y < LPV_GRID_RESOLUTION; y++)
      {
         for(U32 z = 0; z < LPV_GRID_RESOLUTION; z++)
         {
            mLPVRawData[pos]     = 0;     // Blue
            mLPVRawData[pos + 1] = 0;     // Green
            mLPVRawData[pos + 2] = 0;     // Red
            mLPVRawData[pos + 3] = 255;   // Alpha
            pos += 4;
         }
      }
   }

   mLPVTexture.set(LPV_GRID_RESOLUTION, LPV_GRID_RESOLUTION, LPV_GRID_RESOLUTION, &mLPVRawData[0], GFXFormat::GFXFormatR8G8B8A8, &LPVProfile, "Light Propagation Grid");

   _initShader();

   return true;
}

void OfflineLPV::onRemove()
{
   if ( isClientObject() )  
   {
      //smAccuVolumes.remove(this);
      //refreshVolumes();
   }
   Parent::onRemove();

   mLPVTexture.free();
   mLPVTexture = NULL;
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
      _renderLPV();
   }
}

void OfflineLPV::_renderLPV()
{
   if ( !mRenderTarget )
      mRenderTarget = GFX->allocRenderToTextureTarget();
         
   if ( !mRenderTarget ) return;

   if ( !mLightInfoTarget )
      mLightInfoTarget = NamedTexTarget::find( "lightinfo" );

   GFXTextureObject *texObject = mLightInfoTarget->getTexture();
   if ( !texObject ) return;

   mRenderTarget->attachTexture( GFXTextureTarget::Color0, texObject );

   GFXVertexBufferHandle<GFXVertexPT> verts(GFX, 36, GFXBufferTypeVolatile);
   verts.lock();

   Box3F box = getWorldBox();

   Point3F size = box.getExtents();
   Point3F pos = box.getCenter();
   ColorI color = ColorI(255, 0, 255);
   Point3F halfSize = size * 0.5f;

   GFXStateBlockDesc desc;
   desc.setZReadWrite( true, true );
   desc.setBlend( false );
   desc.setFillModeSolid();
   desc.samplers[0] = GFXSamplerStateDesc::getClampPoint();
   desc.samplersDefined = true;

   // setup 6 line loops
   U32 vertexIndex = 0;
   U32 idx;
   for(S32 i = 0; i < 6; i++)
   {
      idx = cubeFaces[i][0];
      verts[vertexIndex].point = cubePoints[idx] * halfSize;      
      verts[vertexIndex].texCoord = cubeUVs[i][0];
      vertexIndex++;

      idx = cubeFaces[i][1];
      verts[vertexIndex].point = cubePoints[idx] * halfSize;
      verts[vertexIndex].texCoord = cubeUVs[i][1];
      vertexIndex++;

      idx = cubeFaces[i][3];
      verts[vertexIndex].point = cubePoints[idx] * halfSize;
      verts[vertexIndex].texCoord = cubeUVs[i][3];
      vertexIndex++;

      idx = cubeFaces[i][1];
      verts[vertexIndex].point = cubePoints[idx] * halfSize;
      verts[vertexIndex].texCoord = cubeUVs[i][1];
      vertexIndex++;

      idx = cubeFaces[i][2];
      verts[vertexIndex].point = cubePoints[idx] * halfSize;
      verts[vertexIndex].texCoord = cubeUVs[i][2];
      vertexIndex++;

      idx = cubeFaces[i][3];
      verts[vertexIndex].point = cubePoints[idx] * halfSize;
      verts[vertexIndex].texCoord = cubeUVs[i][3];
      vertexIndex++;
   }

   // Apply xfm if we were passed one.
   //if ( xfm != NULL )
   //{
   //   for ( U32 i = 0; i < 36; i++ )
   //      xfm->mulV( verts[i].point );
   //}

   // Apply position offset
   for ( U32 i = 0; i < 36; i++ )
      verts[i].point += pos;

   verts.unlock();

   //MatrixF xfm = getTransform();
   //GFX->multWorld(xfm);
   MatrixF xform(GFX->getProjectionMatrix());
   xform *= GFX->getViewMatrix();
   xform *=  GFX->getWorldMatrix();
   mShaderConsts->setSafe( mModelViewProjSC, xform );

   GFX->pushActiveRenderTarget();
   GFX->setActiveRenderTarget( mRenderTarget );

   GFX->setStateBlockByDesc( desc );
   GFX->setVertexBuffer( verts );
   GFX->setShader(mShader);
   GFX->setShaderConstBuffer(mShaderConsts);
   GFX->setTexture(0, mLPVTexture);

   GFX->drawPrimitive( GFXTriangleList, 0, 12 );

   // Delete the SceneRenderState we allocated.
   mRenderTarget->resolve();
   GFX->popActiveRenderTarget();
}

bool OfflineLPV::_initShader()
{
   ShaderData *shaderData;
   if ( !Sim::findObject( "OfflineLPVShaderData", shaderData ) )
   {
      Con::warnf( "OfflineLPV::_initShader - failed to locate shader OfflineLPVShaderData!" );
      return false;
   }
   
   Vector<GFXShaderMacro> macros;
   mShader = shaderData->getShader( macros );

   if ( !mShader )
      return false;

   mShaderConsts = mShader->allocConstBuffer();
   mModelViewProjSC = mShader->getShaderConstHandle( "$modelView" );

   return true;
}


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
                  if ( mPropagatedLightGrid[x][y][z].alpha <= 0.0f ) continue;

                  Box3F new_box;
                  new_box.set(bottom_corner + Point3F(difference.x * x, difference.y * y, difference.z * z), 
                     bottom_corner + Point3F(difference.x * (x + 1), difference.y * (y + 1), difference.z * (z + 1)));
                  drawer->drawCube( desc, new_box, mPropagatedLightGrid[x][y][z] );
               } else {
                  if ( mLightGrid[x][y][z].alpha <= 0.0f ) continue;

                  Box3F new_box;
                  new_box.set(bottom_corner + Point3F(difference.x * x, difference.y * y, difference.z * z), 
                     bottom_corner + Point3F(difference.x * (x + 1), difference.y * (y + 1), difference.z * (z + 1)));
                  drawer->drawCube( desc, new_box, mLightGrid[x][y][z] );
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

   #ifdef DEBUG_DRAW
   if( state->isDiffusePass() )
      DebugDrawer::get()->drawPolyhedronDebugInfo( mPolyhedron, getTransform(), getScale() );
   #endif
}

//-----------------------------------------------------------------------------

void OfflineLPV::setTransform( const MatrixF& mat )
{
   Parent::setTransform( mat );
   mTransformDirty = true;
   //refreshVolumes();
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

   if (stream->writeFlag(mask & InitialUpdateMask))
   {
     // stream->write( mTextureName );
   }

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

   return retMask;  
}

void OfflineLPV::unpackUpdate( NetConnection *connection, BitStream *stream )
{
   Parent::unpackUpdate( connection, stream );

   if (stream->readFlag())
   {
      //stream->read( &mTextureName );
      //setTexture(mTextureName);
   }

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
      propagateLights();
   }
}

//-----------------------------------------------------------------------------

void OfflineLPV::inspectPostApply()
{
   Parent::inspectPostApply();
   setMaskBits(U32(-1) );
}

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
            mGeometryGrid[x][y][z] = hit;
            mLightGrid[x][y][z] = ColorF::ZERO;
            mPropagatedLightGrid[x][y][z] = ColorF::ZERO;
         }
      }
   }
}

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

   // Geometry Grid Visualization.
   for ( U32 x = 0; x < LPV_GRID_RESOLUTION; x++ )
   {
      for ( U32 y = 0; y < LPV_GRID_RESOLUTION; y++ )
      {
         for ( U32 z = 0; z < LPV_GRID_RESOLUTION; z++ )
         {
            if ( !mGeometryGrid[x][y][z] ) continue;

            Point3F point = bottom_corner + Point3F(half_difference.x * (x + 1), half_difference.y * (y + 1), half_difference.z * (z + 1));
            mLightGrid[x][y][z] = calcLightColor(point);
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

bool OfflineLPV::_setPropagateLights( void *object, const char *index, const char *data )
{
   OfflineLPV* volume = reinterpret_cast< OfflineLPV* >( object );
   volume->mPropagateLights = true;
   return false;
}

void OfflineLPV::propagateLights()
{
   Con::printf("Propagating Lights!");

   Box3F worldBox = getWorldBox();
   Point3F bottom_corner = worldBox.minExtents;
   Point3F top_corner = worldBox.maxExtents;
   Point3F half_difference = ((top_corner - bottom_corner) / LPV_GRID_RESOLUTION) / 2;

   RayInfo rayInfo;
   SceneContainer* container = getContainer();

   // Geometry Grid Visualization.
   for ( U32 x = 0; x < LPV_GRID_RESOLUTION; x++ )
   {
      for ( U32 y = 0; y < LPV_GRID_RESOLUTION; y++ )
      {
         for ( U32 z = 0; z < LPV_GRID_RESOLUTION; z++ )
         {
            ColorF blendedColor(0, 0, 0, 0);
            for ( S32 outerX = -1; outerX < 2; outerX++ )
            {
               for ( S32 outerY = -1; outerY < 2; outerY++ )
               {
                  for ( S32 outerZ = -1; outerZ < 2; outerZ++ )
                  {
                     if ( x + outerX < 0 || y + outerY < 0 || z + outerZ < 0 ) continue;
                     if ( x + outerX >= LPV_GRID_RESOLUTION || y + outerY >= LPV_GRID_RESOLUTION || z + outerZ >= LPV_GRID_RESOLUTION ) continue;

                     /*Con::printf("mLightGrid Color (%f, %f, %f): %f %f %f %f", 
                        x + outerX, y + outerY, z + outerZ,
                        mLightGrid[x + outerX][y + outerY][z + outerZ].red,
                        mLightGrid[x + outerX][y + outerY][z + outerZ].green,
                        mLightGrid[x + outerX][y + outerY][z + outerZ].blue,
                        mLightGrid[x + outerX][y + outerY][z + outerZ].alpha);
                        */

                     blendedColor += mLightGrid[x + outerX][y + outerY][z + outerZ];
                  }
               }
            }

            mPropagatedLightGrid[x][y][z] = blendedColor / 27;
            Con::printf("Propagated Light Color: %f %f %f %f", mPropagatedLightGrid[x][y][z].red,
               mPropagatedLightGrid[x][y][z].green,
               mPropagatedLightGrid[x][y][z].blue,
               mPropagatedLightGrid[x][y][z].alpha);
            if ( mPropagatedLightGrid[x][y][z].alpha > 0 )
               mPropagatedLightGrid[x][y][z].alpha = 1.0f;
         }
      }
   }
}