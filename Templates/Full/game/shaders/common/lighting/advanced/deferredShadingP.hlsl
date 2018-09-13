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

uniform float4x4 invCameraMat;
uniform float4x4 cameraMat;
uniform float2 nearFar;
uniform float4x4 matWorldToScreen;
uniform float4x4 matScreenToWorld;
uniform float2 worldToScreenScale;

// Avoid stepping zero distance
static const float	g_fMinRayStep = 0.01f;
// Crude raystep count
static const int	g_iMaxSteps = 16;
// Crude raystep scaling
static const float	g_fRayStep = 1.18f;
// Fine raystep count
static const int	g_iNumBinarySearchSteps = 16;
// Approximate the precision of the search (smaller is more precise)
static const float  g_fRayhitThreshold = 0.9f;

inline float4 reconstruct3DPos(in float2 inUV, in float depth, in float4x4 spaceMat)
{
	float4 positionSS = float4(float3(inUV.x, inUV.y, depth)*2-1, 1.0f);
   
	float4 position3D = mul(positionSS, spaceMat);
	return position3D/position3D.w;
}

inline float2 deconstruct3DPos(in float3 pos, in float4x4 invSpaceMat)
{
		float4 vProjectedCoord = mul(float4(pos, 1.0f), invSpaceMat);
		vProjectedCoord.xy /= vProjectedCoord.w;
		vProjectedCoord.xy = (vProjectedCoord.xy + float2(1,1))/2;
		return vProjectedCoord.xy;
}

float4 SSRBinarySearch(float3 vDir, inout float3 hitCoord, in float4x4 invSpaceMat)
{
	float fDepth;
	for (int i = 0; i < g_iNumBinarySearchSteps; i++)
	{
		float2 hitUV = deconstruct3DPos(hitCoord, invSpaceMat);
      
		fDepth = TORQUE_DEFERRED_UNCONDITION( deferredTex, hitUV ).w * nearFar.y;
		float fDepthDiff = hitCoord.z - fDepth;

		if (fDepthDiff <= 0.0f)
			hitCoord += vDir;
		vDir *= 0.5;
		hitCoord -= vDir;
	}

	float2 hitUV = deconstruct3DPos(hitCoord, invSpaceMat);

	fDepth = TORQUE_DEFERRED_UNCONDITION( deferredTex, hitUV ).w * nearFar.y;
	float fDepthDiff = hitCoord.z - fDepth;

	return float4(hitUV, fDepth, abs(fDepthDiff) < g_fRayhitThreshold ? 1.0f : 0.0f);
}

float4 SSRRayMarch(float3 vDir, inout float3 hitCoord, in float4x4 invSpaceMat, float steplen)
{
	float fDepth;
   float fDepthDiff = 0;
	for (int i = 0; i < g_iMaxSteps; i++)
	{
		hitCoord += vDir;
		float2 hitUV = deconstruct3DPos(hitCoord, invSpaceMat);

		fDepth = TORQUE_DEFERRED_UNCONDITION( deferredTex,hitUV).w * nearFar.y;
		fDepthDiff = hitCoord.z - fDepth;
		[branch]
		if (fDepthDiff > 0.0f)
			return SSRBinarySearch(vDir, hitCoord,invSpaceMat);

		vDir *= steplen;
	}

	return float4(0.0f, 0.0f, 0.0f, 0.0f);
}


float4 main( PFXVertToPix IN) : TORQUE_TARGET0
{        
   float4 normDepth = TORQUE_DEFERRED_UNCONDITION( deferredTex, IN.uv0 );
   float depth = normDepth.w;
      
   if (depth>0.9999)
      return float4(0,0,0,0);
      
   float3 posVS = reconstruct3DPos(IN.uv0,depth,cameraMat).xyz;   

	float3 reflectDir = normalize(reflect(posVS.xyz, normDepth.xyz));
	float4 vCoords = SSRRayMarch(reflectDir, posVS, invCameraMat, g_fRayStep);
   
	float2 vCoordsEdgeFact = float2(1, 1) - pow(saturate(abs(vCoords.xy - float2(0.5f, 0.5f)) * 2), 8);
	float fScreenEdgeFactor = saturate(min(vCoordsEdgeFact.x, vCoordsEdgeFact.y));

	//Color
	float reflectionIntensity =
		saturate(
			fScreenEdgeFactor *		// screen fade
			saturate(reflectDir.z)	// camera facing fade
			* vCoords.w				// rayhit binary fade
			);
         
	float4 ssrColor = TORQUE_TEX2D( colorBufferTex, vCoords.xy );
   float3 albedo = TORQUE_TEX2D( colorBufferTex, IN.uv0 ).rgb; //albedo
   float4 matInfo = TORQUE_TEX2D(matInfoTex, IN.uv0); //flags|smoothness|ao|metallic

   bool emissive = getFlag(matInfo.r, 0);
   if (emissive)
   {
      return float4(albedo, 1.0);
   }
	  
   float4 diffuse = TORQUE_TEX2D( diffuseLightingBuffer, IN.uv0 ); //shadowmap*specular
   float4 specular = TORQUE_TEX2D( specularLightingBuffer, IN.uv0 ); //environment mapping*lightmaps
   
   specular = lerp(specular,ssrColor,reflectionIntensity);
   
   float metalness = matInfo.a;
   
   float3 diffuseColor = albedo - (albedo * metalness);
   float3 specularColor = lerp(float3(0.04,0.04,0.04), albedo, metalness);

   float3 light = (diffuseColor * diffuse.rgb) + (specularColor * specular.rgb);

   
   return float4(light.rgb, 1.0);
}
