#include "../../shaderModelAutoGen.hlsl"

#include "farFrustumQuad.hlsl"
#include "lightingUtils.hlsl"
#include "../../lighting.hlsl"
#include "../../torque.hlsl"

struct ConvexConnectP
{
   float4 pos : TORQUE_POSITION;
   float4 wsEyeDir : TEXCOORD0;
   float4 ssPos : TEXCOORD1;
   float4 vsEyeDir : TEXCOORD2;
};

TORQUE_UNIFORM_SAMPLER2D(deferredBuffer, 0);
TORQUE_UNIFORM_SAMPLER2D(matInfoBuffer, 1);
TORQUE_UNIFORM_SAMPLERCUBE(cubeMap, 2);
TORQUE_UNIFORM_SAMPLERCUBE(irradianceCubemap, 3);
TORQUE_UNIFORM_SAMPLER2D(BRDFTexture, 4);

uniform float4 rtParams0;

uniform float3 probeWSPos;
uniform float3 probeLSPos;
uniform float4 vsFarPlane;

uniform float  radius;
uniform float2 attenuation;

uniform float4x4 invViewMat;

uniform float3 eyePosWorld;
uniform float3 bbMin;
uniform float3 bbMax;

uniform float useSphereMode;

float3 iblSpecular(float3 v, float3 n, float roughness)
{
   float3 R = reflect(v, n);
   const float MAX_REFLECTION_LOD = 6.0;
   float3 prefilteredColor = TORQUE_TEXCUBELOD(cubeMap, float4(R, roughness * MAX_REFLECTION_LOD)).rgb;
   float2 envBRDF = TORQUE_TEX2D(BRDFTexture, float2(max(dot(n, v), 0.0), roughness)).rg;
   //return prefilteredColor * (envBRDF.x + envBRDF.y);
   return prefilteredColor;
}

// Box Projected IBL Lighting
// Based on: http://www.gamedev.net/topic/568829-box-projected-cubemap-environment-mapping/
// and https://seblagarde.wordpress.com/2012/09/29/image-based-lighting-approaches-and-parallax-corrected-cubemap/
float3 boxProject(float3 wsPosition, float3 reflectDir, float3 boxWSPos, float3 boxMin, float3 boxMax)
{ 
    float3 nrdir = normalize(reflectDir);
	float3 offset = wsPosition;
    float3 plane1vec = (boxMax - offset) / nrdir;
    float3 plane2vec = (boxMin - offset) / nrdir;
	
	float3 furthestPlane = max(plane1vec, plane2vec);
	float dist = min(min(furthestPlane.x, furthestPlane.y), furthestPlane.z);
    float3 posonbox = offset + nrdir * dist;

    return posonbox - boxWSPos;
}

float3 iblBoxDiffuse(float3 normal,
					float3 wsPos, 
                    TORQUE_SAMPLERCUBE(irradianceCube), 
                    float3 boxPos,
                    float3 boxMin,
                    float3 boxMax)
{
    // Irradiance (Diffuse)
    float3 cubeN = normalize(normal);
    float3 irradiance = TORQUE_TEXCUBE(irradianceCube, cubeN).xyz;

    return irradiance;
}

float3 iblBoxSpecular(float3 normal,
					float3 wsPos, 
					float roughness,
                    float3 surfToEye, 
                    TORQUE_SAMPLER2D(brdfTexture), 
                    TORQUE_SAMPLERCUBE(radianceCube),
                    float3 boxPos,
                    float3 boxMin,
                    float3 boxMax)
{
    float ndotv = clamp(dot(normal, surfToEye), 0.0, 1.0);

    // BRDF
    float2 brdf = TORQUE_TEX2D(brdfTexture, float2(roughness, ndotv)).xy;

    // Radiance (Specular)
    float lod = roughness * 6.0;
    float3 r = reflect(surfToEye, normal);
    float3 cubeR = normalize(r);
    cubeR = boxProject(wsPos, cubeR, boxPos, boxMin, boxMax);
	
    float3 radiance = TORQUE_TEXCUBELOD(radianceCube, float4(cubeR, lod)).xyz * (brdf.x + brdf.y);
    
    return radiance;
}

