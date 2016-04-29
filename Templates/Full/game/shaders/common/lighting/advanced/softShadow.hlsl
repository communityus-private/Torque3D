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

#include "../../shaderModel.hlsl"

//#if defined( SOFTSHADOW ) && defined( SOFTSHADOW_HIGH_QUALITY )

// Optimized PCF from https://github.com/TheRealMJP/Shadows/

float sampleShadowMap(TORQUE_SAMPLER2DCMP(shadowMap), in float2 base_uv, in float u, in float v, in float depth, in float filterRadius)
{
   float2 uv = base_uv + float2(u, v) *filterRadius;
   return TORQUE_TEX2DCMP(shadowMap, uv, depth);
}

float2 computeReceiverPlaneDepthBias(float3 texCoordDX, float3 texCoordDY)
{
   float2 biasUV;
   biasUV.x = texCoordDY.y * texCoordDX.z - texCoordDX.y * texCoordDY.z;
   biasUV.y = texCoordDX.x * texCoordDY.z - texCoordDY.x * texCoordDX.z;
   biasUV *= 1.0f / ((texCoordDX.x * texCoordDY.y) - (texCoordDX.y * texCoordDY.x));
   return biasUV;
}

float softShadow_filter(   TORQUE_SAMPLER2DCMP(shadowMap),
                           float2 vpos,
                           float2 shadowPos,
                           float filterRadius,
                           float distToLight,
                           float dotNL,
                           float3 shadowPosDX,
                           float3 shadowPosDY)
{
   float2 shadowMapSize;
   //bad, again just for testing, production version should pass in shadowmap size
   texture_shadowMap.GetDimensions(shadowMapSize.x, shadowMapSize.y);

   float shadow = 0;
   float lightDepth = distToLight;

   float2 uv = shadowPos * shadowMapSize;
   float2 shadowMapSizeInv = 1.0 / shadowMapSize;
   float2 receiverPlaneDepthBias = computeReceiverPlaneDepthBias(shadowPosDX, shadowPosDY);
   float fractionalSamplingError = dot(float2(1.0f, 1.0f) * shadowMapSizeInv, abs(receiverPlaneDepthBias));
   lightDepth -= min(fractionalSamplingError, 0.01f);
   float2 base_uv;
   base_uv.x = floor(uv.x + 0.5);
   base_uv.y = floor(uv.y + 0.5);

   float s = (uv.x + 0.5 - base_uv.x);
   float t = (uv.y + 0.5 - base_uv.y);

   base_uv -= float2(0.5, 0.5);
   base_uv *= shadowMapSizeInv;

   float sum = 0;

   float uw0 = (5 * s - 6);
   float uw1 = (11 * s - 28);
   float uw2 = -(11 * s + 17);
   float uw3 = -(5 * s + 1);

   float u0 = (4 * s - 5) / uw0 - 3;
   float u1 = (4 * s - 16) / uw1 - 1;
   float u2 = -(7 * s + 5) / uw2 + 1;
   float u3 = -s / uw3 + 3;

   float vw0 = (5 * t - 6);
   float vw1 = (11 * t - 28);
   float vw2 = -(11 * t + 17);
   float vw3 = -(5 * t + 1);

   float v0 = (4 * t - 5) / vw0 - 3;
   float v1 = (4 * t - 16) / vw1 - 1;
   float v2 = -(7 * t + 5) / vw2 + 1;
   float v3 = -t / vw3 + 3;

   sum += uw0 * vw0 * sampleShadowMap(TORQUE_SAMPLER2D_MAKEARG(shadowMap), base_uv, u0, v0, lightDepth, filterRadius);
   sum += uw1 * vw0 * sampleShadowMap(TORQUE_SAMPLER2D_MAKEARG(shadowMap), base_uv, u1, v0, lightDepth, filterRadius);
   sum += uw2 * vw0 * sampleShadowMap(TORQUE_SAMPLER2D_MAKEARG(shadowMap), base_uv, u2, v0, lightDepth, filterRadius);
   sum += uw3 * vw0 * sampleShadowMap(TORQUE_SAMPLER2D_MAKEARG(shadowMap), base_uv, u3, v0, lightDepth, filterRadius);

   sum += uw0 * vw1 * sampleShadowMap(TORQUE_SAMPLER2D_MAKEARG(shadowMap), base_uv, u0, v1, lightDepth, filterRadius);
   sum += uw1 * vw1 * sampleShadowMap(TORQUE_SAMPLER2D_MAKEARG(shadowMap), base_uv, u1, v1, lightDepth, filterRadius);
   sum += uw2 * vw1 * sampleShadowMap(TORQUE_SAMPLER2D_MAKEARG(shadowMap), base_uv, u2, v1, lightDepth, filterRadius);
   sum += uw3 * vw1 * sampleShadowMap(TORQUE_SAMPLER2D_MAKEARG(shadowMap), base_uv, u3, v1, lightDepth, filterRadius);

   sum += uw0 * vw2 * sampleShadowMap(TORQUE_SAMPLER2D_MAKEARG(shadowMap), base_uv, u0, v2, lightDepth, filterRadius);
   sum += uw1 * vw2 * sampleShadowMap(TORQUE_SAMPLER2D_MAKEARG(shadowMap), base_uv, u1, v2, lightDepth, filterRadius);
   sum += uw2 * vw2 * sampleShadowMap(TORQUE_SAMPLER2D_MAKEARG(shadowMap), base_uv, u2, v2, lightDepth, filterRadius);
   sum += uw3 * vw2 * sampleShadowMap(TORQUE_SAMPLER2D_MAKEARG(shadowMap), base_uv, u3, v2, lightDepth, filterRadius);

   sum += uw0 * vw3 * sampleShadowMap(TORQUE_SAMPLER2D_MAKEARG(shadowMap), base_uv, u0, v3, lightDepth, filterRadius);
   sum += uw1 * vw3 * sampleShadowMap(TORQUE_SAMPLER2D_MAKEARG(shadowMap), base_uv, u1, v3, lightDepth, filterRadius);
   sum += uw2 * vw3 * sampleShadowMap(TORQUE_SAMPLER2D_MAKEARG(shadowMap), base_uv, u2, v3, lightDepth, filterRadius);
   sum += uw3 * vw3 * sampleShadowMap(TORQUE_SAMPLER2D_MAKEARG(shadowMap), base_uv, u3, v3, lightDepth, filterRadius);

   return sum * 1.0f / 2704;

   return shadow;
}