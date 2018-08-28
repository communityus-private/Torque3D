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

#include "../../shaderModelAutoGen.hlsl"
#include "../../postfx/postFx.hlsl"
#include "shaders/common/torque.hlsl"

TORQUE_UNIFORM_SAMPLER2D(colorBufferTex,0);
TORQUE_UNIFORM_SAMPLER2D(diffuseLightingBuffer,1);
TORQUE_UNIFORM_SAMPLER2D(matInfoTex,2);
TORQUE_UNIFORM_SAMPLER2D(specularLightingBuffer,3);
TORQUE_UNIFORM_SAMPLER2D(deferredTex,4);

uniform float radius;
uniform float2 targetSize;
uniform int captureRez;
float4 main( PFXVertToPix IN) : TORQUE_TARGET0
{        
   float depth = TORQUE_DEFERRED_UNCONDITION( deferredTex, IN.uv0 ).w;
   if (depth>0.9999)
      clip(-1);
   float3 colorBuffer = TORQUE_TEX2D( colorBufferTex, IN.uv0 ).rgb; //albedo
   float4 matInfo = TORQUE_TEX2D(matInfoTex, IN.uv0); //flags|smoothness|ao|metallic

   bool emissive = getFlag(matInfo.r, 0);
   if (emissive)
   {
      return float4(colorBuffer, 1.0);
   }
	  
   float4 diffuseLighting = TORQUE_TEX2D( diffuseLightingBuffer, IN.uv0 ); //shadowmap*specular
   colorBuffer *= diffuseLighting.rgb;
   float2 relUV = IN.uv0*targetSize/captureRez;
   
   //we use a 1k depth range in the capture frustum. 
   //reduce that a bit to get something resembling depth fidelity out of 8 bits
   depth*=2000/radius;
   
   float rLen = length(float3(relUV,depth)-float3(0.5,0.5,0));
   return hdrEncode( float4(colorBuffer,rLen));
}
