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
#include "T3D/ReflectionProbe.h"

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

extern bool gEditingMission;

/*#include "platform/platform.h"
#include "lighting/lightInfo.h"

#include "math/mMath.h"
#include "core/color.h"
#include "gfx/gfxCubemap.h"
#include "console/simObject.h"
#include "math/mathUtils.h"*/


ReflectProbeInfo::ReflectProbeInfo()
   : mTransform(true),
   mColor(0.0f, 0.0f, 0.0f, 1.0f),
   mBrightness(1.0f),
   mAmbient(0.0f, 0.0f, 0.0f, 1.0f),
   mRange(1.0f, 1.0f, 1.0f),
   mInnerConeAngle(90.0f),
   mOuterConeAngle(90.0f),
   mPriority(1.0f),
   mScore(0.0f),
   mDebugRender(false),
   mCubemap(NULL),
   mRadius(1.0f),
   mOverrideColor(false),
   mGroundColor(ColorI::WHITE),
   mSkyColor(ColorI::WHITE),
   mIntensity(1.0f)
{
}

ReflectProbeInfo::~ReflectProbeInfo()
{
}

void ReflectProbeInfo::set(const ReflectProbeInfo *light)
{
   mTransform = light->mTransform;
   mColor = light->mColor;
   mBrightness = light->mBrightness;
   mAmbient = light->mAmbient;
   mRange = light->mRange;
}

void ReflectProbeInfo::getWorldToLightProj(MatrixF *outMatrix) const
{
   *outMatrix = getTransform();
   outMatrix->inverse();
}

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
{
   ReflectionProbe::Sphere, "Sphere", "Sphere shaped"
},
{ ReflectionProbe::Convex, "Convex", "Convex-based shape" }
EndImplementEnumType;

//-----------------------------------------------------------------------------
// Object setup and teardown
//-----------------------------------------------------------------------------
ReflectionProbe::ReflectionProbe()
{
   // Flag this object so that it will always
   // be sent across the network to clients
   mNetFlags.set(Ghostable | ScopeAlways);

   // Set it as a "static" object that casts shadows
   mTypeMask |= StaticObjectType | StaticShapeObjectType | LightObjectType;

   // Make sure we the Material instance to NULL
   // so we don't try to access it incorrectly
   mMaterialInst = NULL;

   mProbeShapeType = Sphere;

   mProbeInfo = new ReflectProbeInfo();

   mRadius = 10;

   mOverrideColor = false;
   mSkyColor = ColorF(0.5f, 0.5f, 1.0f, 1.0f);
   mGroundColor = ColorF(0.8f, 0.7f, 0.5f, 1.0f);
   mIntensity = 0.1f;

   mUseCubemap = false;
   mCubemap = NULL;
}

ReflectionProbe::~ReflectionProbe()
{
   if (mMaterialInst)
      SAFE_DELETE(mMaterialInst);
}

//-----------------------------------------------------------------------------
// Object Editing
//-----------------------------------------------------------------------------
void ReflectionProbe::initPersistFields()
{
   addGroup("Rendering");
   addField("ProbeShape", TypeReflectProbeType, Offset(mProbeShapeType, ReflectionProbe),
      "The type of mesh data to use for collision queries.");
   addField("radius", TypeF32, Offset(mRadius, ReflectionProbe), "The name of the material used to render the mesh.");

   addField("useCubemap", TypeBool, Offset(mUseCubemap, ReflectionProbe), "Cubemap used instead of reflection texture if fullReflect is off.");
   addField("cubemap", TypeCubemapName, Offset(mCubemapName, ReflectionProbe), "Cubemap used instead of reflection texture if fullReflect is off.");

   addField("OverrideColor", TypeBool, Offset(mOverrideColor, ReflectionProbe), "Path of file to save and load results.");
   addField("SkyColor", TypeColorF, Offset(mSkyColor, ReflectionProbe), "Path of file to save and load results.");
   addField("GroundColor", TypeColorF, Offset(mGroundColor, ReflectionProbe), "Path of file to save and load results.");
   addField("Intensity", TypeF32, Offset(mIntensity, ReflectionProbe), "Path of file to save and load results.");
   endGroup("Rendering");

   // SceneObject already handles exposing the transform
   Parent::initPersistFields();
}

