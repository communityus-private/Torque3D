//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
// Portions Copyright Zefiros
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

#include "./torque.glsl"
#include "./brdf.glsl"
#ifndef TORQUE_SHADERGEN
#line 26
// These are the uniforms used by most lighting shaders.

uniform vec4 inLightPos[3];
uniform vec4 inLightInvRadiusSq;
uniform vec4 inLightColor[4];

#ifndef TORQUE_BL_NOSPOTLIGHT
   uniform vec4 inLightSpotDir[3];
   uniform vec4 inLightSpotAngle;
   uniform vec4 inLightSpotFalloff;
#endif

uniform vec4 ambient;
#define ambientCameraFactor 0.3
uniform float smoothness;
uniform float metalness;
uniform vec4 albedo;

#endif // !TORQUE_SHADERGEN

vec3 getDistanceVectorToPlane( vec3 origin, vec3 direction, vec4 plane )
{
   float denum = dot( plane.xyz, direction.xyz );
   float num = dot( plane, vec4( origin, 1.0 ) );
   float t = -num / denum;

   return direction.xyz * t;
}

vec3 getDistanceVectorToPlane( float negFarPlaneDotEye, vec3 direction, vec4 plane )
{
   float denum = dot( plane.xyz, direction.xyz );
   float t = negFarPlaneDotEye / denum;

   return direction.xyz * t;
}

void compute4Lights( vec3 wsView, 
                     vec3 wsPosition, 
                     vec3 wsNormal,
                     vec4 shadowMask,

                     #ifdef TORQUE_SHADERGEN
                     
                        vec4 inLightPos[3],
                        vec4 inLightInvRadiusSq,
                        vec4 inLightColor[4],
                        vec4 inLightSpotDir[3],
                        vec4 inLightSpotAngle,
                        vec4 inLightSpotFalloff,
                        float smoothness,
                        float metalness,
                        vec4 albedo,

                     #endif // TORQUE_SHADERGEN
                     
                     out vec4 outDiffuse,
                     out vec4 outSpecular )
{
   outDiffuse = vec4(0,0,0,0);
   outSpecular = vec4(0,0,0,0);
}

struct Surface
{
	vec3 P;				// world space position
	vec3 N;				// world space normal
	vec3 V;				// world space view vector
	vec4 baseColor;		// base color [0 -> 1] (rgba)
	float metalness;		// metalness [0:dielectric -> 1:metal]
	float roughness;		// roughness: [0:smooth -> 1:rough] (linear)
	float roughness_brdf; // roughness remapped from linear to BRDF
	float depth;         // depth: [0:near -> 1:far] (linear)
	float ao;            // ambient occlusion [0 -> 1]
	float matFlag;       // material flag - use getFlag to retreive 

	float NdotV;			// cos(angle between normal and view vector)
	vec3 f0;				// fresnel value (rgb)
	vec3 albedo;			// diffuse light absorbtion value (rgb)
	vec3 R;				// reflection vector
	vec3 F;				// fresnel term computed from f0, N and V
	void Update();
};

void Surface::Update()
{
	NdotV = abs(dot(N, V)) + 1e-5f; // avoid artifact

	albedo = baseColor.rgb * (1.0 - metalness);
	f0 = lerp(vec3(0.04), baseColor.rgb, metalness);
	R = -reflect(V, N);
	float f90 = saturate(50.0 * dot(f0, vec3(0.33,0.33,0.33)));
	F = F_Schlick(f0, f90, NdotV);
}
	
Surface createSurface(vec4 gbuffer0, sampler2D gbufferTex1, sampler2D gbufferTex2, in vec2 uv, in vec3 wsEyePos, in vec3 wsEyeRay, in mat4 invView)
{
	Surface surface;// = Surface();

	vec4 gbuffer1 = texture(gbufferTex1, uv);
	vec4 gbuffer2 = texture(gbufferTex2, uv);
	surface.depth = gbuffer0.a;
	surface.P = wsEyePos + wsEyeRay * surface.depth;
	surface.N = tMul(invView, vec4(gbuffer0.xyz,0)).xyz; //TODO move t3d to use WS normals
	surface.V = normalize(wsEyePos - surface.P);
	surface.baseColor = gbuffer1;
	const float minRoughness=1e-4;
	surface.roughness = clamp(1.0 - gbuffer2.b, minRoughness, 1.0); //t3d uses smoothness, so we convert to roughness.
	surface.roughness_brdf = surface.roughness * surface.roughness;
	surface.metalness = gbuffer2.a;
	surface.ao = gbuffer2.g;
	surface.matFlag = gbuffer2.r;
	surface.Update();
	return surface;
}

struct SurfaceToLight
{
	vec3 L;				// surface to light vector
	vec3 Lu;				// un-normalized surface to light vector
	vec3 H;				// half-vector between view vector and light vector
	float NdotL;			// cos(angle between N and L)
	float HdotV;			// cos(angle between H and V) = HdotL = cos(angle between H and L)
	float NdotH;			// cos(angle between N and H)

};

