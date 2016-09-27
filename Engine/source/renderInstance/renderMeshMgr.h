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
#ifndef _RENDERMESHMGR_H_
#define _RENDERMESHMGR_H_

#ifndef _RENDERBINMANAGER_H_
#include "renderInstance/renderBinManager.h"
#endif
#ifndef _OPTIMIZEDPOLYLIST_H_
#include "collision/optimizedPolyList.h"
#endif
#ifndef _GFXVERTEXBUFFER_H_
#include "gfx/gfxVertexBuffer.h"
#endif
#ifndef _GFXPRIMITIVEBUFFER_H_
#include "gfx/gfxPrimitiveBuffer.h"
#endif

//**************************************************************************
// RenderMeshMgr
//**************************************************************************
class RenderMeshMgr : public RenderBinManager
{
   typedef RenderBinManager Parent;
public:
   RenderMeshMgr();
   RenderMeshMgr(RenderInstType riType, F32 renderOrder, F32 processAddOrder);   

   // RenderBinManager interface
   virtual void init();
   virtual void render(SceneRenderState * state);
   virtual void addElement( RenderInst *inst );

   void addStaticBatchElement(SimObject* owner, OptimizedPolyList* geometry, String batchName);
   void addDynamicBatchElement(SimObject* owner, OptimizedPolyList* geometry, String batchName);

   virtual void clear();

   //We retain the pushed geometry data for rendering here. It's static(unless forced to change through editing or whatnot)
   //so rendering the batches is real fast
   struct BufferMaterials
   {
      // The name of the Material we will use for rendering
      String            mMaterialName;
      // The actual Material instance
      BaseMatInstance*  mMaterialInst;

      BufferMaterials()
      {
         mMaterialName = "";
         mMaterialInst = NULL;
      }
   };

   Vector<BufferMaterials> mBufferMaterials;

   struct BufferSet
   {
      U32 surfaceMaterialId;

      U32 vertCount;
      U32 primCount;

      Point3F center;

      struct Buffers
      {
         U32 vertStart;
         U32 primStart;
         U32 vertCount;
         U32 primCount;

         Vector<GFXVertexPNTT> vertData;
         Vector<U32> primData;

         GFXVertexBufferHandle< GFXVertexPNTT > vertexBuffer;
         GFXPrimitiveBufferHandle            primitiveBuffer;

         Buffers()
         {
            vertStart = 0;
            primStart = 0;
            vertCount = 0;
            primCount = 0;

            vertexBuffer = NULL;
            primitiveBuffer = NULL;
         }
      };

      Vector<Buffers> buffers;

      BufferSet()
      {
         Buffers newBuffer;
         buffers.push_back(newBuffer);

         surfaceMaterialId = 0;

         vertCount = 0;
         primCount = 0;

         center = Point3F::Zero;
      }
   };

   Vector<BufferSet> mStaticBuffers;

   Vector<BufferSet> mDynamicBuffers;

   //Batching
   struct MeshBatchElement
   {
      SimObject* owner;
      OptimizedPolyList geometry;
      String batchName;
   };

   Vector<MeshBatchElement> mDynamicBatchElements;

   Vector<MeshBatchElement> mStaticBatchElements;

   void rebuildStaticBuffers();
   void buildDynamicBuffers();

   void prepareBufferRenderInsts(SceneRenderState *state);

   U32 findBufferSetByMaterial(U32 matId)
   {
      for (U32 i = 0; i < mStaticBuffers.size(); i++)
      {
         if (mStaticBuffers[i].surfaceMaterialId == matId)
            return i;
      }

      return -1;
   }

   // ConsoleObject interface
   static void initPersistFields();
   DECLARE_CONOBJECT(RenderMeshMgr);
protected:
   GFXStateBlockRef mNormalSB;
   GFXStateBlockRef mReflectSB;

   void construct();
};

#endif // _RENDERMESHMGR_H_
