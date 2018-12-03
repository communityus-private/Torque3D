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
#include "gfx/gfxRenderDoc.h"
#include "console/console.h"
#include "windowManager/platformWindowMgr.h"
#ifdef TORQUE_OS_WIN
#include "platformWin32/platformWin32.h"
#include "gfx/D3D11/gfxD3D11Device.h"
#endif

RENDERDOC_API_1_3_0 *GFXRenderDoc::mRenderDocApi = NULL;
void *GFXRenderDoc::mModule = NULL;

void GFXRenderDoc::init()
{
#ifdef TORQUE_OS_WIN
   mModule = GetModuleHandleA("renderdoc.dll");
   if(mModule)
   {
      pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(static_cast<HMODULE>(mModule), "RENDERDOC_GetAPI");
      int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_0, (void **)&mRenderDocApi);
      if(ret !=1)
         Con::errorf("GFXRenderDoc::init: RENDERDOC_GetAPI failed");

      S32 major = 999, minor = 999, patch = 999;
      mRenderDocApi->GetAPIVersion(&major, &minor, &patch);
      Con::printf("GFXRenderDoc::init: Loaded RenderDoc version %i.%i.%i", major, minor, patch);
   }
#endif
}

void GFXRenderDoc::shutdown()
{
   if (mRenderDocApi)
   {
      mRenderDocApi->Shutdown();
   }

#ifdef TORQUE_OS_WIN
   if (mModule)
   {
      FreeLibrary(static_cast<HMODULE>(mModule));
   }
#endif
}

void GFXRenderDoc::triggerCapture()
{
   if (mRenderDocApi)
   {
      mRenderDocApi->TriggerCapture();
   }
}

void GFXRenderDoc::triggerMultiFrameCapture(const U32 numFrames)
{
   if (mRenderDocApi)
   {
      mRenderDocApi->TriggerMultiFrameCapture(numFrames);
   }
}


//TODO: should store the WindowManager->getFocusedWindow() just in case it might change between start/end call
void GFXRenderDoc::startFrameCapture()
{
   if (mRenderDocApi)
   {
      RENDERDOC_DevicePointer pDevice = NULL;
      RENDERDOC_WindowHandle pWindow = NULL;

   #ifdef TORQUE_OS_WIN
      if (GFX->getAdapterType() == GFXAdapterType::Direct3D11)
      {
         pDevice = D3D11DEVICE;
         pWindow = WindowManager->getFocusedWindow()->getSystemWindow(PlatformWindow::WindowSystem_Windows);
      }
   #endif

      mRenderDocApi->StartFrameCapture(pDevice, pWindow);
   }
}

void GFXRenderDoc::endFrameCapture()
{
   if (mRenderDocApi)
   {
      RENDERDOC_DevicePointer pDevice = NULL;
      RENDERDOC_WindowHandle pWindow = NULL;

   #ifdef TORQUE_OS_WIN
      if (GFX->getAdapterType() == GFXAdapterType::Direct3D11)
      {
         pDevice = D3D11DEVICE;
         pWindow = WindowManager->getFocusedWindow()->getSystemWindow(PlatformWindow::WindowSystem_Windows);
      }
   #endif

      mRenderDocApi->EndFrameCapture(pDevice, pWindow);
   }
}

bool GFXRenderDoc::isLoaded()
{
   if (mModule && mRenderDocApi)
      return true;
   else 
      return false;
}
#endif