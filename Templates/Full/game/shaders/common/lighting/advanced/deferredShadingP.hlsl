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
uniform float4x4 cameraProj;
uniform float2 nearFar;

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

float4 SSRBinarySearch(float3 vDir, inout float3 hitCoord)
{
	float fDepth;

	for (int i = 0; i < g_iNumBinarySearchSteps; i++)
	{
		float4 vProjectedCoord = mul(float4(hitCoord, 1.0f), cameraProj);
		vProjectedCoord.xy /= vProjectedCoord.w;
		vProjectedCoord.xy = vProjectedCoord.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
      
		fDepth = TORQUE_DEFERRED_UNCONDITION( deferredTex, vProjectedCoord.xy ).w * nearFar.y;
		float fDepthDiff = hitCoord.z - fDepth;

		if (fDepthDiff <= 0.0f)
			hitCoord += vDir;

		vDir *= 0.5f;
		hitCoord -= vDir;
	}

	float4 vProjectedCoord = mul(float4(hitCoord, 1.0f), cameraProj);
	vProjectedCoord.xy /= vProjectedCoord.w;
	vProjectedCoord.xy = vProjectedCoord.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

	fDepth = TORQUE_DEFERRED_UNCONDITION( deferredTex, vProjectedCoord.xy ).w * nearFar.y;
	float fDepthDiff = hitCoord.z - fDepth;

	return float4(vProjectedCoord.xy, fDepth, abs(fDepthDiff) < g_fRayhitThreshold ? 1.0f : 0.0f);
}

float4 SSRRayMarch(float3 vDir, inout float3 hitCoord)
{
	float fDepth;

	for (int i = 0; i < g_iMaxSteps; i++)
	{
		hitCoord += vDir;

		float4 vProjectedCoord = mul(float4(hitCoord, 1.0f), cameraProj);
		vProjectedCoord.xy /= vProjectedCoord.w;
		vProjectedCoord.xy = vProjectedCoord.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

		fDepth = TORQUE_DEFERRED_UNCONDITION( deferredTex, vProjectedCoord.xy).w * nearFar.y;

		float fDepthDiff = hitCoord.z - fDepth;

		[branch]
		if (fDepthDiff > 0.0f)
			return SSRBinarySearch(vDir, hitCoord);

		vDir *= g_fRayStep;

	}

	return float4(0.0f, 0.0f, 0.0f, 0.0f);
}

inline float3 reconstructVS(in float2 inUV, in float depth, in float4x4 InvVP)
{
	float4 positionSS = float4(inUV.x * 2 - 1, inUV.y * 2 - 1, depth* 2 - 1, 1.0f);
	float4 positionVS = mul(positionSS, InvVP);
	return positionVS.xyz;
}
float4 main( PFXVertToPix IN) : TORQUE_TARGET0
{        
   float4 normDepth = TORQUE_DEFERRED_UNCONDITION( deferredTex, IN.uv0 );
   float depth = normDepth.w;
      
   if (depth>0.9999)
      return float4(0,0,0,0);
      
   float3 posWS = reconstructVS(IN.uv0,depth* nearFar.y,invCameraMat).xyz;   
   float3 normWS = mul(float4(normDepth.xyz, 1), invCameraMat).xyz;
   
   float3 posVS = mul(float4(posWS, 0),cameraMat).xyz;   
   float3 normVS = mul(float4(normWS, 1), cameraMat).xyz;
   
	float3 reflectDir = normalize(reflect(posVS.xyz, normVS.xyz));

	float4 vCoords = SSRRayMarch(reflectDir, posVS);
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

   //albedo = diffuseColor+lerp(reflectColor,indiffuseLighting,frez);
   //albedo *= max(diffuseLighting.rgb,float3(0,0,0));
   
   return float4(light.rgb, 1.0);
   //return float4(light.rgb, 1.0);
}
