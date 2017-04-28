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
#include "T3D/reflectionProbe.h"

#include "math/mathIO.h"
#include "scene/sceneRenderState.h"
#include "console/consoleTypes.h"
#include "core/stream/bitStream.h"
#include "materials/materialManager.h"
#include "materials/baseMatInstance.h"
#include "renderInstance/renderPassManager.h"
#include "lighting/lightQuery.h"
#include "console/engineAPI.h"

#include "gfx/gfxDrawUtil.h"

#include "lighting/advanced/advancedLightBinManager.h"

#include "gfx/gfxDebugEvent.h"
#include "gfx/gfxTransformSaver.h"
#include "math/mathUtils.h"
#include "math/util/frustum.h"
#include "gfx/bitmap/gBitmap.h"
#include "gfx/gfxTextureHandle.h"
#include "core/stream/fileStream.h"
#include "core/fileObject.h"
#include "lighting/advanced/advancedLightManager.h"
#include "core/resourceManager.h"

#include "console/simPersistId.h"
#include <string>
#include "postFx/postEffectManager.h"
#include "T3D/gameFunctions.h"
#include "gui/core/guiOffscreenCanvas.h"
#include "postFx/postEffect.h"

//
#include "platform/platform.h"
#include "T3D/gameFunctions.h"
#include "gui/3d/guiTSControl.h"
#include "renderInstance/renderProbeMgr.h"
//

extern bool gEditingMission;
extern ColorI gCanvasClearColor;
bool ReflectionProbe::smRenderReflectionProbes = true;

/*#include "platform/platform.h"
#include "lighting/lightInfo.h"

#include "math/mMath.h"
#include "core/color.h"
#include "gfx/gfxCubemap.h"
#include "console/simObject.h"
#include "math/mathUtils.h"*/

IMPLEMENT_CO_NETOBJECT_V1(ReflectionProbe);

ConsoleDocClass(ReflectionProbe,
   "@brief An example scene object which renders a mesh.\n\n"
   "This class implements a basic SceneObject that can exist in the world at a "
   "3D position and render itself. There are several valid ways to render an "
   "object in Torque. This class implements the preferred rendering method which "
   "is to submit a MeshRenderInst along with a Material, vertex buffer, "
   "primitive buffer, and transform and allow the RenderMeshMgr handle the "
   "actual setup and rendering for you.\n\n"
   "See the C++ code for implementation details.\n\n"
   "@ingroup Examples\n");

ImplementEnumType(ReflectProbeType,
   "Type of mesh data available in a shape.\n"
   "@ingroup gameObjects")
{ ProbeRenderInst::Sphere, "Sphere", "Sphere shaped" },
{ ProbeRenderInst::Box, "Box", "Box shape" }
EndImplementEnumType;

ImplementEnumType(IndrectLightingModeEnum,
   "Type of mesh data available in a shape.\n"
   "@ingroup gameObjects")
{ ReflectionProbe::NoIndirect, "No Lighting", "This probe does not provide any local indirect lighting data" },
{ ReflectionProbe::AmbientColor, "Ambient Color", "Adds a flat color to act as the local indirect lighting" },
{ ReflectionProbe::SphericalHarmonics, "Spherical Harmonics", "Creates spherical harmonics data based off the reflection data" },
   EndImplementEnumType;

ImplementEnumType(ReflectionModeEnum,
   "Type of mesh data available in a shape.\n"
   "@ingroup gameObjects")
{ ReflectionProbe::NoReflection, "No Reflections", "This probe does not provide any local reflection data"},
{ ReflectionProbe::StaticCubemap, "Static Cubemap", "Uses a static CubemapData" },
{ ReflectionProbe::BakedCubemap, "Baked Cubemap", "Uses a cubemap baked from the probe's current position" },
{ ReflectionProbe::SkyLight, "Skylight", "Captures a cubemap of the sky and far terrain, designed to influence the whole level" }
   EndImplementEnumType;

