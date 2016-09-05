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
#include "T3D/examples/staticBatchTester.h"

#include "math/mathIO.h"
#include "scene/sceneRenderState.h"
#include "console/consoleTypes.h"
#include "core/stream/bitStream.h"
#include "materials/materialManager.h"
#include "materials/baseMatInstance.h"
#include "renderInstance/renderPassManager.h"
#include "lighting/lightQuery.h"
#include "console/engineAPI.h"


IMPLEMENT_CO_NETOBJECT_V1(StaticBatchTester);

ConsoleDocClass( StaticBatchTester, 
   "@brief An example scene object which renders a mesh.\n\n"
   "This class implements a basic SceneObject that can exist in the world at a "
   "3D position and render itself. There are several valid ways to render an "
   "object in Torque. This class implements the preferred rendering method which "
   "is to submit a MeshRenderInst along with a Material, vertex buffer, "
   "primitive buffer, and transform and allow the RenderMeshMgr handle the "
   "actual setup and rendering for you.\n\n"
   "See the C++ code for implementation details.\n\n"
   "@ingroup Examples\n" );


//-----------------------------------------------------------------------------
// Object setup and teardown
//-----------------------------------------------------------------------------
StaticBatchTester::StaticBatchTester()
{
   // Flag this object so that it will always
   // be sent across the network to clients
   mNetFlags.set( Ghostable | ScopeAlways );

   // Set it as a "static" object that casts shadows
   mTypeMask |= StaticObjectType | StaticShapeObjectType;

   // Make sure we the Material instance to NULL
   // so we don't try to access it incorrectly
   mMaterialInst = NULL;
}

StaticBatchTester::~StaticBatchTester()
{
   if ( mMaterialInst )
      SAFE_DELETE( mMaterialInst );
}

//-----------------------------------------------------------------------------
// Object Editing
//-----------------------------------------------------------------------------
void StaticBatchTester::initPersistFields()
{
   addGroup( "Rendering" );
   addField( "material",      TypeMaterialName, Offset( mMaterialName, StaticBatchTester ),
      "The name of the material used to render the mesh." );
   endGroup( "Rendering" );

   // SceneObject already handles exposing the transform
   Parent::initPersistFields();
}

void StaticBatchTester::inspectPostApply()
{
   Parent::inspectPostApply();

   // Flag the network mask to send the updates
   // to the client object
   setMaskBits( UpdateMask );
}

bool StaticBatchTester::onAdd()
{
   if ( !Parent::onAdd() )
      return false;

   // Set up a 1x1x1 bounding box
   mObjBox.set( Point3F( -0.5f, -0.5f, -0.5f ),
                Point3F(  0.5f,  0.5f,  0.5f ) );

   resetWorldBox();

   // Add this object to the scene
   addToScene();

   // Refresh this object's material (if any)
   updateMaterial();

   return true;
}

void StaticBatchTester::onRemove()
{
   // Remove this object from the scene
   removeFromScene();

   Parent::onRemove();
}

void StaticBatchTester::setTransform(const MatrixF & mat)
{
   // Let SceneObject handle all of the matrix manipulation
   Parent::setTransform( mat );

   // Dirty our network mask so that the new transform gets
   // transmitted to the client object
   setMaskBits( TransformMask );
}

U32 StaticBatchTester::packUpdate( NetConnection *conn, U32 mask, BitStream *stream )
{
   // Allow the Parent to get a crack at writing its info
   U32 retMask = Parent::packUpdate( conn, mask, stream );

   // Write our transform information
   if ( stream->writeFlag( mask & TransformMask ) )
   {
      mathWrite(*stream, getTransform());
      mathWrite(*stream, getScale());
   }

   // Write out any of the updated editable properties
   if ( stream->writeFlag( mask & UpdateMask ) )
      stream->write( mMaterialName );

   return retMask;
}

void StaticBatchTester::unpackUpdate(NetConnection *conn, BitStream *stream)
{
   // Let the Parent read any info it sent
   Parent::unpackUpdate(conn, stream);

   if ( stream->readFlag() )  // TransformMask
   {
      mathRead(*stream, &mObjToWorld);
      mathRead(*stream, &mObjScale);

      setTransform( mObjToWorld );
   }

   if ( stream->readFlag() )  // UpdateMask
   {
      stream->read( &mMaterialName );

      if ( isProperlyAdded() )
         updateMaterial();
   }
}