SurfaceToLight createSurfaceToLight(in Surface surface, in vec3 L)
{
	SurfaceToLight surfaceToLight;// = SurfaceToLight();
	surfaceToLight.Lu = L;
	surfaceToLight.L = normalize(L);
	surfaceToLight.H = normalize(surface.V + surfaceToLight.L);
	surfaceToLight.NdotL = saturate(dot(surfaceToLight.L, surface.N));
	surfaceToLight.HdotV = saturate(dot(surfaceToLight.H, surface.V));
	surfaceToLight.NdotH = saturate(dot(surfaceToLight.H, surface.N));
	return surfaceToLight;
}

vec3 BRDF_GetSpecular(in Surface surface, in SurfaceToLight surfaceToLight)
{
	float f90 = saturate(50.0 * dot(surface.f0, vec3(0.33,0.33,0.33)));
	vec3 F = F_Schlick(surface.f0, f90, surfaceToLight.HdotV);
	float Vis = V_SmithGGXCorrelated(surface.NdotV, surfaceToLight.NdotL, surface.roughness_brdf);
	float D = D_GGX(surfaceToLight.NdotH, surface.roughness_brdf);
	vec3 Fr = D * F * Vis / M_PI_F;
	return Fr;
}

vec3 BRDF_GetDiffuse(in Surface surface, in SurfaceToLight surfaceToLight)
{
   //getting some banding with disney method, using lambert instead - todo futher testing
	float Fd = 1.0 / M_PI_F;
   //energy conservation - remove this if reverting back to disney method
   vec3 kD = vec3(1.0) - surface.F;
	kD *= 1.0 - surface.metalness;
   vec3 diffuse = kD * surface.baseColor.rgb * Fd;
	return diffuse;
}

//attenuations functions from "moving frostbite to pbr paper"
//https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
float smoothDistanceAtt ( float squaredDistance , float invSqrAttRadius )
{
   float factor = squaredDistance * invSqrAttRadius ;
   float smoothFactor = saturate (1.0f - factor * factor );
   return sqr(smoothFactor);
}

float getDistanceAtt( vec3 unormalizedLightVector , float invSqrAttRadius )
{
   float sqrDist = dot ( unormalizedLightVector , unormalizedLightVector );
   float attenuation = 1.0 / (max ( sqrDist , 0.01*0.01) );
   attenuation *= smoothDistanceAtt ( sqrDist , invSqrAttRadius );
   return attenuation;
}

 float getSpotAngleAtt( vec3 normalizedLightVector , vec3 lightDir , vec2 lightSpotParams )
 {
   float cd = dot ( lightDir , normalizedLightVector );
   float attenuation = saturate ( ( cd - lightSpotParams.x ) / lightSpotParams.y );
   // smooth the transition
   return sqr(attenuation);
}

vec3 getDirectionalLight(in Surface surface, in SurfaceToLight surfaceToLight, vec3 lightColor, float lightIntensity, float shadow)
{
   vec3 factor = lightColor * max(surfaceToLight.NdotL, 0) * shadow * lightIntensity;
   vec3 diffuse = BRDF_GetDiffuse(surface,surfaceToLight) * factor;
   vec3 spec = BRDF_GetSpecular(surface,surfaceToLight) * factor;

   vec3 final = max(vec3(0.0f), diffuse + spec * surface.ao);
   return final;
}

vec3 getPunctualLight(in Surface surface, in SurfaceToLight surfaceToLight, vec3 lightColor, float lightIntensity, float radius, float shadow)
{
   float attenuation = getDistanceAtt(surfaceToLight.Lu, radius);
   vec3 factor = lightColor * max(surfaceToLight.NdotL, 0) * shadow * lightIntensity * attenuation;

   vec3 diffuse = BRDF_GetDiffuse(surface,surfaceToLight) * factor;
   vec3 spec = BRDF_GetSpecular(surface,surfaceToLight) * factor;

   vec3 final = max(vec3(0.0f), diffuse + spec * surface.ao * surface.F);
   return final;
}

float G1V(float dotNV, float k)
{
	return 1.0f/(dotNV*(1.0f-k)+k);
}

vec3 directSpecular(vec3 N, vec3 V, vec3 L, float roughness, float F0)
{
	float alpha = roughness*roughness;

	//TODO don't need to calculate all this again timmy!!!!!!
    vec3 H = normalize(V + L);
    float dotNL = clamp(dot(N,L), 0.0, 1.0);
    float dotNV = clamp(dot(N,V), 0.0, 1.0);
    float dotNH = clamp(dot(N,H), 0.0, 1.0);
	float dotHV = clamp(dot(H,V), 0.0, 1.0);
	float dotLH = clamp(dot(L,H), 0.0, 1.0);

	float F, D, vis;

	// D
	float alphaSqr = alpha*alpha;
	float pi = 3.14159f;
	float denom = dotNH * dotNH *(alphaSqr-1.0) + 1.0f;
	D = alphaSqr/(pi * denom * denom);

	// F
	float dotLH5 = pow(1.0f-dotLH,5);
	F = F0 + (1.0-F0)*(dotLH5);

	// V
	float k = alpha/2.0f;
	vis = G1V(dotNL,k)*G1V(dotNV,k);

	float specular = dotNL * D * F * vis;
	return vec3(specular,specular,specular);
}