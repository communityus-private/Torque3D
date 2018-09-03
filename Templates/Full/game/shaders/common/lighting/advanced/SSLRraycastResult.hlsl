// Originating work by By Morgan McGuire and Michael Mara at Williams College 2014
// Released as open source under the BSD 2-Clause License
// http://opensource.org/licenses/BSD-2-Clause
//
// Copyright (c) 2014, Morgan McGuire and Michael Mara
// All rights reserved.
//
// From McGuire and Mara, Efficient GPU Screen-Space Ray Tracing,
// Journal of Computer Graphics Techniques, 2014
//
// This software is open source under the "BSD 2-clause license":
//
// Redistribution and use in source and binary forms, with or
// without modification, are permitted provided that the following
// conditions are met:
//
// 1. Redistributions of source code must retain the above
// copyright notice, this list of conditions and the following
// disclaimer.
//
// 2. Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following
// disclaimer in the documentation and/or other materials provided
// with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
// CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
// USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
// AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
// IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
/**
 * The ray tracing step of the SSLR implementation.
 * Modified version of the work stated above.
 */
#include "SSLRConstantBuffer.hlsl"
#include "../../shaderModelAutoGen.hlsl"
#include "../../postfx/postFx.hlsl"
#include "shaders/common/torque.hlsl"

TORQUE_UNIFORM_SAMPLER2D(deferredTex,0);

				   
float distanceSquared(float2 a, float2 b)
{
    a -= b;
    return dot(a, a);
}

bool intersectsDepthBuffer(float z, float minZ, float maxZ)
{
    /*
     * Based on how far away from the camera the depth is,
     * adding a bit of extra thickness can help improve some
     * artifacts. Driving this value up too high can cause
     * artifacts of its own.
     */
    float depthScale = min(1.0f, z * cb_strideZCutoff);
    z += cb_zThickness + lerp(0.0f, 2.0f, depthScale);
    return (maxZ >= z) && (minZ - cb_zThickness <= z);
}

void swap(inout float a, inout float b)
{
    float t = a;
    a = b;
    b = t;
}

float getDepthAt(float2 hitPixel)
{
    //return 0 for any value accessed out of bounds
	if ((hitPixel.x != saturate(hitPixel).x)&&((hitPixel.y != saturate(hitPixel).y)))
		return 0;
		
    return TORQUE_DEFERRED_UNCONDITION( deferredTex, hitPixel ).w;	
}

float linearDepthTexelFetch(float2 hitPixel)
{
    return getDepthAt(hitPixel);
}