void ReflectionProbe::inspectPostApply()
{
   Parent::inspectPostApply();

   // Flag the network mask to send the updates
   // to the client object
   setMaskBits(-1);
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

   // Refresh this object's material (if any)
   updateMaterial();

   if (isClientObject())
   {
      if (mCubemapName.isNotEmpty())
         Sim::findObject(mCubemapName, mCubemap);
   }

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

   stream->writeFlag(mOverrideColor);
   stream->write(mSkyColor);
   stream->write(mGroundColor);
   stream->writeFloat(mIntensity, 7);
   stream->write(mRadius);

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

   if (stream->readFlag())  // UpdateMask
   {
      stream->read(&mMaterialName);

      if (isProperlyAdded())
         updateMaterial();
   }

   mOverrideColor = stream->readFlag();
   stream->read(&mSkyColor);
   stream->read(&mGroundColor);
   mIntensity = stream->readFloat(7);
   stream->read(&mRadius);

   mUseCubemap = stream->readFlag();
   stream->read(&mCubemapName);

   if (mCubemapName.isNotEmpty())
      Sim::findObject(mCubemapName, mCubemap);

   updateMaterial();
}

//-----------------------------------------------------------------------------
// Object Rendering
//-----------------------------------------------------------------------------
void ReflectionProbe::createGeometry()
{
   static const Point3F cubePoints[8] =
   {
      Point3F(1, -1, -1), Point3F(1, -1,  1), Point3F(1,  1, -1), Point3F(1,  1,  1),
      Point3F(-1, -1, -1), Point3F(-1,  1, -1), Point3F(-1, -1,  1), Point3F(-1,  1,  1)
   };

   static const Point3F cubeNormals[6] =
   {
      Point3F(1,  0,  0), Point3F(-1,  0,  0), Point3F(0,  1,  0),
      Point3F(0, -1,  0), Point3F(0,  0,  1), Point3F(0,  0, -1)
   };

   static const Point2F cubeTexCoords[4] =
   {
      Point2F(0,  0), Point2F(0, -1),
      Point2F(1,  0), Point2F(1, -1)
   };

   static const U32 cubeFaces[36][3] =
   {
      { 3, 0, 3 },{ 0, 0, 0 },{ 1, 0, 1 },
      { 2, 0, 2 },{ 0, 0, 0 },{ 3, 0, 3 },
      { 7, 1, 1 },{ 4, 1, 2 },{ 5, 1, 0 },
      { 6, 1, 3 },{ 4, 1, 2 },{ 7, 1, 1 },
      { 3, 2, 1 },{ 5, 2, 2 },{ 2, 2, 0 },
      { 7, 2, 3 },{ 5, 2, 2 },{ 3, 2, 1 },
      { 1, 3, 3 },{ 4, 3, 0 },{ 6, 3, 1 },
      { 0, 3, 2 },{ 4, 3, 0 },{ 1, 3, 3 },
      { 3, 4, 3 },{ 6, 4, 0 },{ 7, 4, 1 },
      { 1, 4, 2 },{ 6, 4, 0 },{ 3, 4, 3 },
      { 2, 5, 1 },{ 4, 5, 2 },{ 0, 5, 0 },
      { 5, 5, 3 },{ 4, 5, 2 },{ 2, 5, 1 }
   };

   // Fill the vertex buffer
   VertexType *pVert = NULL;

   mVertexBuffer.set(GFX, 36, GFXBufferTypeStatic);
   pVert = mVertexBuffer.lock();

   Point3F halfSize = getObjBox().getExtents() * 0.5f;

   for (U32 i = 0; i < 36; i++)
   {
      const U32& vdx = cubeFaces[i][0];
      const U32& ndx = cubeFaces[i][1];
      const U32& tdx = cubeFaces[i][2];

      pVert[i].point = cubePoints[vdx] * halfSize;
      pVert[i].normal = cubeNormals[ndx];
      pVert[i].texCoord = cubeTexCoords[tdx];
   }

   mVertexBuffer.unlock();

   // Fill the primitive buffer
   U16 *pIdx = NULL;

   mPrimitiveBuffer.set(GFX, 36, 12, GFXBufferTypeStatic);

   mPrimitiveBuffer.lock(&pIdx);

   for (U16 i = 0; i < 36; i++)
      pIdx[i] = i;

   mPrimitiveBuffer.unlock();
}

void ReflectionProbe::updateMaterial()
{
   /*if (mMaterialName.isEmpty())
      return;

   // If the material name matches then don't bother updating it.
   if (mMaterialInst && mMaterialName.equal(mMaterialInst->getMaterial()->getName(), String::NoCase))
      return;

   SAFE_DELETE(mMaterialInst);

   mMaterialInst = MATMGR->createMatInstance(mMaterialName, getGFXVertexFormat< VertexType >());
   if (!mMaterialInst)
      Con::errorf("ReflectionProbe::updateMaterial - no Material called '%s'", mMaterialName.c_str());*/

   /*SAFE_DELETE(mMaterialInst);

   Material* reflMat;
   if (!Sim::findObject("ReflectionProbeMaterial", reflMat))
      return;

   mMaterialInst = reflMat->createMatInstance();*/

   mProbeInfo->setPosition(getPosition());

   mProbeInfo->mCubemap = mCubemap;
   mProbeInfo->mIntensity = mIntensity;
   mProbeInfo->mUseCubemap = mUseCubemap;
   mProbeInfo->mSkyColor = mSkyColor;
   mProbeInfo->mGroundColor = mGroundColor;
   mProbeInfo->mRadius = mRadius;
}

