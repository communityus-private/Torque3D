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

Vector< SimObjectPtr<SceneObject> > LightProbeVolume::smProbedObjects;
Vector< SimObjectPtr<LightProbeVolume> > LightProbeVolume::smLightProbeVolumes;

GFXCubemap * gLevelEnvMap;

//#define DEBUG_DRAW

IMPLEMENT_CO_NETOBJECT_V1( LightProbeVolume );

ConsoleDocClass( LightProbeVolume,
   "@brief An invisible shape that overrides cubemapping on objects it encloses.\n\n"

   "@ingroup enviroMisc"
);

//-----------------------------------------------------------------------------

LightProbeVolume::LightProbeVolume()
   : mTransformDirty( true )
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
         "Environment map applied to objects for a given area." );

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
      if (!mAreaEnvMap)
         Con::warnf("LightProbeVolume::setTexture - Unable to load cubemap: %s", mAreaEnvMapName.c_str());
      else
      {
         if (!mAreaEnvMap->mCubemap)
            mAreaEnvMap->createMap();
         mEnvMap = mAreaEnvMap->mCubemap;
      }
   }
   refreshVolumes();
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
   for (S32 n = 0; n < smProbedObjects.size(); ++n)
   {
      SimObjectPtr<SceneObject> object = smProbedObjects[n];
      if ( object.isValid() )
         object->mEnvMap = gLevelEnvMap;
   }

   // 
   for (S32 i = 0; i < smLightProbeVolumes.size(); ++i)
   {
      SimObjectPtr<LightProbeVolume> volume = smLightProbeVolumes[i];

      if ( volume.isNull() ) continue;

      for (S32 n = 0; n < smProbedObjects.size(); ++n)
      {
         SimObjectPtr<SceneObject> object = smProbedObjects[n];
         if ( object.isNull() ) continue;

         if ((volume->mAreaEnvMap) && !(volume->mAreaEnvMap->mCubemap))
            volume->mAreaEnvMap->createMap();

         if (volume->containsPoint(object->getPosition()))
         {
            if (volume->mAreaEnvMap)
            {
               object->mEnvMap = volume->mAreaEnvMap->mCubemap;
            }
            else
               Con::errorf("Invalid area environment map!");

         }
      }
   }
}

// LightProbe Object Management.
void LightProbeVolume::addObject(SimObjectPtr<SceneObject> object)
{
   smProbedObjects.push_back(object);
   refreshVolumes();
}

void LightProbeVolume::removeObject(SimObjectPtr<SceneObject> object)
{
   smProbedObjects.remove(object);
   refreshVolumes();
}

void LightProbeVolume::updateObject(SceneObject* object)
{
   // This function is called when an individual object
   // has been moved. Tests to see if it's in any of the
   // accumulation volumes.

   // We use ZERO instead of NULL so the accumulation
   // texture will be updated in renderMeshMgr.
   object->mEnvMap = gLevelEnvMap;

   for (S32 i = 0; i < smLightProbeVolumes.size(); ++i)
   {
      SimObjectPtr<LightProbeVolume> volume = smLightProbeVolumes[i];
      if (volume.isNull()) continue;

      if ((volume->mAreaEnvMap) && !(volume->mAreaEnvMap->mCubemap))
         volume->mAreaEnvMap->createMap();

      if (volume->containsPoint(object->getPosition()))
      {
         if (volume->mAreaEnvMap)
         {
            object->mEnvMap = volume->mAreaEnvMap->mCubemap;
         }
         else
            Con::errorf("Invalid area environment map!");

      }
   }
}