//-----------------------------------------------------------------------------
// Object Rendering
//-----------------------------------------------------------------------------
void StaticBatchTester::createGeometry()
{
   static const Point3F cubePoints[8] = 
   {
      Point3F( 1, -1, -1), Point3F( 1, -1,  1), Point3F( 1,  1, -1), Point3F( 1,  1,  1),
      Point3F(-1, -1, -1), Point3F(-1,  1, -1), Point3F(-1, -1,  1), Point3F(-1,  1,  1)
   };

   static const Point3F cubeNormals[6] = 
   {
      Point3F( 1,  0,  0), Point3F(-1,  0,  0), Point3F( 0,  1,  0),
      Point3F( 0, -1,  0), Point3F( 0,  0,  1), Point3F( 0,  0, -1)
   };

   static const Point2F cubeTexCoords[4] = 
   {
      Point2F( 0,  0), Point2F( 0, -1),
      Point2F( 1,  0), Point2F( 1, -1)
   };

   static const U32 cubeFaces[36][3] = 
   {
      { 3, 0, 3 }, { 0, 0, 0 }, { 1, 0, 1 },
      { 2, 0, 2 }, { 0, 0, 0 }, { 3, 0, 3 },
      { 7, 1, 1 }, { 4, 1, 2 }, { 5, 1, 0 },
      { 6, 1, 3 }, { 4, 1, 2 }, { 7, 1, 1 },
      { 3, 2, 1 }, { 5, 2, 2 }, { 2, 2, 0 },
      { 7, 2, 3 }, { 5, 2, 2 }, { 3, 2, 1 },
      { 1, 3, 3 }, { 4, 3, 0 }, { 6, 3, 1 },
      { 0, 3, 2 }, { 4, 3, 0 }, { 1, 3, 3 },
      { 3, 4, 3 }, { 6, 4, 0 }, { 7, 4, 1 },
      { 1, 4, 2 }, { 6, 4, 0 }, { 3, 4, 3 },
      { 2, 5, 1 }, { 4, 5, 2 }, { 0, 5, 0 },
      { 5, 5, 3 }, { 4, 5, 2 }, { 2, 5, 1 }
   };

   // Fill the vertex buffer
   VertexType *pVert = NULL;

   mVertexBuffer.set( GFX, 36, GFXBufferTypeStatic );
   pVert = mVertexBuffer.lock();

   Point3F halfSize = getObjBox().getExtents() * 0.5f;

   for (U32 i = 0; i < 36; i++)
   {
      const U32& vdx = cubeFaces[i][0];
      const U32& ndx = cubeFaces[i][1];
      const U32& tdx = cubeFaces[i][2];

      pVert[i].point    = cubePoints[vdx] * halfSize;
      pVert[i].normal   = cubeNormals[ndx];
      pVert[i].texCoord = cubeTexCoords[tdx];
   }

   mVertexBuffer.unlock();

   // Fill the primitive buffer
   U16 *pIdx = NULL;

   mPrimitiveBuffer.set( GFX, 36, 12, GFXBufferTypeStatic );

   mPrimitiveBuffer.lock(&pIdx);     
   
   for (U16 i = 0; i < 36; i++)
      pIdx[i] = i;

   mPrimitiveBuffer.unlock();
}

void StaticBatchTester::updateMaterial()
{
   if ( mMaterialName.isEmpty() )
      return;

   // If the material name matches then don't bother updating it.
   if ( mMaterialInst && mMaterialName.equal( mMaterialInst->getMaterial()->getName(), String::NoCase ) )
      return;

   SAFE_DELETE( mMaterialInst );

   mMaterialInst = MATMGR->createMatInstance( mMaterialName, getGFXVertexFormat< VertexType >() );
   if ( !mMaterialInst )
      Con::errorf( "StaticBatchTester::updateMaterial - no Material called '%s'", mMaterialName.c_str() );
}

