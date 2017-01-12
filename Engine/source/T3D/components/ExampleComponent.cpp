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
#include "T3D/components/ExampleComponent.h"
#include "core/stream/bitStream.h"
#include "sim/netConnection.h"

//////////////////////////////////////////////////////////////////////////
// Constructor/Destructor
//////////////////////////////////////////////////////////////////////////
ExampleComponent::ExampleComponent() : Component()
{
   //These flags inform that, in this particular component, we network down to the client, which enables the pack/unpackData functions to operate
   mNetworked = true;
   mNetFlags.set(Ghostable | ScopeAlways);
}

ExampleComponent::~ExampleComponent() 
{
}

IMPLEMENT_CO_NETOBJECT_V1(ExampleComponent);

//Standard onAdd function, for when the component is created
bool ExampleComponent::onAdd()
{
   if (!Parent::onAdd())
      return false;

   return true;
}

//Standard onRemove function, when the component object is deleted
void ExampleComponent::onRemove()
{
   Parent::onRemove();
}

//This is called when the component has been added to an entity
void ExampleComponent::onComponentAdd()
{
   Parent::onComponentAdd();

   Con::printf("We were added to an entity! ExampleComponent reporting in for owner entity %g", mOwner->getId());
}

//This is called when the component has been removed from an entity
void ExampleComponent::onComponentRemove()
{
   Parent::onComponentRemove();

   Con::printf("We were removed from our entity! ExampleComponent signing off for owner entity %g", mOwner->getId());
}

//This is called any time a component is added to an entity. Every component currently owned by the entity is informed of the event. 
//This allows you to do dependency behavior, like collisions being aware of a mesh component, etc
void ExampleComponent::componentAddedToOwner(Component *comp)
{
   Con::printf("Our owner entity has a new component being added! ExampleComponent welcomes component %g of type %s", comp->getId(), comp->getClassRep()->getNameSpace());
}

//This is called any time a component is removed from an entity. Every component current owned by the entity is informed of the event.
//This allows cleanup and dependency management.
void ExampleComponent::componentRemovedFromOwner(Component *comp)
{
   Con::printf("Our owner entity has a removed a component! ExampleComponent waves farewell to component %g of type %s", comp->getId(), comp->getClassRep()->getNameSpace());
}

//Regular init persist fields function to set up static fields.
void ExampleComponent::initPersistFields()
{
   Parent::initPersistFields();
}

U32 ExampleComponent::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);
   
   return retMask;
}

void ExampleComponent::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);
}

//This allows custom behavior in the event the owner is being edited
void ExampleComponent::onInspect()
{
}

//This allows cleanup of the custom editor behavior if our owner stopped being edited
void ExampleComponent::onEndInspect()
{
}

//Process tick update function, natch
void ExampleComponent::processTick()
{
   Parent::processTick();
}

//Client-side advance function
void ExampleComponent::advanceTime(F32 dt)
{

}

//Client-side interpolation function
void ExampleComponent::interpolateTick(F32 delta)
{

}