//-----------------------------------------------------------------------------
// Object setup and teardown
//-----------------------------------------------------------------------------
ReflectionProbe::ReflectionProbe()
{
   // Flag this object so that it will always
   // be sent across the network to clients
   mNetFlags.set(Ghostable | ScopeAlways);

   mTypeMask = LightObjectType | MarkerObjectType;

   // Make sure we the Material instance to NULL
   // so we don't try to access it incorrectly
   mMaterialInst = NULL;

   mProbeShapeType = ProbeRenderInst::Sphere;

   mIndrectLightingModeType = AmbientColor;
   mAmbientColor = ColorF(1, 1, 1, 1);
   mSphericalHarmonics = ColorF(0,0,0,1);

   mReflectionModeType = BakedCubemap;

   mEnabled = true;
   mBake = false;
   mDirty = false;

   mProbeInfo = new ProbeRenderInst();

   mRadius = 10;
   mIntensity = 1.0f;

   mUseCubemap = false;
   mCubemap = NULL;
   mReflectionPath = "";
   mProbeUniqueID = "";

   mEditorShapeInst = NULL;
   mEditorShape = NULL;
}

ReflectionProbe::~ReflectionProbe()
{
   if (mMaterialInst)
      SAFE_DELETE(mMaterialInst);

   if (mEditorShapeInst)
      SAFE_DELETE(mEditorShapeInst);

   if (mCubemap)
      mCubemap->deleteObject();
}

//-----------------------------------------------------------------------------
// Object Editing
//-----------------------------------------------------------------------------
void ReflectionProbe::initPersistFields()
{
   addGroup("Rendering");
      addProtectedField("enabled", TypeBool, Offset(mEnabled, ReflectionProbe),
         &_setEnabled, &defaultProtectedGetFn, "Regenerate Voxel Grid");

      addField("ProbeShape", TypeReflectProbeType, Offset(mProbeShapeType, ReflectionProbe),
         "The type of mesh data to use for collision queries.");
      addField("radius", TypeF32, Offset(mRadius, ReflectionProbe), "The name of the material used to render the mesh.");

      addField("Intensity", TypeF32, Offset(mIntensity, ReflectionProbe), "Path of file to save and load results.");
   endGroup("Rendering");

   addGroup("IndirectLighting");
      addField("IndirectLightMode", TypeIndrectLightingModeEnum, Offset(mIndrectLightingModeType, ReflectionProbe),
         "The type of mesh data to use for collision queries.");

      addField("IndirectLight", TypeColorF, Offset(mAmbientColor, ReflectionProbe), "Path of file to save and load results.");
   endGroup("IndirectLighting");

   addGroup("Reflection");
      addField("ReflectionMode", TypeReflectionModeEnum, Offset(mReflectionModeType, ReflectionProbe),
         "The type of mesh data to use for collision queries.");

      addField("reflectionPath", TypeImageFilename, Offset(mReflectionPath, ReflectionProbe),
         "The type of mesh data to use for collision queries.");

      addField("StaticCubemap", TypeCubemapName, Offset(mCubemapName, ReflectionProbe), "Cubemap used instead of reflection texture if fullReflect is off.");

      addProtectedField("Bake", TypeBool, Offset(mBake, ReflectionProbe),
         &_doBake, &defaultProtectedGetFn, "Regenerate Voxel Grid", AbstractClassRep::FieldFlags::FIELD_ComponentInspectors);
   endGroup("Reflection");

   addGroup("CubemapMode");
     
   endGroup("CubemapMode");

   addGroup("ColorMode");
      
   endGroup("ColorMode");

   addGroup("Baking");
      
   endGroup("Baking");

   Con::addVariable("$Light::renderReflectionProbes", TypeBool, &ReflectionProbe::smRenderReflectionProbes,
      "Toggles rendering of light frustums when the light is selected in the editor.\n\n"
      "@note Only works for shadow mapped lights.\n\n"
      "@ingroup Lighting");

   // SceneObject already handles exposing the transform
   Parent::initPersistFields();
}

void ReflectionProbe::inspectPostApply()
{
   Parent::inspectPostApply();

   mDirty = true;

   // Flag the network mask to send the updates
   // to the client object
   setMaskBits(-1);
}

bool ReflectionProbe::_setEnabled(void *object, const char *index, const char *data)
{
   ReflectionProbe* probe = reinterpret_cast< ReflectionProbe* >(object);

   probe->mEnabled = dAtob(data);
   probe->setMaskBits(-1);

   return true;
}

bool ReflectionProbe::_doBake(void *object, const char *index, const char *data)
{
   ReflectionProbe* probe = reinterpret_cast< ReflectionProbe* >(object);

   if (probe->mDirty)
      probe->bake(probe->mReflectionPath, 256);

   return false;
}

