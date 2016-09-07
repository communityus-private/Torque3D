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

#include "console/consoleTypes.h"
#include "T3D/components/game/levelCellManager.h"
#include "core/util/safeDelete.h"
#include "console/consoleTypes.h"
#include "console/consoleObject.h"
#include "core/stream/bitStream.h"
#include "console/engineAPI.h"
#include "sim/netConnection.h"
#include "T3D/gameBase/gameConnection.h"
#include "T3D/components/coreInterfaces.h"
#include "math/mathUtils.h"
#include "collision/concretePolyList.h"
#include "collision/clippedPolyList.h"

#include "gfx/sim/debugDraw.h"

IMPLEMENT_CALLBACK( LevelCellManager, onEnterViewCmd, void, 
   ( Entity* cameraEnt, bool firstTimeSeeing ), ( cameraEnt, firstTimeSeeing ),
   "@brief Called when an object enters the volume of the Trigger instance using this TriggerData.\n\n"

   "@param trigger the Trigger instance whose volume the object entered\n"
   "@param obj the object that entered the volume of the Trigger instance\n" );

IMPLEMENT_CALLBACK( LevelCellManager, onExitViewCmd, void, 
   ( Entity* cameraEnt ), ( cameraEnt ),
   "@brief Called when an object enters the volume of the Trigger instance using this TriggerData.\n\n"

   "@param trigger the Trigger instance whose volume the object entered\n"
   "@param obj the object that entered the volume of the Trigger instance\n" );

IMPLEMENT_CALLBACK( LevelCellManager, onUpdateInViewCmd, void, 
   ( Entity* cameraEnt ), ( cameraEnt ),
   "@brief Called when an object enters the volume of the Trigger instance using this TriggerData.\n\n"

   "@param trigger the Trigger instance whose volume the object entered\n"
   "@param obj the object that entered the volume of the Trigger instance\n" );

IMPLEMENT_CALLBACK( LevelCellManager, onUpdateOutOfViewCmd, void, 
   ( Entity* cameraEnt ), ( cameraEnt ),
   "@brief Called when an object enters the volume of the Trigger instance using this TriggerData.\n\n"

   "@param trigger the Trigger instance whose volume the object entered\n"
   "@param obj the object that entered the volume of the Trigger instance\n" );

//////////////////////////////////////////////////////////////////////////
// Constructor/Destructor
//////////////////////////////////////////////////////////////////////////

LevelCellManager::LevelCellManager() : Component()
{
   mObjectList.clear();

   mVisible = false;

   mFriendlyName = "Trigger";
   mComponentType = "Trigger";

   mDescription = getDescriptionText("Calls trigger events when a client starts and stops seeing it. Also ticks while visible to clients.");

   mCellWidth = 50;
}

LevelCellManager::~LevelCellManager()
{
   for(S32 i = 0;i < mFields.size();++i)
   {
      ComponentField &field = mFields[i];
      SAFE_DELETE_ARRAY(field.mFieldDescription);
   }

   SAFE_DELETE_ARRAY(mDescription);
}

IMPLEMENT_CO_NETOBJECT_V1(LevelCellManager);


bool LevelCellManager::onAdd()
{
   if(! Parent::onAdd())
      return false;

   return true;
}

void LevelCellManager::onRemove()
{
   Parent::onRemove();
}

//This is mostly a catch for situations where the behavior is re-added to the object and the like and we may need to force an update to the behavior
void LevelCellManager::onComponentAdd()
{
   Parent::onComponentAdd();

   CollisionInterface *colInt = mOwner->getComponent<CollisionInterface>();

   if(colInt)
   {
      colInt->onCollisionSignal.notify(this, &LevelCellManager::potentialEnterObject);
   }
}

void LevelCellManager::onComponentRemove()
{
   CollisionInterface *colInt = mOwner->getComponent<CollisionInterface>();

   if(colInt)
   {
      colInt->onCollisionSignal.remove(this, &LevelCellManager::potentialEnterObject);
   }

   Parent::onComponentRemove();
}

void LevelCellManager::componentAddedToOwner(Component *comp)
{
   if (comp->getId() == getId())
      return;

   CollisionInterface *colInt = mOwner->getComponent<CollisionInterface>();

   if (colInt)
   {
      colInt->onCollisionSignal.notify(this, &LevelCellManager::potentialEnterObject);
   }
}

