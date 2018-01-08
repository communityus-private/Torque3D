#pragma once
#include "scene/sceneRenderState.h"
#include "T3D/systems/componentSystem.h"
#include "ts/tsShape.h"
#include "ts/tsShapeInstance.h"
#include "T3D/assets/ShapeAsset.h"
#include "T3D/assets/MaterialAsset.h"

class MeshComponentInterface
{
public:
   TSShape*		            mShape;
   TSShapeInstance*        mShapeInstance;

   StringTableEntry        mMeshAssetId;
   AssetPtr<ShapeAsset>    mMeshAsset;

   MatrixF                 mTransform;
   Point3F                 mScale;
   Box3F						   mBounds;
   SphereF                 mSphere;

   bool                    mIsClient;

   struct matMap
   {
      //MaterialAsset* matAsset;
      String assetId;
      U32 slot;
   };

   Vector<matMap>  mChangingMaterials;
   Vector<matMap>  mMaterials;

   MeshComponentInterface()
   {
      mShape = nullptr;
      mShapeInstance = nullptr;

      mMeshAssetId = StringTable->EmptyString();
      mMeshAsset = StringTable->EmptyString();
   }

   ~MeshComponentInterface()
   {
      SAFE_DELETE(mShape);
      SAFE_DELETE(mShapeInstance);
   }
};

class MeshRenderSystem : public ComponentSystem < MeshComponentInterface >
{
   //static Vector<MeshComponentInterface> smInterfaceList;

public:
   /*virtual void prepRenderImage(SceneRenderState *state);

   bool setMeshAsset(const char* assetName);

   virtual TSShape* getShape() { if (mMeshAsset)  return mMeshAsset->getShape(); else return NULL; }
   virtual TSShapeInstance* getShapeInstance() { return mShapeInstance; }

   Resource<TSShape> getShapeResource() { return mMeshAsset->getShapeResource(); }

   void _onResourceChanged(const Torque::Path &path);

   virtual bool castRayRendered(const Point3F &start, const Point3F &end, RayInfo *info);

   void mountObjectToNode(SceneObject* objB, String node, MatrixF txfm);

   virtual void onDynamicModified(const char* slotName, const char* newValue);

   void changeMaterial(U32 slot, MaterialAsset* newMat);
   bool setMatInstField(U32 slot, const char* field, const char* value);

   virtual void onInspect();
   virtual void onEndInspect();

   virtual Vector<MatrixF> getNodeTransforms()
   {
      Vector<MatrixF> bob;
      return bob;
   }

   virtual void setNodeTransforms(Vector<MatrixF> transforms)
   {
      return;
   }*/

   /*MeshRenderSystem()
   {

   }
   virtual ~MeshRenderSystem()
   {
      smInterfaceList.clear();
   }

   static MeshComponentInterface* GetNewInterface()
   {
      smInterfaceList.increment();

      return &smInterfaceList.last();
   }

   static void RemoveInterface(T* q)
   {
      smInterfaceList.erase(q);
   }*/

   //
   //
   //Core render function, which does all the real work
   static void render(SceneManager *sceneManager, SceneRenderState* state);

   //Render our particular interface's data
   static void renderInterface(U32 interfaceIndex, SceneRenderState* state);
};