struct PS_OUTPUT
{
    float4 diffuse: TORQUE_TARGET0;
    float4 spec: TORQUE_TARGET1;
};

PS_OUTPUT main( ConvexConnectP IN )
{ 
    PS_OUTPUT Output = (PS_OUTPUT)0;

    // Compute scene UV
    float3 ssPos = IN.ssPos.xyz / IN.ssPos.w; 

    float2 uvScene = getUVFromSSPos( ssPos, rtParams0 );

    // Matinfo flags
    float4 matInfo = TORQUE_TEX2D( matInfoBuffer, uvScene ); 

    // Sample/unpack the normal/z data
    float4 deferredSample = TORQUE_DEFERRED_UNCONDITION( deferredBuffer, uvScene );
    float3 normal = deferredSample.rgb;
    float depth = deferredSample.a;
    if (depth>0.9999)
          clip(-1);

    // Need world-space normal.
    float3 wsNormal = mul(float4(normal, 1), invViewMat).rgb;

    float3 eyeRay = getDistanceVectorToPlane( -vsFarPlane.w, IN.vsEyeDir.xyz, vsFarPlane );
    float3 viewSpacePos = eyeRay * depth;

    float3 wsEyeRay = mul(float4(eyeRay, 1), invViewMat).rgb;

    // Use eye ray to get ws pos
    float3 worldPos = float3(eyePosWorld + wsEyeRay * depth);
		  
    float blendVal = 1.0;
	
	//clip bounds and (TODO properly: set falloff)
	if(useSphereMode)
    {
        // Build light vec, get length, clip pixel if needed
        float3 lightVec = probeLSPos - viewSpacePos;
        float lenLightV = length( lightVec );
        clip( radius - lenLightV );

        // Get the attenuated falloff.
        float atten = attenuate( float4(1,1,1,1), attenuation, lenLightV );
        clip( atten - 1e-6 );

        // Normalize lightVec
        lightVec = lightVec /= lenLightV;

        // If we can do dynamic branching then avoid wasting
        // fillrate on pixels that are backfacing to the light.
        float nDotL = abs(dot( lightVec, normal ));

        float Sat_NL_Att = saturate( nDotL * atten );
		
        blendVal = Sat_NL_Att;
    }
    else
    {
       //Try to clip anything that falls outside our box as well
       //TODO: Make it support rotated boxes as well
       if(worldPos.x > bbMax.x || worldPos.y > bbMax.y || worldPos.z > bbMax.z ||
          worldPos.x < bbMin.x || worldPos.y < bbMin.y || worldPos.z < bbMin.z)
          clip(-1);
    }
	
	//render into the bound space defined above
	float3 surfToEye = normalize(worldPos.xyz-eyePosWorld.xyz);
	Output.diffuse = float4(iblBoxDiffuse(wsNormal, worldPos, TORQUE_SAMPLERCUBE_MAKEARG(irradianceCubemap), probeWSPos, bbMin, bbMax), blendVal);
	Output.spec = float4(iblBoxSpecular(wsNormal, worldPos, 1.0 - matInfo.b, surfToEye, TORQUE_SAMPLER2D_MAKEARG(BRDFTexture), TORQUE_SAMPLERCUBE_MAKEARG(cubeMap), probeWSPos, bbMin, bbMax), blendVal);
	
	
	//TODO properly: filter out pixels projected uppon by probes behind walls by looking up the depth stored in the probes cubemap alpha
	//and comparing legths
	
	float3 fromlightVec = probeLSPos-viewSpacePos;
	float lenLightV = length( fromlightVec );
	fromlightVec = normalize(fromlightVec);
	float nDotL = dot( fromlightVec, normal );
	float3 reflectionVec = reflect(IN.wsEyeDir, float4(wsNormal,nDotL)).xyz;
	
	float depthRef = TORQUE_TEXCUBE(cubeMap, reflectionVec).a;
	//if (lenLightV>depthRef)
	//clip(-1);
	Output.spec = float4(Output.spec.rgb,blendVal);
	return Output;
}