void LevelCellManager::componentRemovedFromOwner(Component *comp)
{
   if (comp->getId() == getId()) //?????????
      return;

   CollisionInterface *colInt = mOwner->getComponent<CollisionInterface>();

   if (colInt)
   {
      colInt->onCollisionSignal.remove(this, &LevelCellManager::potentialEnterObject);
   }
}

void LevelCellManager::initPersistFields()
{
   Parent::initPersistFields();

   addField("visibile",   TypeBool,  Offset( mVisible, LevelCellManager ), "" );

   addField("onEnterViewCmd", TypeCommand, Offset(mEnterCommand, LevelCellManager), "");
   addField("onExitViewCmd", TypeCommand, Offset(mOnExitCommand, LevelCellManager), "");
   addField("onUpdateInViewCmd", TypeCommand, Offset(mOnUpdateInViewCmd, LevelCellManager), "");
}

U32 LevelCellManager::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);
   return retMask;
}

void LevelCellManager::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);
}

void LevelCellManager::potentialEnterObject(SceneObject *collider)
{
   if(testObject(collider))
   {
      bool found = false;
      for(U32 i=0; i < mObjectList.size(); i++)
      {
         if(mObjectList[i]->getId() == collider->getId())
         {
            found = true;
            break;
         }
      }

      if (!found)
      {
         mObjectList.push_back(collider);

         if (!mEnterCommand.isEmpty())
         {
            String command = String("%obj = ") + collider->getIdString() + ";" + 
               String("%this = ") + getIdString() + ";" + mEnterCommand;
            Con::evaluate(command.c_str());
         }

         //onEnterTrigger_callback(this, enter);
      }
   }
}

bool LevelCellManager::testObject(SceneObject* enter)
{
   //First, test to early out
   Box3F enterBox = enter->getWorldBox();

   //if(!mOwner->getWorldBox().intersect(enterBox) || !)
   //   return false;

   //We're still here, so we should do actual work
   //We're going to be 
   ConcretePolyList mClippedList;

   SphereF sphere;
   sphere.center = (mOwner->getWorldBox().minExtents + mOwner->getWorldBox().maxExtents) * 0.5;
   VectorF bv = mOwner->getWorldBox().maxExtents - sphere.center;
   sphere.radius = bv.len();

   Entity* enterEntity = dynamic_cast<Entity*>(enter);
   if(enterEntity)
   {
      //quick early out. If the bounds don't overlap, it cannot be colliding or inside
      if (!mOwner->getWorldBox().isOverlapped(enterBox))
         return false;

      //check if the entity has a collision shape
      CollisionInterface *cI = enterEntity->getComponent<CollisionInterface>();
      if (cI)
      {
         cI->buildPolyList(PLC_Collision, &mClippedList, mOwner->getWorldBox(), sphere);

         if (!mClippedList.isEmpty())
         {
            //well, it's clipped with, or inside, our bounds
            //now to test the clipped list against our own collision mesh
            CollisionInterface *myCI = mOwner->getComponent<CollisionInterface>();

            //wait, how would we NOT have this?
            if (myCI)
            {
               //anywho, build our list and then we'll check intersections
               ClippedPolyList myList;

               MatrixF ownerTransform = mOwner->getTransform();
               myList.setTransform(&ownerTransform, mOwner->getScale());
               myList.setObject(mOwner);

               myCI->buildPolyList(PLC_Collision, &myList, enterBox, sphere);

               bool test = true;
            }
         }
      }
   }

   return mClippedList.isEmpty() == false;
}

