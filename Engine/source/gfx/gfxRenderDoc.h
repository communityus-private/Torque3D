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
#if defined (renderDoc_ON)
#ifndef GFX_RENDER_DOC_H_
#define GFX_RENDER_DOC_H_

#ifndef _ENGINEOBJECT_H_
#include "console/engineObject.h"
#endif

#include "app/renderdoc_app.h"

//basic support for renderdoc api
class GFXRenderDoc
{
public:
   DECLARE_STATIC_CLASS(GFXRenderDoc);

   static void init();
   static void shutdown();

   static void triggerCapture();
   static void triggerMultiFrameCapture(const U32 numFrames);

   static void startFrameCapture();
   static void endFrameCapture();

   //is the renderdoc library loaded and are we ready to catpure? 
   static bool isLoaded();

private:

   //don't forget to update this struct version number as required
   static RENDERDOC_API_1_1_0 *mRenderDocApi;
   static void *mModule;

};

#endif
#endif