void StaticBatchTester::prepRenderImage( SceneRenderState *state )
{
   // Do a little prep work if needed
   //if ( mVertexBuffer.isNull() )
   //   createGeometry();

   // If we have no material then skip out.
   if ( /*!mMaterialInst ||*/ !state)
      return;

   // If we don't have a material instance after the override then 
   // we can skip rendering all together.
   //BaseMatInstance *matInst = state->getOverrideMaterial( mMaterialInst );
   //if ( !matInst )
   //   return;

   BaseMatInstance *matInst = MATMGR->getWarningMatInstance();

   // Get a handy pointer to our RenderPassmanager
   RenderPassManager *renderPass = state->getRenderPass();

   for (U32 i = 0; i < mStaticBuffers.size(); i++)
   {
      for (U32 b = 0; b < mStaticBuffers[i].buffers.size(); b++)
      {
         if (mStaticBuffers[i].buffers[b].vertData.empty())
            continue;

         MeshRenderInst *ri = renderPass->allocInst<MeshRenderInst>();

         // Set our RenderInst as a standard mesh render
         ri->type = RenderPassManager::RIT_Mesh;

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
         ri->vertBuff = &mStaticBuffers[i].buffers[b].vertexBuffer;
         ri->primBuff = &mStaticBuffers[i].buffers[b].primitiveBuffer;

         ri->prim = renderPass->allocPrim();
         ri->prim->type = GFXTriangleList;
         ri->prim->minIndex = 0;
         ri->prim->startIndex = 0;
         ri->prim->numPrimitives = mStaticBuffers[i].buffers[b].primData.size();
         ri->prim->startVertex = 0;
         ri->prim->numVertices = mStaticBuffers[i].buffers[b].vertData.size();

         // We sort by the material then vertex buffer
         ri->defaultKey = matInst->getStateHint();
         ri->defaultKey2 = (uintptr_t)ri->vertBuff; // Not 64bit safe!

         // Submit our RenderInst to the RenderPassManager
         state->getRenderPass()->addInst(ri);
      }
   }
}

DefineEngineMethod( StaticBatchTester, postApply, void, (),,
   "A utility method for forcing a network update.\n")
{
	object->inspectPostApply();
}

void StaticBatchTester::addStaticElement(SimObject* owner, OptimizedPolyList* geometry, String batchName)
{
   Vector<U32> tempIndices;
   tempIndices.reserve(4);

   bool found = false;
   for (U32 i = 0; i < mStaticElements.size(); ++i)
   {
      if (mStaticElements[i].owner != NULL && mStaticElements[i].owner->getId() == owner->getId())
      {
         found = true;
         mStaticElements[i].geometry = *geometry;
         break;
      }
   }

   if (!found)
   {
      StaticBatchElement newElement;

      newElement.owner = owner;
      newElement.geometry = *geometry;

      mStaticElements.push_back(newElement);
   }

   rebuildBuffers(); 
}