void LevelCellManager::processTick()
{
   Parent::processTick();

   if (!isActive())
      return;

   //We run through our list of clients, and get the cells they're in. Then, we update our load/unload of the cells surrounding those
   if (isServerObject())
   {
      U32 clientIdx = 0;
      while (GameConnection* gameConn = getConnection(clientIdx))
      {
         Entity* cameraEntity = dynamic_cast<Entity*>(gameConn->getCameraObject());

         if (cameraEntity)
         {
            /*//find the active cell
            cameraEntity->getPosition();

            Point3F gridStart = mSelectedTileObject->getOwner()->getPosition() + Point3F(-50, -50, 0);

            //draw the grid
            for (U32 i = 0; i < MAX_TILES / 2; i++)
            {
               //X aix
               Point3F xLineStart = Point3F(gridStart.x, gridStart.y + (i * 2), gridStart.z);
               drawer->drawLine(xLineStart, xLineStart + Point3F(100, 0, 0), ColorI(200, 200, 200, 255));

               Point3F yLineStart = Point3F(gridStart.x + (i * 2), gridStart.y, gridStart.z);
               drawer->drawLine(yLineStart, yLineStart + Point3F(0, 100, 0), ColorI(200, 200, 200, 255));
            }

            //see if we have a camera-type component
            CameraInterface *camInterface = cameraEntity->getComponent<CameraInterface>();
            if (camInterface)
            {
               Frustum visFrustum;

               visFrustum = camInterface->getFrustum();

               DebugDrawer *ddraw = DebugDrawer::get();

               const PolyhedronData::Edge* edges = visFrustum.getEdges();
               const Point3F* points = visFrustum.getPoints();
               for (U32 i = 0; i < visFrustum.EdgeCount; i++)
               {
                  Point3F vertA = points[edges[i].vertex[0]];
                  Point3F vertB = points[edges[i].vertex[1]];
                  ddraw->drawLine(vertA, vertB, ColorI(0, 255, 0, 255));
                  ddraw->setLastTTL(renderTimeMS);
               }
            }*/
         }

         ++clientIdx;
      }
   }

	//get our list of active clients, and see if they have cameras, if they do, build a frustum and see if we exist inside that
   mVisible = false;
   if(isServerObject())
   {
      for(U32 i=0; i < mObjectList.size(); i++)
      {
         if(!testObject(mObjectList[i]))
         {
            if (!mOnExitCommand.isEmpty())
            {
               String command = String("%obj = ") + mObjectList[i]->getIdString() + ";" +
                  String("%this = ") + getIdString() + ";" + mOnExitCommand;
               Con::evaluate(command.c_str());
            }

            mObjectList.erase(i);
            //mDataBlock->onLeaveTrigger_callback( this, remove );
            //onLeaveTrigger_callback(this, remove);
         }
      }

      /*if (!mTickCommand.isEmpty())
         Con::evaluate(mTickCommand.c_str());

      if (mObjects.size() != 0)
         onTickTrigger_callback(this);*/
   }
}

void LevelCellManager::packToStream(Stream &stream, U32 tabStop, S32 behaviorID, U32 flags /* = 0  */)
{
   Parent::packToStream(stream, tabStop, behaviorID, flags);

   //Iterate over the children of this object and store them out to cell files
}

void LevelCellManager::visualizeFrustums(F32 renderTimeMS)
{
   
}

GameConnection* LevelCellManager::getConnection(S32 connectionID)
{
   for(NetConnection *conn = NetConnection::getConnectionList(); conn; conn = conn->getNext())  
   {  
      GameConnection* gameConn = dynamic_cast<GameConnection*>(conn);
  
      if (!gameConn || (gameConn && gameConn->isAIControlled()))
         continue; 

      if(connectionID == gameConn->getId())
         return gameConn;
   }

   return NULL;
}

void LevelCellManager::addClient(S32 clientID)
{
   
}

void LevelCellManager::removeClient(S32 clientID)
{

}

DefineEngineMethod( LevelCellManager, addClient, void,
                   ( S32 clientID ), ( -1 ),
                   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

                   "@param objB  Object to mount onto us\n"
                   "@param slot  Mount slot ID\n"
                   "@param txfm (optional) mount offset transform\n"
                   "@return true if successful, false if failed (objB is not valid)" )
{
   if(clientID == -1)
      return;

   object->addClient( clientID );
}

DefineEngineMethod( LevelCellManager, removeClient, void,
                   ( S32 clientID ), ( -1 ),
                   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

                   "@param objB  Object to mount onto us\n"
                   "@param slot  Mount slot ID\n"
                   "@param txfm (optional) mount offset transform\n"
                   "@return true if successful, false if failed (objB is not valid)" )
{
   if(clientID == -1)
      return;

   object->removeClient( clientID );
}

DefineEngineMethod( LevelCellManager, visualizeFrustums, void,
                   (F32 renderTime), (1000),
                   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

                   "@param objB  Object to mount onto us\n"
                   "@param slot  Mount slot ID\n"
                   "@param txfm (optional) mount offset transform\n"
                   "@return true if successful, false if failed (objB is not valid)" )
{
   object->visualizeFrustums(renderTime);
}