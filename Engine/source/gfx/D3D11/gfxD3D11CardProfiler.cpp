//-----------------------------------------------------------------------------
// Copyright (c) 2015 GarageGames, LLC
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

#include "gfx/D3D11/gfxD3D11Device.h"
#include "console/console.h"
#include "gfx/primBuilder.h"
#include "gfx/D3D11/gfxD3D11CardProfiler.h"
#include "gfx/D3D11/gfxD3D11EnumTranslate.h"
#include "platformWin32/videoInfo/wmiVideoInfo.h"

GFXD3D11CardProfiler::GFXD3D11CardProfiler() : GFXCardProfiler()
{
}

GFXD3D11CardProfiler::~GFXD3D11CardProfiler()
{

}

void GFXD3D11CardProfiler::init()
{
   IDXGIAdapter1 *adapter;
   IDXGIFactory1* factory;
   HRESULT hres;
   hres = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory));
   factory->EnumAdapters1(D3D11->getAdaterIndex(), &adapter);
   DXGI_ADAPTER_DESC1 desc;
   adapter->GetDesc1(&desc);

   mCardDescription = desc.Description;
   mVideoMemory = desc.DedicatedVideoMemory / 1048576; //convert to megabytes
   
   //WMI causes issues with graphics debugger in Visual studio
   
   /*
   WMIVideoInfo wmiVidInfo;
   if(wmiVidInfo.profileAdapters())
   {
      const PlatformVideoInfo::PVIAdapter &adapter = wmiVidInfo.getAdapterInformation(D3D11->getAdaterIndex());

      mCardDescription = adapter.description;
      mChipSet = adapter.chipSet;
      mVersionString = adapter.driverVersion;
      mVideoMemory = adapter.vram;
   }
   */
   adapter->Release();
   factory->Release();
   Parent::init();
}

void GFXD3D11CardProfiler::setupCardCapabilities()
{
	// anis -> resource limits direct3d11: http://msdn.microsoft.com/en-us/library/windows/desktop/ff819065(v=vs.85).aspx
}

bool GFXD3D11CardProfiler::_queryCardCap(const String &query, U32 &foundResult)
{
	return false;
}

bool GFXD3D11CardProfiler::_queryFormat( const GFXFormat fmt, const GFXTextureProfile *profile, bool &inOutAutogenMips )
{
   // anis -> D3D11 feature level should guarantee that any format is valid!
   return GFXD3D11TextureFormat[fmt] != DXGI_FORMAT_UNKNOWN;
}