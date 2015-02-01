//-----------------------------------------------------------------------------
// Copyright (c) 2015 Andrew Mac
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

#include "shadergen:/autogenConditioners.h"
#include "torque.hlsl"

struct Conn
{
   float4 position : POSITION;
   float2 uv0      : TEXCOORD0;
   float3 wsEyeRay : TEXCOORD1;
};

uniform sampler2D prePassBuffer : register(S1);
uniform sampler3D lpvData : register(S0);
uniform float3 eyePosWorld;
uniform float3 volumeStart;
uniform float3 volumeSize;

float4 main( Conn IN ) : COLOR0
{ 
   float4 prepassSample = prepassUncondition( prePassBuffer, IN.uv0 );
   float3 normal = prepassSample.rgb;
   float depth = prepassSample.a;

   // Use eye ray to get ws pos
   float4 worldPos = float4(eyePosWorld + IN.wsEyeRay * depth, 1.0f);

   float3 volume_position = (worldPos.xyz - volumeStart) / volumeSize;
   if ( volume_position.x < 0 || volume_position.x > 1 || 
        volume_position.y < 0 || volume_position.y > 1 || 
        volume_position.z < 0 || volume_position.z > 1 )
   {
        return float4(0.0, 0.0, 0.0, 0.0); 
   }

   float4 color = tex3D(lpvData, volume_position);
   return float4(color.rgb, 0.0);
   //return float4(0.0, 0.0, 0.0, 0.0); 
}
