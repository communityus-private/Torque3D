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

#ifndef LEVEL_CELL_MANAGER_COMPONENT_H
#define LEVEL_CELL_MANAGER_COMPONENT_H

#ifndef _COMPONENT_H_
#include "T3D/components/component.h"
#endif
#ifndef _ENTITY_H_
#include "T3D/entity.h"
#endif
#ifndef _COLLISION_INTERFACES_H_
#include "T3D/components/collision/collisionInterfaces.h"
#endif

//////////////////////////////////////////////////////////////////////////
/// 
/// 
//////////////////////////////////////////////////////////////////////////
class LevelCellManager : public Component
{
   typedef Component Parent;

protected:

   struct ClientActiveCellData
   {
      GameConnection* clientConnection;

      U32 mainCellIndex;
      Vector<U32> adjacentCells;
   };

   Vector<ClientActiveCellData> mClientInfo;

   Vector<SceneObject*> mObjectList;

   bool mVisible;

   String mEnterCommand;
   String mOnExitCommand;
   String mOnUpdateInViewCmd;

   S32 mCellWidth;

public:
   LevelCellManager();
   virtual ~LevelCellManager();
   DECLARE_CONOBJECT(LevelCellManager);

   virtual bool onAdd();
   virtual void onRemove();
   static void initPersistFields();

   virtual void onComponentAdd();
   virtual void onComponentRemove();

   virtual void componentAddedToOwner(Component *comp);
   virtual void componentRemovedFromOwner(Component *comp);

   virtual U32 packUpdate(NetConnection *con, U32 mask, BitStream *stream);
   virtual void unpackUpdate(NetConnection *con, BitStream *stream);

   void potentialEnterObject(SceneObject *collider);

   bool testObject(SceneObject* enter);

   virtual void processTick();

   virtual void packToStream(Stream &stream, U32 tabStop, S32 behaviorID, U32 flags /* = 0  */);

   GameConnection* getConnection(S32 connectionID);

   void addClient(S32 clientID);
   void removeClient(S32 clientID);

   void visualizeFrustums(F32 renderTimeMS);

   DECLARE_CALLBACK(void, onEnterViewCmd, (Entity* cameraEnt, bool firstTimeSeeing));
   DECLARE_CALLBACK(void, onExitViewCmd, (Entity* cameraEnt));
   DECLARE_CALLBACK(void, onUpdateInViewCmd, (Entity* cameraEnt));
   DECLARE_CALLBACK(void, onUpdateOutOfViewCmd, (Entity* cameraEnt));
};

#endif // LEVEL_CELL_MANAGER_COMPONENT_H