void StaticBatchTester::rebuildBuffers()
{
   U32 BUFFER_SIZE = 65000;
   Vector<U32> tempIndices;
   tempIndices.reserve(4);

   Box3F newBounds = Box3F::Zero;

   mStaticBuffers.clear();

   for (U32 i = 0; i < mStaticElements.size(); ++i)
   {
      for (U32 j = 0; j < mStaticElements[i].geometry.mPolyList.size(); j++)
      {
         const OptimizedPolyList::Poly& poly = mStaticElements[i].geometry.mPolyList[j];

         if (poly.vertexCount < 3)
            continue;

         tempIndices.setSize(poly.vertexCount);
         dMemset(tempIndices.address(), 0, poly.vertexCount);

         if (poly.type == OptimizedPolyList::TriangleStrip ||
            poly.type == OptimizedPolyList::TriangleFan)
         {
            tempIndices[0] = 0;
            U32 idx = 1;

            for (U32 k = 1; k < poly.vertexCount; k += 2)
               tempIndices[idx++] = k;

            for (U32 k = ((poly.vertexCount - 1) & (~0x1)); k > 0; k -= 2)
               tempIndices[idx++] = k;
         }
         else if (poly.type == OptimizedPolyList::TriangleList)
         {
            for (U32 k = 0; k < poly.vertexCount; k++)
               tempIndices[k] = k;
         }
         else
            continue;

         //got our data, now insert it into the correct buffer!
         S32 bufferId = findBufferSetByMaterial(poly.material);

         if (bufferId == -1)
         {
            //add a new buffer set if we didn't get a match!
            BufferSet newSet;
            newSet.surfaceMaterialId = poly.material;

            mStaticBuffers.push_back(newSet);

            bufferId = mStaticBuffers.size() - 1;
         }

         //see if this would push us over our buffer size limit, if it is, make a new buffer for this set
         if (mStaticBuffers[bufferId].buffers.last().vertData.size() + 3 > BUFFER_SIZE
            || mStaticBuffers[bufferId].buffers.last().primData.size() + 1 > BUFFER_SIZE)
         {
            //yep, we'll overstep with this, so spool up a new buffer in this set
            BufferSet::Buffers newBuffer = BufferSet::Buffers();
            mStaticBuffers[bufferId].buffers.push_back(newBuffer);
         }

         const U32& firstIdx = mStaticElements[i].geometry.mIndexList[poly.vertexStart];
         const OptimizedPolyList::VertIndex& firstVertIdx = mStaticElements[i].geometry.mVertexList[firstIdx];

         Vector<Point3F> geomPoints = mStaticElements[i].geometry.mPoints;
         Vector<Point3F> geomNormals = mStaticElements[i].geometry.mNormals;
         Vector<Point2F> geoUVs = mStaticElements[i].geometry.mUV0s;

         for (U32 k = 1; k < poly.vertexCount - 1; k++)
         {
            const U32& secondIdx = mStaticElements[i].geometry.mIndexList[poly.vertexStart + tempIndices[k]];
            const U32& thirdIdx = mStaticElements[i].geometry.mIndexList[poly.vertexStart + tempIndices[k + 1]];

            const OptimizedPolyList::VertIndex& secondVertIdx = mStaticElements[i].geometry.mVertexList[secondIdx];
            const OptimizedPolyList::VertIndex& thirdVertIdx = mStaticElements[i].geometry.mVertexList[thirdIdx];

            Point3F points[3];
            points[0] = mStaticElements[i].geometry.mPoints[firstVertIdx.vertIdx];
            points[1] = mStaticElements[i].geometry.mPoints[secondVertIdx.vertIdx];
            points[2] = mStaticElements[i].geometry.mPoints[thirdVertIdx.vertIdx];

            Point3F normals[3];
            normals[0] = mStaticElements[i].geometry.mNormals[firstVertIdx.normalIdx];
            normals[1] = mStaticElements[i].geometry.mNormals[secondVertIdx.normalIdx];
            normals[2] = mStaticElements[i].geometry.mNormals[thirdVertIdx.normalIdx];

            Point3F tangents[3];
            tangents[0] = mCross(points[1] - points[0], normals[0]);
            tangents[1] = mCross(points[2] - points[1], normals[1]);
            tangents[2] = mCross(points[0] - points[2], normals[2]);

            Point2F uvs[3];
            uvs[0] = mStaticElements[i].geometry.mUV0s[firstVertIdx.uv0Idx];
            uvs[1] = mStaticElements[i].geometry.mUV0s[secondVertIdx.uv0Idx];
            uvs[2] = mStaticElements[i].geometry.mUV0s[thirdVertIdx.uv0Idx];

            mStaticBuffers[bufferId].vertCount += 3;
            mStaticBuffers[bufferId].primCount += 1;

            for (U32 v = 0; v < 3; ++v)
            {
               //Build the vert and store it to the buffers!
               GFXVertexPNTT bufVert;
               bufVert.point = points[v];
               bufVert.normal = normals[v];
               bufVert.tangent = tangents[v];
               bufVert.texCoord = uvs[v];

               newBounds.extend(points[v]);

               mStaticBuffers[bufferId].buffers.last().vertData.push_back(bufVert);

               U32 vertPrimId = mStaticBuffers[bufferId].buffers.last().vertData.size() - 1;
               mStaticBuffers[bufferId].buffers.last().primData.push_back(vertPrimId);

               mStaticBuffers[bufferId].center += points[v];
            }
         }
      }
   }

   //Now, iterate through the organized data and turn them into renderable buffers
   for (U32 i = 0; i < mStaticBuffers.size(); i++)
   {
      for (U32 b = 0; b < mStaticBuffers[i].buffers.size(); b++)
      {
         BufferSet::Buffers& buffers = mStaticBuffers[i].buffers[b];

         //if there's no data to be had in this buffer, just skip it
         if (buffers.vertData.empty())
            continue;

         buffers.vertexBuffer.set(GFX, buffers.vertData.size(), GFXBufferTypeStatic);
         GFXVertexPNTT *pVert = buffers.vertexBuffer.lock();

         for (U32 v = 0; v < buffers.vertData.size(); v++)
         {
            pVert->normal = buffers.vertData[v].normal;
            pVert->tangent = buffers.vertData[v].tangent;
            //pVert->color = buffers.vertData[v].color;
            pVert->point = buffers.vertData[v].point;
            pVert->texCoord = buffers.vertData[v].texCoord;

            pVert++;
         }

         buffers.vertexBuffer.unlock();

         // Allocate PB
         buffers.primitiveBuffer.set(GFX, buffers.primData.size(), buffers.primData.size() / 3, GFXBufferTypeStatic);

         U16 *pIndex;
         buffers.primitiveBuffer.lock(&pIndex);

         for (U16 i = 0; i < buffers.primData.size(); i++)
         {
            *pIndex = i;
            pIndex++;
         }

         buffers.primitiveBuffer.unlock();
      }

      mStaticBuffers[i].center /= mStaticBuffers[i].vertCount;
   }

   mObjBox = newBounds;
   resetWorldBox();
}