// Returns true if the ray hit something
bool traceScreenSpaceRay(
    // Camera-space ray origin, which must be within the view volume
    float3 csOrig,
    // Unit length camera-space ray direction
    float3 csDir,
    // Number between 0 and 1 for how far to bump the ray in stride units
    // to conceal banding artifacts. Not needed if stride == 1.
    float jitter,
    // Pixel coordinates of the first intersection with the scene
    out float2 hitPixel,
    // Camera space location of the ray hit
    out float3 hitPoint)
{
    // Clip to the near plane
    float rayLength = ((csOrig.z + csDir.z * cb_maxDistance) < cb_nearPlaneZ) ?
    (cb_nearPlaneZ - csOrig.z) / csDir.z : cb_maxDistance;
    float3 csEndPoint = csOrig + csDir * rayLength;

    // Project into homogeneous clip space
    float4 H0 = mul(float4(csOrig, 1.0f), viewToTextureSpaceMatrix);
    H0.xy *= targetSize;
    float4 H1 = mul(float4(csEndPoint, 1.0f), viewToTextureSpaceMatrix);
    H1.xy *= targetSize;
    float k0 = 1.0f / H0.w;
    float k1 = 1.0f / H1.w;

    // The interpolated homogeneous version of the camera-space points
    float3 Q0 = csOrig * k0;
    float3 Q1 = csEndPoint * k1;

    // Screen-space endpoints
    float2 P0 = H0.xy * k0;
    float2 P1 = H1.xy * k1;

    // If the line is degenerate, make it cover at least one pixel
    // to avoid handling zero-pixel extent as a special case later
    P1 += (distanceSquared(P0, P1) < 0.0001f) ? float2(0.01f, 0.01f) : 0.0f;
    float2 delta = P1 - P0;

    // Permute so that the primary iteration is in x to collapse
    // all quadrant-specific DDA cases later
    bool permute = false;
    if(abs(delta.x) < abs(delta.y))
    {
        // This is a more-vertical line
        permute = true;
        delta = delta.yx;
        P0 = P0.yx;
        P1 = P1.yx;
    }

    float stepDir = sign(delta.x);
    float invdx = stepDir / delta.x;

    // Track the derivatives of Q and k
    float3 dQ = (Q1 - Q0) * invdx;
    float dk = (k1 - k0) * invdx;
    float2 dP = float2(stepDir, delta.y * invdx);

    // Scale derivatives by the desired pixel stride and then
    // offset the starting values by the jitter fraction
    float strideScale = 1.0f - min(1.0f, csOrig.z * cb_strideZCutoff);
    float stride = 1.0f + strideScale * cb_stride;
    dP *= stride;
    dQ *= stride;
    dk *= stride;

    P0 += dP * jitter;
    Q0 += dQ * jitter;
    k0 += dk * jitter;

    // Slide P from P0 to P1, (now-homogeneous) Q from Q0 to Q1, k from k0 to k1
    float4 PQk = float4(P0, Q0.z, k0);
    float4 dPQk = float4(dP, dQ.z, dk);
    float3 Q = Q0; 

    // Adjust end condition for iteration direction
    float end = P1.x * stepDir;

    float stepCount = 0.0f;
    float prevZMaxEstimate = csOrig.z;
    float rayZMin = prevZMaxEstimate;
    float rayZMax = prevZMaxEstimate;
    float sceneZMax = rayZMax + 100.0f;
    for(;
        ((PQk.x * stepDir) <= end) && (stepCount < cb_maxSteps) &&
        !intersectsDepthBuffer(sceneZMax, rayZMin, rayZMax) &&
        (sceneZMax != 0.0f);
        ++stepCount)
    {
        rayZMin = prevZMaxEstimate;
        rayZMax = (dPQk.z * 0.5f + PQk.z) / (dPQk.w * 0.5f + PQk.w);
        prevZMaxEstimate = rayZMax;
        if(rayZMin > rayZMax)
        {
            swap(rayZMin, rayZMax);
        }

        hitPixel = permute ? PQk.yx : PQk.xy;
        // You may need hitPixel.y = depthBufferSize.y - hitPixel.y; here if your vertical axis
        // is different than ours in screen space
        sceneZMax = linearDepthTexelFetch(hitPixel);

        PQk += dPQk;
    }

    // Advance Q based on the number of steps
    Q.xy += dQ.xy * stepCount;
    hitPoint = Q * (1.0f / PQk.w);
    return intersectsDepthBuffer(sceneZMax, rayZMin, rayZMax);
}

//The return values include the UV coordinates of the ray hit in the x and y components,
// the depth in the z component,
// and the dot product of the view ray
// and the reflection ray in the w component.
//  The value stored in the w-component is used in the cone tracing step to fade rays facing towards the camera.
//  Black pixels mark areas where no intersection occurred.
float4 main(PFXVertToPix IN) : TORQUE_TARGET0
{
    float3 normalVS = TORQUE_DEFERRED_UNCONDITION( deferredTex, IN.uv0 ).xyz;
    if(!any(normalVS))
    {
        return 0.0f;
    }

    float depth = TORQUE_DEFERRED_UNCONDITION( deferredTex, IN.uv0 ).w;
    float3 rayOriginVS = IN.wsEyeRay * depth;

    /*
     * Since position is reconstructed in view space, just normalize it to get the
     * vector from the eye to the position and then reflect that around the normal to
     * get the ray direction to trace.
     */
    float3 toPositionVS = normalize(rayOriginVS);
    float3 rayDirectionVS = normalize(reflect(toPositionVS, normalVS));

    // output rDotV to the alpha channel for use in determining how much to fade the ray
    float rDotV = dot(rayDirectionVS, toPositionVS);

    // out parameters
    float2 hitPixel = float2(0.0f, 0.0f);
    float3 hitPoint = float3(0.0f, 0.0f, 0.0f);

    float jitter = cb_stride > 1.0f ? float(int(IN.hpos.x + IN.hpos.y) & 1) * 0.5f : 0.0f;

    // perform ray tracing - true if hit found, false otherwise
    bool intersection = traceScreenSpaceRay(rayOriginVS, rayDirectionVS, jitter, hitPixel, hitPoint);

    depth = getDepthAt(hitPixel);

    // move hit pixel from pixel position to UVs
    hitPixel *= oneOverTargetSize;
    if(hitPixel.x > 1.0f || hitPixel.x < 0.0f || hitPixel.y > 1.0f || hitPixel.y < 0.0f)
    {
        intersection = false;
    }

    return float4(hitPixel, depth, rDotV) * (intersection ? 1.0f : 0.0f);
}