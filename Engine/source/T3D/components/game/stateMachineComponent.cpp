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

#include "T3D/components/game/stateMachineComponent.h"

#include "platform/platform.h"
#include "console/consoleTypes.h"
#include "core/util/safeDelete.h"
#include "core/resourceManager.h"
#include "core/stream/fileStream.h"
#include "console/consoleTypes.h"
#include "console/consoleObject.h"
#include "ts/tsShapeInstance.h"
#include "core/stream/bitStream.h"
#include "gfx/gfxTransformSaver.h"
#include "console/engineAPI.h"
#include "lighting/lightQuery.h"

IMPLEMENT_CALLBACK( StateMachineComponent, onStateChange, void, (), (),
                   "@brief Called when we collide with another object.\n\n"
                   "@param obj The ShapeBase object\n"
                   "@param collObj The object we collided with\n"
                   "@param vec Collision impact vector\n"
                   "@param len Length of the impact vector\n" );

//////////////////////////////////////////////////////////////////////////
// Constructor/Destructor
//////////////////////////////////////////////////////////////////////////

StateMachineComponent::StateMachineComponent() : Component()
{
   mFriendlyName = "State Machine";
   mComponentType = "Game";

   mDescription = getDescriptionText("A generic state machine.");

   mStateMachineFile = "";

   mSMAssetId = StringTable->insert("");
   mSMAsset = StringTable->insert("");

   //doesn't need to be networked
   mNetworked = false;
   mNetFlags.clear();
}

StateMachineComponent::~StateMachineComponent()
{
   for(S32 i = 0;i < mFields.size();++i)
   {
      ComponentField &field = mFields[i];
      SAFE_DELETE_ARRAY(field.mFieldDescription);
   }

   SAFE_DELETE_ARRAY(mDescription);
}

IMPLEMENT_CO_NETOBJECT_V1(StateMachineComponent);

bool StateMachineComponent::onAdd()
{
   if(! Parent::onAdd())
      return false;

   // Register for the resource change signal.
   ResourceManager::get().getChangedSignal().notify(this, &StateMachineComponent::_onResourceChanged);
   mStateMachine.onStateChanged.notify(this, &StateMachineComponent::onStateChanged);

   return true;
}

void StateMachineComponent::onRemove()
{
   Parent::onRemove();
}

U32 StateMachineComponent::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);
   return retMask;
}

void StateMachineComponent::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);
}

//This is mostly a catch for situations where the behavior is re-added to the object and the like and we may need to force an update to the behavior
void StateMachineComponent::onComponentAdd()
{
   Parent::onComponentAdd();

   loadStateMachineFile();
}

void StateMachineComponent::onComponentRemove()
{
   Parent::onComponentRemove();
}

void StateMachineComponent::initPersistFields()
{
   Parent::initPersistFields();

   addProtectedField("StateMachineAsset", TypeStateMachineAssetPtr, Offset(mSMAsset, StateMachineComponent), &_setSMFile, &defaultProtectedGetFn,
      "The asset Id used for the state machine.", AbstractClassRep::FieldFlags::FIELD_ComponentInspectors);
}

bool StateMachineComponent::_setSMFile(void *object, const char *index, const char *data)
{
   StateMachineComponent* smComp = static_cast<StateMachineComponent*>(object);

   // Sanity!
   AssertFatal(data != NULL, "Cannot use a NULL asset Id.");

   return smComp->setStateMachineAsset(data);
}

bool StateMachineComponent::setStateMachineAsset(const char* assetName)
{
   // Fetch the asset Id.
   mSMAssetId = StringTable->insert(assetName);

   mSMAsset = mSMAssetId;

   if (mSMAsset.isNull())
   {
      Con::errorf("[StateMachineComponent] Failed to load state machine asset.");
      return false;
   }

   mSMAsset = mSMAssetId;

   mStateMachineFile = mSMAsset->getStateMachineFileName();
   loadStateMachineFile();

   return true;
}

void StateMachineComponent::_onResourceChanged(const Torque::Path &path)
{
   if (path != Torque::Path(mStateMachineFile))
      return;

   loadStateMachineFile();
}

void StateMachineComponent::setStateMachineFile(const char* fileName)
{ 
   mStateMachineFile = StringTable->insert(fileName); 
   //ResourceManager::get().load(mStateMachineFile);
}

void StateMachineComponent::loadStateMachineFile()
{
   if (!dStrIsEmpty(mStateMachineFile))
   {
      mStateMachine.mStateMachineFile = mStateMachineFile;
      mStateMachine.mOwnerObject = this;

      mStateMachine.loadStateMachineFile();

      //now that it's loaded, we need to parse the SM's fields and set them as script vars on ourselves
      S32 smFieldCount = mStateMachine.getFieldsCount();

      for (U32 i = 0; i < smFieldCount; i++)
      {
         StateMachine::StateField field = mStateMachine.getField(i);

         char buffer[128];

         if (field.data.fieldType == StateMachine::Field::BooleanType)
         {
            dSprintf(buffer, sizeof(buffer), "%b", field.data.boolVal);
            setDataField(field.name, NULL, buffer);
         }
         else if (field.data.fieldType == StateMachine::Field::NumberType)
         {
            dSprintf(buffer, sizeof(buffer), "%g", field.data.numVal);
            setDataField(field.name, NULL, buffer);
         }
         else if (field.data.fieldType == StateMachine::Field::StringType)
         {
            setDataField(field.name, NULL, field.data.stringVal);
         }
      }
   }
}

void StateMachineComponent::processTick()
{
   if (!isServerObject() || !isActive())
      return;

   mStateMachine.update();
}

void StateMachineComponent::onDynamicModified( const char* slotName, const char* newValue )
{
   Parent::onDynamicModified(slotName, newValue);

   if (!isServerObject() || !isActive())
      return;

   StringTableEntry fieldName = StringTable->insert(slotName);

   mStateMachine.setField(fieldName, newValue);
   mStateMachine.checkTransitions();
}

void StateMachineComponent::onStaticModified( const char* slotName, const char* newValue )
{
   Parent::onStaticModified(slotName, newValue);

   if (!isServerObject() || !isActive())
      return;

   StringTableEntry fieldName = StringTable->insert(slotName);

   mStateMachine.setField(fieldName, newValue);
   mStateMachine.checkTransitions();
}

void StateMachineComponent::onStateChanged(StateMachine* sm, S32 stateIdx)
{
   if (!isServerObject() || !isActive())
      return;

   //do a script callback, if we have one
   //check if we have a function for that, and then also check if our owner does
   StringTableEntry callbackName = mStateMachine.getCurrentState().callbackName;

   if (isMethod(callbackName))
         Con::executef(this, callbackName);

   if (mOwner->isMethod(callbackName))
      Con::executef(mOwner, callbackName);

   //we'll also do a state-agnostic callback for any general behavior

   if (isMethod("onStateChanged"))
      Con::executef(this, "onStateChanged", mStateMachine.getCurrentState().stateName);

   if (mOwner->isMethod("onStateChanged"))
      Con::executef(mOwner, "onStateChanged", mStateMachine.getCurrentState().stateName);
}