bool ReflectionProbe::onAdd()
{
   if (!Parent::onAdd())
      return false;

   mObjBox.minExtents.set(-1, -1, -1);
   mObjBox.maxExtents.set(1, 1, 1);
   mObjScale.set(mRadius, mRadius, mRadius);

   // Skip our transform... it just dirties mask bits.
   Parent::setTransform(mObjToWorld);

   resetWorldBox();

   // Add this object to the scene
   addToScene();

   if (isServerObject())
   {
      if (!mPersistentId)
         mPersistentId = getOrCreatePersistentId();

      mProbeUniqueID = std::to_string(mPersistentId->getUUID().getHash()).c_str();
   }

   //Store the hash as our probeID string

   // Refresh this object's material (if any)
   if (isClientObject())
      updateMaterial();

   /*Point3F origin = Point3F(-0.5000000, 0.5000000, -0.5000000);
   Point3F vecs[3];
   vecs[0] = Point3F(1.0000000, 0.0000000, 0.0000000);
   vecs[1] = Point3F(0.0000000, -1.0000000, 0.0000000);
   vecs[2] = Point3F(0.0000000, 0.0000000, 1.0000000);

   origin += getPosition();

   mPolyhedron.pointList.setSize(8);
   mPolyhedron.pointList[0] = origin;
   mPolyhedron.pointList[1] = origin + vecs[0];
   mPolyhedron.pointList[2] = origin + vecs[1];
   mPolyhedron.pointList[3] = origin + vecs[2];
   mPolyhedron.pointList[4] = origin + vecs[0] + vecs[1];
   mPolyhedron.pointList[5] = origin + vecs[0] + vecs[2];
   mPolyhedron.pointList[6] = origin + vecs[1] + vecs[2];
   mPolyhedron.pointList[7] = origin + vecs[0] + vecs[1] + vecs[2];

   Point3F normal;
   mPolyhedron.planeList.setSize(6);

   mCross(vecs[2], vecs[0], &normal);
   mPolyhedron.planeList[0].set(origin, normal);
   mCross(vecs[0], vecs[1], &normal);
   mPolyhedron.planeList[1].set(origin, normal);
   mCross(vecs[1], vecs[2], &normal);
   mPolyhedron.planeList[2].set(origin, normal);
   mCross(vecs[1], vecs[0], &normal);
   mPolyhedron.planeList[3].set(mPolyhedron.pointList[7], normal);
   mCross(vecs[2], vecs[1], &normal);
   mPolyhedron.planeList[4].set(mPolyhedron.pointList[7], normal);
   mCross(vecs[0], vecs[2], &normal);
   mPolyhedron.planeList[5].set(mPolyhedron.pointList[7], normal);

   mPolyhedron.edgeList.setSize(12);
   mPolyhedron.edgeList[0].vertex[0] = 0; mPolyhedron.edgeList[0].vertex[1] = 1; mPolyhedron.edgeList[0].face[0] = 0; mPolyhedron.edgeList[0].face[1] = 1;
   mPolyhedron.edgeList[1].vertex[0] = 1; mPolyhedron.edgeList[1].vertex[1] = 5; mPolyhedron.edgeList[1].face[0] = 0; mPolyhedron.edgeList[1].face[1] = 4;
   mPolyhedron.edgeList[2].vertex[0] = 5; mPolyhedron.edgeList[2].vertex[1] = 3; mPolyhedron.edgeList[2].face[0] = 0; mPolyhedron.edgeList[2].face[1] = 3;
   mPolyhedron.edgeList[3].vertex[0] = 3; mPolyhedron.edgeList[3].vertex[1] = 0; mPolyhedron.edgeList[3].face[0] = 0; mPolyhedron.edgeList[3].face[1] = 2;
   mPolyhedron.edgeList[4].vertex[0] = 3; mPolyhedron.edgeList[4].vertex[1] = 6; mPolyhedron.edgeList[4].face[0] = 3; mPolyhedron.edgeList[4].face[1] = 2;
   mPolyhedron.edgeList[5].vertex[0] = 6; mPolyhedron.edgeList[5].vertex[1] = 2; mPolyhedron.edgeList[5].face[0] = 2; mPolyhedron.edgeList[5].face[1] = 5;
   mPolyhedron.edgeList[6].vertex[0] = 2; mPolyhedron.edgeList[6].vertex[1] = 0; mPolyhedron.edgeList[6].face[0] = 2; mPolyhedron.edgeList[6].face[1] = 1;
   mPolyhedron.edgeList[7].vertex[0] = 1; mPolyhedron.edgeList[7].vertex[1] = 4; mPolyhedron.edgeList[7].face[0] = 4; mPolyhedron.edgeList[7].face[1] = 1;
   mPolyhedron.edgeList[8].vertex[0] = 4; mPolyhedron.edgeList[8].vertex[1] = 2; mPolyhedron.edgeList[8].face[0] = 1; mPolyhedron.edgeList[8].face[1] = 5;
   mPolyhedron.edgeList[9].vertex[0] = 4; mPolyhedron.edgeList[9].vertex[1] = 7; mPolyhedron.edgeList[9].face[0] = 4; mPolyhedron.edgeList[9].face[1] = 5;
   mPolyhedron.edgeList[10].vertex[0] = 5; mPolyhedron.edgeList[10].vertex[1] = 7; mPolyhedron.edgeList[10].face[0] = 3; mPolyhedron.edgeList[10].face[1] = 4;
   mPolyhedron.edgeList[11].vertex[0] = 7; mPolyhedron.edgeList[11].vertex[1] = 6; mPolyhedron.edgeList[11].face[0] = 3; mPolyhedron.edgeList[11].face[1] = 5;*/

   setMaskBits(-1);

   return true;
}

