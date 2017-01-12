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

#ifndef EXAMPLE_COMPONENT_H
#define EXAMPLE_COMPONENT_H
#pragma once

#ifndef COMPONENT_H
#include "T3D/components/component.h"
#endif

//ExampleComponent
//A basic example of the various functions you can utilize to make your own component!
//This example doesn't really DO anything, persay, but you can readily copy it as a base
//and use it as a starting point for your own.
class ExampleComponent : public Component, public EditorInspectInterface
{
   typedef Component Parent;

public:
   ExampleComponent();
   virtual ~ExampleComponent();
   DECLARE_CONOBJECT(ExampleComponent);

   virtual bool onAdd();
   virtual void onRemove();
   static void initPersistFields();

   virtual U32 packUpdate(NetConnection *con, U32 mask, BitStream *stream);
   virtual void unpackUpdate(NetConnection *con, BitStream *stream);

   virtual void onComponentRemove();
   virtual void onComponentAdd();

   virtual void componentAddedToOwner(Component *comp);
   virtual void componentRemovedFromOwner(Component *comp);

   virtual void onInspect();
   virtual void onEndInspect();

   virtual void processTick();
   virtual void advanceTime(F32 dt);
   virtual void interpolateTick(F32 delta);
};

#endif