void ReflectionProbe::prepRenderImage(SceneRenderState *state)
{
   // Do a little prep work if needed
   //if (mVertexBuffer.isNull())
   //   createGeometry();

   // If we have no material then skip out.
   //if (!mMaterialInst || !state)
  //    return;

   // If we don't have a material instance after the override then 
   // we can skip rendering all together.
   //BaseMatInstance *matInst = state->getOverrideMaterial(mMaterialInst);
   //if (!matInst)
   //   return;

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

   // Get a handy pointer to our RenderPassmanager
   /*RenderPassManager *renderPass = state->getRenderPass();

   // Allocate an MeshRenderInst so that we can submit it to the RenderPassManager
   ProbeRenderInst *ri = renderPass->allocInst<ProbeRenderInst>();

   // Set our RenderInst as a standard mesh render
   ri->type = RenderPassManager::RIT_ReflectProbe;

   //If our material has transparency set on this will redirect it to proper render bin
   if (matInst->getMaterial()->isTranslucent())
   {
      ri->type = RenderPassManager::RIT_Translucent;
      ri->translucentSort = true;
   }

   // Calculate our sorting point
   if (state)
   {
      // Calculate our sort point manually.
      const Box3F& rBox = getRenderWorldBox();
      ri->sortDistSq = rBox.getSqDistanceToPoint(state->getCameraPosition());
   }
   else
      ri->sortDistSq = 0.0f;

   // Set up our transforms
   MatrixF objectToWorld = getRenderTransform();
   objectToWorld.scale(getScale());

   ri->objectToWorld = renderPass->allocUniqueXform(objectToWorld);
   ri->worldToCamera = renderPass->allocSharedXform(RenderPassManager::View);
   ri->projection = renderPass->allocSharedXform(RenderPassManager::Projection);

   // If our material needs lights then fill the RIs 
   // light vector with the best lights.
   if (matInst->isForwardLit())
   {
      LightQuery query;
      query.init(getWorldSphere());
      query.getLights(ri->lights, 8);
   }

   // Make sure we have an up-to-date backbuffer in case
   // our Material would like to make use of it
   // NOTICE: SFXBB is removed and refraction is disabled!
   //ri->backBuffTex = GFX->getSfxBackBuffer();

   // Set our Material
   ri->matInst = matInst;

   // Set up our vertex buffer and primitive buffer
   ri->vertBuff = &mVertexBuffer;
   ri->primBuff = &mPrimitiveBuffer;

   ri->prim = renderPass->allocPrim();
   ri->prim->type = GFXTriangleList;
   ri->prim->minIndex = 0;
   ri->prim->startIndex = 0;
   ri->prim->numPrimitives = 12;
   ri->prim->startVertex = 0;
   ri->prim->numVertices = 36;

   // We sort by the material then vertex buffer
   ri->defaultKey = matInst->getStateHint();
   ri->defaultKey2 = (uintptr_t)ri->vertBuff; // Not 64bit safe!

                                              // Submit our RenderInst to the RenderPassManager
   state->getRenderPass()->addInst(ri);*/
}

void ReflectionProbe::submitLights(LightManager *lm, bool staticLighting)
{
   //if (!mIsEnabled)
   //   return;

   lm->addSphereReflectProbe(mProbeInfo);
}

void ReflectionProbe::_onRenderViz(ObjectRenderInst *ri,
   SceneRenderState *state,
   BaseMatInstance *overrideMat)
{
   if (overrideMat)
      return;

   GFXDrawUtil *draw = GFX->getDrawUtil();

   GFXStateBlockDesc desc;
   desc.setZReadWrite(true, false);
   desc.setCullMode(GFXCullNone);
   desc.setBlend(true);

   // Base the sphere color on the light color.
   ColorI color = ColorI::WHITE;
   color.alpha = 50;

   draw->drawSphere(desc, mRadius, getPosition(), color);
}

DefineEngineMethod(ReflectionProbe, postApply, void, (), ,
   "A utility method for forcing a network update.\n")
{
   object->inspectPostApply();
}