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

#include "T3D/reflectionProbe.h"
#include "math/mathIO.h"
#include "scene/sceneRenderState.h"
#include "console/consoleTypes.h"
#include "core/stream/bitStream.h"
#include "materials/baseMatInstance.h"
#include "console/engineAPI.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/gfxDebugEvent.h"
#include "gfx/gfxTransformSaver.h"
#include "math/mathUtils.h"
#include "gfx/bitmap/gBitmap.h"
#include "core/stream/fileStream.h"
#include "core/fileObject.h"
#include "core/resourceManager.h"
#include "console/simPersistId.h"
#include <string>
#include "T3D/gameFunctions.h"
#include "postFx/postEffect.h"
#include "renderInstance/renderProbeMgr.h"

#include "math/util/sphereMesh.h"
#include "materials/materialManager.h"
#include "math/util/matrixSet.h"

extern bool gEditingMission;
extern ColorI gCanvasClearColor;
bool ReflectionProbe::smRenderReflectionProbes = true;

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

   // Refresh this object's material (if any)
   if (isClientObject())
      updateMaterial();
  
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

   createGeometry();
   updateMaterial();
}

void ReflectionProbe::createGeometry()
{
   // Clean up our previous shape
   if (mEditorShapeInst)
      SAFE_DELETE(mEditorShapeInst);
   
   mEditorShape = NULL;
   
   String shapeFile = "tools/resources/ReflectProbeSphere.dae";
   
   // Attempt to get the resource from the ResourceManager
   mEditorShape = ResourceManager::get().load(shapeFile);
   if (mEditorShape)
   {
      mEditorShapeInst = new TSShapeInstance(mEditorShape, isClientObject());
   }
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

   //Submit our probe to actually do the probe action
   // Get a handy pointer to our RenderPassmanager
   RenderPassManager *renderPass = state->getRenderPass();

   // Allocate an MeshRenderInst so that we can submit it to the RenderPassManager
   ProbeRenderInst *probeInst = renderPass->allocInst<ProbeRenderInst>();

   probeInst->set(mProbeInfo);

   probeInst->type = RenderPassManager::RIT_Probes;

   // Submit our RenderInst to the RenderPassManager
   state->getRenderPass()->addInst(probeInst);

   if (gEditingMission && mEditorShapeInst)
   {
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

      BaseMatInstance* probePrevMat = mEditorShapeInst->getMaterialList()->getMaterialInst(0);

      setPreviewMatParameters(state, probePrevMat);

      // GFXTransformSaver is a handy helper class that restores
      // the current GFX matrices to their original values when
      // it goes out of scope at the end of the function

      // Set up our TS render state      
      TSRenderState rdata;
      rdata.setSceneState(state);
      rdata.setFadeOverride(1.0f);
      rdata.setCubemap(mCubemap->mCubemap);

      // We might have some forward lit materials
      // so pass down a query to gather lights.
      LightQuery query;
      query.init(getWorldSphere());
      rdata.setLightQuery(&query);

      // Set the world matrix to the objects render transform
      MatrixF mat = getRenderTransform();
      mat.scale(Point3F(1, 1, 1));
      GFX->setWorldMatrix(mat);

      // Animate the the shape
      mEditorShapeInst->animate();

      // Allow the shape to submit the RenderInst(s) for itself
      mEditorShapeInst->render(rdata);

      saver.restore();
   }

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
}

void ReflectionProbe::_onRenderViz(ObjectRenderInst *ri,
   SceneRenderState *state,
   BaseMatInstance *overrideMat)
{
   if (!ReflectionProbe::smRenderReflectionProbes)
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

void ReflectionProbe::setPreviewMatParameters(SceneRenderState* renderState, BaseMatInstance* mat)
{
   //Set up the params
   MaterialParameters *matParams = mat->getMaterialParameters();

   //Get the deferred render target
   NamedTexTarget* deferredTexTarget = NamedTexTarget::find("deferred");

   GFXTextureObject *deferredTexObject = deferredTexTarget->getTexture();
   if (!deferredTexObject) 
      return;

   GFX->setTexture(0, deferredTexObject);

   //Set the cubemap
   GFX->setCubeTexture(1, mCubemap->mCubemap);

   //Set the invViewMat
   MatrixSet &matrixSet = renderState->getRenderPass()->getMatrixSet();
   const MatrixF &worldToCameraXfm = matrixSet.getWorldToCamera();

   MaterialParameterHandle *invViewMat = mat->getMaterialParameterHandle("$invViewMat");

   matParams->setSafe(invViewMat, worldToCameraXfm);
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
      F32 nearPlane = 0.01f;
      F32 farDist = 1000.f;

      if (mReflectionModeType == SkyLight)
      {
         nearPlane = 1000.f;
         farDist = 10000.f;
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