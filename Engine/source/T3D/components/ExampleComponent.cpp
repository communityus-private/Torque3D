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
   mNetworked = true;
   mNetFlags.set(Ghostable | ScopeAlways);
}

ExampleComponent::~ExampleComponent() {}

IMPLEMENT_CO_NETOBJECT_V1(ExampleComponent);

bool ExampleComponent::onAdd()
{
   if (!Parent::onAdd())
      return false;

   return true;
}

void ExampleComponent::onRemove()
{
   Parent::onRemove();
}

void ExampleComponent::onComponentAdd()
{
   Parent::onComponentAdd();
}

void ExampleComponent::onComponentRemove()
{
   Parent::onComponentRemove();
}

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

void ExampleComponent::onInspect()
{
}

void ExampleComponent::onEndInspect()
{
}