void ReflectionProbe::onRemove()
{
   // Remove this object from the scene
   removeFromScene();

   Parent::onRemove();
}

void ReflectionProbe::setTransform(const MatrixF & mat)
{
   // Let SceneObject handle all of the matrix manipulation
   Parent::setTransform(mat);

   mDirty = true;

   // Dirty our network mask so that the new transform gets
   // transmitted to the client object
   setMaskBits(TransformMask);
}

U32 ReflectionProbe::packUpdate(NetConnection *conn, U32 mask, BitStream *stream)
{
   // Allow the Parent to get a crack at writing its info
   U32 retMask = Parent::packUpdate(conn, mask, stream);

   // Write our transform information
   if (stream->writeFlag(mask & TransformMask))
   {
      mathWrite(*stream, getTransform());
      mathWrite(*stream, getScale());
   }

   // Write out any of the updated editable properties
   if (stream->writeFlag(mask & UpdateMask))
      stream->write(mMaterialName);

   stream->write((U32)mProbeShapeType);

   stream->write((U32)mIndrectLightingModeType);
   stream->write(mAmbientColor);

   stream->write((U32)mReflectionModeType);

   stream->writeFloat(mIntensity, 7);
   stream->write(mRadius);

   stream->write(mReflectionPath);
   stream->write(mProbeUniqueID);

   stream->writeFlag(mEnabled);

   stream->writeFlag(mUseCubemap);
   stream->write(mCubemapName);

   //if (stream->writeFlag(mask & PolyMask))
   /*{
      U32 i;
      stream->write(mPolyhedron.pointList.size());
      for (i = 0; i < mPolyhedron.pointList.size(); i++)
         mathWrite(*stream, mPolyhedron.pointList[i]);

      stream->write(mPolyhedron.planeList.size());
      for (i = 0; i < mPolyhedron.planeList.size(); i++)
         mathWrite(*stream, mPolyhedron.planeList[i]);

      stream->write(mPolyhedron.edgeList.size());
      for (i = 0; i < mPolyhedron.edgeList.size(); i++) {
         const Polyhedron::Edge& rEdge = mPolyhedron.edgeList[i];

         stream->write(rEdge.face[0]);
         stream->write(rEdge.face[1]);
         stream->write(rEdge.vertex[0]);
         stream->write(rEdge.vertex[1]);
      }
   }*/

   return retMask;
}

