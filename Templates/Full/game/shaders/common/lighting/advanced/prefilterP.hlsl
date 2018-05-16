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

#include "../../torque.hlsl"

struct ConnectData
{
    float4 hpos     : SV_Position;
    float2 uv      : TEXCOORD;
};

TORQUE_UNIFORM_SAMPLERCUBE(environmentMap, 0);

uniform float roughness;
uniform int face;

// Based on http://byteblacksmith.com/improvements-to-the-canonical-one-liner-glsl-rand-for-opengl-es-2-0/
float random(float2 co)
{
	float a = 12.9898;
	float b = 78.233;
	float c = 43758.5453;
	float dt= dot(co.xy ,float2(a,b));
	float sn= mod(dt,3.14);
	return frac(sin(sn) * c); //port note, glsl is fract
}

float2 hammersley2d(uint i, uint N) 
{
	// Radical inverse based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
	uint bits = (i << 16u) | (i >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	float rdi = float(bits) * 2.3283064365386963e-10;
	return float2(float(i) /float(N), rdi);
}

// Based on http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_slides.pdf
float3 importanceSample_GGX(float2 Xi, float roughness, float3 normal) 
{
	// Maps a 2D point to a hemisphere with spread based on roughness
	float alpha = roughness * roughness;
	float phi = 2.0 * M_PI_F * Xi.x + random(normal.xz) * 0.1;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (alpha*alpha - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	float3 H = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

	// Tangent space
	float3 up = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	float3 tangentX = normalize(cross(up, normal));
	float3 tangentY = normalize(cross(normal, tangentX));

	// Convert to world Space
	return normalize(tangentX * H.x + tangentY * H.y + normal * H.z);
}

// Normal Distribution function
float D_GGX(float dotNH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2)/(M_PI_F * denom*denom); 
}

float3 prefilterEnvMap(float3 R)
{
	float3 N = R;
	float3 V = R;
	float3 color = 0;
	float totalWeight = 0.0;
	uint numSamples = 32; //todo could pass this value in

	float width;
	float height;
	texture_environmentMap.GetDimensions(width, height);
	for(uint i = 0u; i < numSamples; i++) 
	{
		float2 Xi = hammersley2d(i, numSamples);
		float3 H = importanceSample_GGX(Xi, roughness, N);
		float3 L = 2.0 * dot(V, H) * H - V;
		float dotNL = clamp(dot(N, L), 0.0, 1.0);
		if(dotNL > 0.0) 
		{
			// Filtering based on https://placeholderart.wordpress.com/2015/07/28/implementation-notes-runtime-environment-map-filtering-for-image-based-lighting/
			float dotNH = clamp(dot(N, H), 0.0, 1.0);
			float dotVH = clamp(dot(V, H), 0.0, 1.0);
			// Probability Distribution Function
			float pdf = D_GGX(dotNH, roughness) * dotNH / (4.0 * dotVH) + 0.0001;
			// Slid angle of current smple
			float omegaS = 1.0 / (float(numSamples) * pdf);
			// Solid angle of 1 pixel across all cube faces
			float omegaP = 4.0 * M_PI_F / (6.0 * width * height);
			// Biased (+1.0) mip level for better result
			float mipLevel = roughness == 0.0 ? 0.0 : max(0.5 * log2(omegaS / omegaP) + 1.0, 0.0f);
			color += TORQUE_TEXCUBELOD(environmentMap, float4(L, mipLevel)).rgb * dotNL;
			totalWeight += dotNL;
		}
	}

	return (color / totalWeight);
}

float4 main(ConnectData IN) : TORQUE_TARGET0
{
	float3 N = getCubeDir(face, IN.uv);
	//return float4(prefilterEnvMap(N), 1);
	return float4(1,1,1,1);
}