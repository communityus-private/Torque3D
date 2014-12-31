//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
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
#include "T3D/lightProbeVolume.h"

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

#include "math/mPolyhedron.impl.h"

Vector< SimObjectPtr<SceneObject> > LightProbeVolume::smAccuObjects;
Vector< SimObjectPtr<LightProbeVolume> > LightProbeVolume::smLightProbeVolumes;

//#define DEBUG_DRAW

IMPLEMENT_CO_NETOBJECT_V1( LightProbeVolume );

ConsoleDocClass( LightProbeVolume,
   "@brief An invisible shape that allow objects within it to have an accumulation map.\n\n"

   "LightProbeVolume is used to add additional realism to a scene. It's main use is in outdoor scenes "
   " where objects could benefit from overlaying environment accumulation textures such as sand, snow, etc.\n\n"

   "Objects within the volume must have accumulation enabled in their material. \n\n"

   "@ingroup enviroMisc"
);

//-----------------------------------------------------------------------------

LightProbeVolume::LightProbeVolume()
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
   mAreaEnvMapName = "";
   mAreaEnvMap = NULL;

   resetWorldBox();
}

LightProbeVolume::~LightProbeVolume()
{
   mAreaEnvMap = NULL;
}

void LightProbeVolume::initPersistFields()
{
      addField( "AreaEnvMap", TypeCubemapName, Offset( mAreaEnvMapName, LightProbeVolume ),
         "Cubemap visible during night." );

   Parent::initPersistFields();
}

//-----------------------------------------------------------------------------

void LightProbeVolume::consoleInit()
{
   // Disable rendering of occlusion volumes by default.
   getStaticClassRep()->mIsRenderEnabled = false;
}

//-----------------------------------------------------------------------------

bool LightProbeVolume::onAdd()
{
   if( !Parent::onAdd() )
      return false;

   // Prepare some client side things.
   if ( isClientObject() )  
   {
      smLightProbeVolumes.push_back(this);
      refreshVolumes();
   }

   // Set up the silhouette extractor.
   mSilhouetteExtractor = SilhouetteExtractorType( mPolyhedron );

   return true;
}

void LightProbeVolume::onRemove()
{
   if ( isClientObject() )  
   {
      smLightProbeVolumes.remove(this);
      refreshVolumes();
   }
   Parent::onRemove();
}

//-----------------------------------------------------------------------------

void LightProbeVolume::_renderObject( ObjectRenderInst* ri, SceneRenderState* state, BaseMatInstance* overrideMat )
{
   Parent::_renderObject( ri, state, overrideMat );

   #ifdef DEBUG_DRAW
   if( state->isDiffusePass() )
      DebugDrawer::get()->drawPolyhedronDebugInfo( mPolyhedron, getTransform(), getScale() );
   #endif
}

//-----------------------------------------------------------------------------

void LightProbeVolume::setTransform( const MatrixF& mat )
{
   Parent::setTransform( mat );
   mTransformDirty = true;
   refreshVolumes();
}

//-----------------------------------------------------------------------------

void LightProbeVolume::buildSilhouette( const SceneCameraState& cameraState, Vector< Point3F >& outPoints )
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

U32 LightProbeVolume::packUpdate( NetConnection *connection, U32 mask, BitStream *stream )
{
   U32 retMask = Parent::packUpdate( connection, mask, stream );

   if (stream->writeFlag(mask & InitialUpdateMask))
   {
      stream->write( mAreaEnvMapName );
   }

   return retMask;  
}

void LightProbeVolume::unpackUpdate( NetConnection *connection, BitStream *stream )
{
   Parent::unpackUpdate( connection, stream );

   if (stream->readFlag())
   {
      stream->read( &mAreaEnvMapName );
      setTexture(mAreaEnvMapName);
   }
}

//-----------------------------------------------------------------------------

void LightProbeVolume::inspectPostApply()
{
   Parent::inspectPostApply();
   setMaskBits(U32(-1) );
}

void LightProbeVolume::setTexture( const String& name )
{
   mAreaEnvMapName = name;
   if ( isClientObject() && mAreaEnvMapName.isNotEmpty() )
   {
      Sim::findObject(mAreaEnvMapName, mAreaEnvMap);
      if (mEnvMap)
         Con::warnf( "LightProbeVolume::setTexture - Unable to load cubemap: %s", mAreaEnvMapName.c_str() );
   }
   refreshVolumes();
}

//-----------------------------------------------------------------------------
// Static Functions
//-----------------------------------------------------------------------------
bool LightProbeVolume::_setTexture( void *object, const char *index, const char *data )
{
   LightProbeVolume* volume = reinterpret_cast< LightProbeVolume* >( object );
   volume->setTexture( data );
   return false;
}

void LightProbeVolume::refreshVolumes()
{
   // This function tests each accumulation object to
   // see if it's within the bounds of an accumulation
   // volume. If so, it will pass on the accumulation
   // texture of the volume to the object.

   // This function should only be called when something
   // global like change of volume or material occurs.

   // Clear old data.
   for (S32 n = 0; n < smAccuObjects.size(); ++n)
   {
      SimObjectPtr<SceneObject> object = smAccuObjects[n];
      if ( object.isValid() )
         object->mEnvMap = NULL;
   }

   // 
   for (S32 i = 0; i < smLightProbeVolumes.size(); ++i)
   {
      SimObjectPtr<LightProbeVolume> volume = smLightProbeVolumes[i];
      if ( volume.isNull() ) continue;

      for (S32 n = 0; n < smAccuObjects.size(); ++n)
      {
         SimObjectPtr<SceneObject> object = smAccuObjects[n];
         if ( object.isNull() ) continue;

         if (volume->containsPoint(object->getPosition()) && volume->mAreaEnvMap)
            object->mEnvMap = volume->mAreaEnvMap->mCubemap;
      }
   }
}

// Accumulation Object Management.
void LightProbeVolume::addObject(SimObjectPtr<SceneObject> object)
{
   smAccuObjects.push_back(object);
   refreshVolumes();
}

void LightProbeVolume::removeObject(SimObjectPtr<SceneObject> object)
{
   smAccuObjects.remove(object);
   refreshVolumes();
}

void LightProbeVolume::updateObject(SceneObject* object)
{
   // This function is called when an individual object
   // has been moved. Tests to see if it's in any of the
   // accumulation volumes.

   // We use ZERO instead of NULL so the accumulation
   // texture will be updated in renderMeshMgr.
   object->mEnvMap = NULL;

   for (S32 i = 0; i < smLightProbeVolumes.size(); ++i)
   {
      SimObjectPtr<LightProbeVolume> volume = smLightProbeVolumes[i];
      if ( volume.isNull() ) continue;

      if ( volume->containsPoint(object->getPosition()) && volume->mAreaEnvMap)
         object->mEnvMap = volume->mAreaEnvMap->mCubemap;
   }
}