void ReflectionProbe::unpackUpdate(NetConnection *conn, BitStream *stream)
{
   // Let the Parent read any info it sent
   Parent::unpackUpdate(conn, stream);

   if (stream->readFlag())  // TransformMask
   {
      mathRead(*stream, &mObjToWorld);
      mathRead(*stream, &mObjScale);

      setTransform(mObjToWorld);
   }

   if (stream->readFlag())  // UpdateMask
   {
      stream->read(&mMaterialName);

      if (isProperlyAdded())
         updateMaterial();
   }

   U32 shapeType = ProbeRenderInst::Sphere;
   stream->read(&shapeType);
   mProbeShapeType = (ProbeRenderInst::ProbeShapeType)shapeType;

   U32 indirectModeType = AmbientColor;
   stream->read(&indirectModeType);
   if ((IndrectLightingModeType)indirectModeType != NoIndirect)
   {
      mIndrectLightingModeType = (IndrectLightingModeType)indirectModeType;
   }

   stream->read(&mAmbientColor);

   U32 reflectModeType = BakedCubemap;
   stream->read(&reflectModeType);
   if ((ReflectionModeType)reflectModeType != BakedCubemap && (ReflectionModeType)reflectModeType != NoReflection)
   {
      mReflectionModeType = (ReflectionModeType)reflectModeType;
   }

   mIntensity = stream->readFloat(7);
   stream->read(&mRadius);

   stream->read(&mReflectionPath);
   stream->read(&mProbeUniqueID);

   mEnabled = stream->readFlag();

   mUseCubemap = stream->readFlag();
   stream->read(&mCubemapName);

   if (mCubemapName.isNotEmpty() && mReflectionModeType == ReflectionModeType::StaticCubemap)
      Sim::findObject(mCubemapName, mCubemap);

   updateMaterial();
}

//-----------------------------------------------------------------------------
// Object Rendering
//-----------------------------------------------------------------------------

void ReflectionProbe::updateMaterial()
{
   mProbeInfo->setPosition(getPosition());

   if (!mCubemap)
   {
      mCubemap = new CubemapData();
      mCubemap->registerObject();
   }

   if (!mProbeUniqueID.isEmpty())
   {
      bool validCubemap = true;

      for (U32 i = 0; i < 6; ++i)
      {
         char faceFile[256];
         dSprintf(faceFile, sizeof(faceFile), "%s%s_%i.png", mReflectionPath.c_str(),
            mProbeUniqueID.c_str(), i);

         if (Platform::isFile(faceFile))
            mCubemap->setCubeFaceFile(i, FileName(faceFile));
         else
            validCubemap = false;
      }

      if (validCubemap)
      {
         mCubemap->createMap();
         mCubemap->updateFaces();
      }
   }

   mProbeInfo->mCubemap = mCubemap;
   
   mProbeInfo->mIntensity = mIntensity;
   mProbeInfo->mRadius = mRadius;

   if (mIndrectLightingModeType == AmbientColor)
   {
      mProbeInfo->mAmbient = mAmbientColor;
   }
   else
   {
      mProbeInfo->mAmbient = ColorF(0, 0, 0, 0);
   }

   mProbeInfo->mProbeShapeType = mProbeShapeType;
}

void ReflectionProbe::prepRenderImage(SceneRenderState *state)
{
   if (!mEnabled)
      return;

   // If the light is selected or light visualization
   // is enabled then register the callback.
   const bool isSelectedInEditor = (gEditingMission && isSelected());
   if (isSelectedInEditor)
   {
      ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
      ri->renderDelegate.bind(this, &ReflectionProbe::_onRenderViz);
      ri->type = RenderPassManager::RIT_Editor;
      state->getRenderPass()->addInst(ri);
   }

   if (gEditingMission && ReflectionProbe::smRenderReflectionProbes)
   {
      if (!mEditorShapeInst)
         return;

      GFXTransformSaver saver;

      // Calculate the distance of this object from the camera
      Point3F cameraOffset;
      getRenderTransform().getColumn(3, &cameraOffset);
      cameraOffset -= state->getDiffuseCameraPosition();
      F32 dist = cameraOffset.len();
      if (dist < 0.01f)
         dist = 0.01f;

      // Set up the LOD for the shape
      F32 invScale = (1.0f / getMax(getMax(mObjScale.x, mObjScale.y), mObjScale.z));

      mEditorShapeInst->setDetailFromDistance(state, dist * invScale);

      // Make sure we have a valid level of detail
      if (mEditorShapeInst->getCurrentDetail() < 0)
         return;

      // GFXTransformSaver is a handy helper class that restores
      // the current GFX matrices to their original values when
      // it goes out of scope at the end of the function

      // Set up our TS render state      
      TSRenderState rdata;
      rdata.setSceneState(state);
      rdata.setFadeOverride(1.0f);

      // We might have some forward lit materials
      // so pass down a query to gather lights.
      LightQuery query;
      query.init(getWorldSphere());
      rdata.setLightQuery(&query);

      // Set the world matrix to the objects render transform
      MatrixF mat = getRenderTransform();
      mat.scale(Point3F(1,1,1));
      GFX->setWorldMatrix(mat);

      // Animate the the shape
      mEditorShapeInst->animate();

      // Allow the shape to submit the RenderInst(s) for itself
      mEditorShapeInst->render(rdata);

      saver.restore();
   }

   //Submit our probe to actually do the probe action
   // Get a handy pointer to our RenderPassmanager
   RenderPassManager *renderPass = state->getRenderPass();

   // Allocate an MeshRenderInst so that we can submit it to the RenderPassManager
   ProbeRenderInst *probeInst = renderPass->allocInst<ProbeRenderInst>();

   probeInst->set(mProbeInfo);

   probeInst->type = RenderPassManager::RIT_Probes;

   // Submit our RenderInst to the RenderPassManager
   state->getRenderPass()->addInst(probeInst);
}

void ReflectionProbe::_onRenderViz(ObjectRenderInst *ri,
   SceneRenderState *state,
   BaseMatInstance *overrideMat)
{
   if (overrideMat || !ReflectionProbe::smRenderReflectionProbes)
      return;

   GFXDrawUtil *draw = GFX->getDrawUtil();

   GFXStateBlockDesc desc;
   desc.setZReadWrite(true, false);
   desc.setCullMode(GFXCullNone);
   desc.setBlend(true);

   // Base the sphere color on the light color.
   ColorI color = ColorI::WHITE;
   color.alpha = 50;

   if (mProbeShapeType == ProbeRenderInst::Sphere)
   {
      draw->drawSphere(desc, mRadius, getPosition(), color);
   }
   else
   {
      Box3F cube(mRadius);
      draw->drawCube(desc, cube, color);
   }
}

DefineEngineMethod(ReflectionProbe, postApply, void, (), ,
   "A utility method for forcing a network update.\n")
{
   object->inspectPostApply();
}

void ReflectionProbe::bake(String outputPath, S32 resolution)
{
   GFXDEBUGEVENT_SCOPE(ReflectionProbe_Bake, ColorI::WHITE);

   PostEffect *preCapture = dynamic_cast<PostEffect*>(Sim::findObject("AL_PreCapture"));
   PostEffect *deferredShading = dynamic_cast<PostEffect*>(Sim::findObject("AL_DeferredShading"));
   if (preCapture)
      preCapture->enable();
   if (deferredShading)
      deferredShading->disable();
   if (mReflectionModeType == StaticCubemap || mReflectionModeType == BakedCubemap || mReflectionModeType == SkyLight)
   {
      if (!mCubemap)
      {
         mCubemap = new CubemapData();
         mCubemap->registerObject();
      }
   }

   if (mReflectionPath.isEmpty() || !mPersistentId)
   {
      if (!mPersistentId)
         mPersistentId = getOrCreatePersistentId();

      mReflectionPath = outputPath.c_str();

      mProbeUniqueID = std::to_string(mPersistentId->getUUID().getHash()).c_str();
   }

   bool validCubemap = true;

   // Save the current transforms so we can restore
   // it for child control rendering below.
   GFXTransformSaver saver;

   //bool saveEditingMission = gEditingMission;
   //gEditingMission = false;

   //Set this to true to use the prior method where it goes through the SPT_Reflect path for the bake
   bool probeRenderState = ReflectionProbe::smRenderReflectionProbes;
   ReflectionProbe::smRenderReflectionProbes = false;
   for (U32 i = 0; i < 6; ++i)
   {
      GFXTexHandle blendTex;
      blendTex.set(resolution, resolution, GFXFormatR8G8B8A8, &GFXRenderTargetProfile, "");

      GFXTextureTargetRef mBaseTarget = GFX->allocRenderToTextureTarget();
      mBaseTarget->attachTexture(GFXTextureTarget::Color0, blendTex);

      // Standard view that will be overridden below.
      VectorF vLookatPt(0.0f, 0.0f, 0.0f), vUpVec(0.0f, 0.0f, 0.0f), vRight(0.0f, 0.0f, 0.0f);

      switch (i)
      {
      case 0: // D3DCUBEMAP_FACE_POSITIVE_X:
         vLookatPt = VectorF(1.0f, 0.0f, 0.0f);
         vUpVec = VectorF(0.0f, 1.0f, 0.0f);
         break;
      case 1: // D3DCUBEMAP_FACE_NEGATIVE_X:
         vLookatPt = VectorF(-1.0f, 0.0f, 0.0f);
         vUpVec = VectorF(0.0f, 1.0f, 0.0f);
         break;
      case 2: // D3DCUBEMAP_FACE_POSITIVE_Y:
         vLookatPt = VectorF(0.0f, 1.0f, 0.0f);
         vUpVec = VectorF(0.0f, 0.0f, -1.0f);
         break;
      case 3: // D3DCUBEMAP_FACE_NEGATIVE_Y:
         vLookatPt = VectorF(0.0f, -1.0f, 0.0f);
         vUpVec = VectorF(0.0f, 0.0f, 1.0f);
         break;
      case 4: // D3DCUBEMAP_FACE_POSITIVE_Z:
         vLookatPt = VectorF(0.0f, 0.0f, 1.0f);
         vUpVec = VectorF(0.0f, 1.0f, 0.0f);
         break;
      case 5: // D3DCUBEMAP_FACE_NEGATIVE_Z:
         vLookatPt = VectorF(0.0f, 0.0f, -1.0f);
         vUpVec = VectorF(0.0f, 1.0f, 0.0f);
         break;
      }

      // create camera matrix
      VectorF cross = mCross(vUpVec, vLookatPt);
      cross.normalizeSafe();

      MatrixF matView(true);
      matView.setColumn(0, cross);
      matView.setColumn(1, vLookatPt);
      matView.setColumn(2, vUpVec);
      matView.setPosition(getPosition());
      matView.inverse();

      // set projection to 90 degrees vertical and horizontal
      F32 left, right, top, bottom;
      F32 nearPlane = 0.01;
      F32 farDist = 1000;

      if (mReflectionModeType == SkyLight)
      {
         nearPlane = 1000;
         farDist = 10000;
      }

      MathUtils::makeFrustum(&left, &right, &top, &bottom, M_HALFPI_F, 1.0f, nearPlane);
      Frustum frustum(false, left, right, top, bottom, nearPlane, farDist);

      renderFrame(&mBaseTarget, matView, frustum, StaticObjectType | StaticShapeObjectType & EDITOR_RENDER_TYPEMASK, gCanvasClearColor);

      mBaseTarget->resolve();

      char fileName[256];
      dSprintf(fileName, 256, "%s%s_%i.png", mReflectionPath.c_str(),
         mProbeUniqueID.c_str(), i);

      FileStream stream;
      if (!stream.open(fileName, Torque::FS::File::Write))
      {
         Con::errorf("ReflectionProbe::bake(): Couldn't open cubemap face file fo writing " + String(fileName));
         if (preCapture)
            preCapture->disable();
         if (deferredShading)
            deferredShading->enable();
         return;
      }

      GBitmap bitmap(blendTex->getWidth(), blendTex->getHeight(), false, GFXFormatR8G8B8);
      blendTex->copyToBmp(&bitmap);
      bitmap.writeBitmap("png", stream);

      if (Platform::isFile(fileName) && mCubemap)
         mCubemap->setCubeFaceFile(i, FileName(fileName));
      else
         validCubemap = false;

      bitmap.deleteImage();
   }

   if (validCubemap)
   {
      if (mCubemap->mCubemap)
         mCubemap->updateFaces();
      else
         mCubemap->createMap();

      mDirty = false;
   }
   ReflectionProbe::smRenderReflectionProbes = probeRenderState;
   setMaskBits(-1);
   if (preCapture)
      preCapture->disable();
   if (deferredShading)
      deferredShading->enable();
}

DefineEngineMethod(ReflectionProbe, Bake, void, (String outputPath, S32 resolution), ("", 256),
   "@brief returns true if control object is inside the fog\n\n.")
{
   object->bake(outputPath, resolution);
   //object->renderFrame(false);
}