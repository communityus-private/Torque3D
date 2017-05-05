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

uniform float4 rtParams0;
uniform float4 lightColor;

uniform float  lightBrightness;
uniform float3 lightPosition;

uniform float4 vsFarPlane;

uniform float  lightRange;
uniform float2 lightAttenuation;

uniform float4x4 invViewMat;

uniform float3 eyePosWorld;
uniform float3 volumeStart;
uniform float3 volumeSize;
uniform float3 volumePosition;

uniform float4 AmbientColor;

uniform float Intensity;
uniform float cubeMips;

uniform float useSphereMode;

float4 main( ConvexConnectP IN ) : TORQUE_TARGET0
{ 
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
        return float4(0,0,0,0);

    // Need world-space normal.
    float3 wsNormal = mul(float4(normal, 1), invViewMat).rgb;

    float4 color = float4(1, 1, 1, 1);
    float4 ref = float4(0,0,0,0);
    float alpha = 0;

    // Use eye ray to get ws pos
    float4 worldPos = float4(eyePosWorld + IN.wsEyeDir.rgb * depth, 1.0f);

    if(useSphereMode)
    {

        // Eye ray - Eye -> Pixel
        float3 eyeRay = getDistanceVectorToPlane( -vsFarPlane.w, IN.vsEyeDir.xyz, vsFarPlane );
        float3 viewSpacePos = eyeRay * depth;
            
        // Build light vec, get length, clip pixel if needed
        float3 lightVec = lightPosition - viewSpacePos;
        float lenLightV = length( lightVec );
        clip( lightRange - lenLightV );

        // Get the attenuated falloff.
        float atten = attenuate( lightColor, lightAttenuation, lenLightV );
        clip( atten - 1e-6 );

        // Normalize lightVec
        lightVec /= lenLightV;

        // If we can do dynamic branching then avoid wasting
        // fillrate on pixels that are backfacing to the light.
        float nDotL = dot( lightVec, normal );

        float Sat_NL_Att = saturate( nDotL * atten ) * lightBrightness;

        float3 reflectionVec = reflect(IN.wsEyeDir, float4(normalize(wsNormal),nDotL)).rgb;
        //float smoothness = min((1.0 - matInfo.b)*11.0 + 1.0, 8.0);//bump up to 8 for finalization
        ref = float4(reflectionVec, 16);

        alpha = Sat_NL_Att;
    }
    else
    {
        float3 wPos = worldPos.rgb;

        //box proj?
        float3 rdir = reflect(IN.wsEyeDir.rgb, normalize(wsNormal));

        //
        float3 nrdir = normalize(rdir);
        float3 rbmax = ((volumePosition + volumeSize) - wPos) / nrdir;
        float3 rbmin = (volumeStart - wPos) / nrdir;

        float3 rbminmax = (nrdir>0.0) ? rbmax : rbmin;
        float fa = min(min(rbminmax.x,rbminmax.y),rbminmax.z);

        float3 posOnBox = wPos + nrdir*fa;
        rdir = posOnBox - volumePosition;

        //float smoothness = min((1.0 - matInfo.b)*11.0 + 1.0, 8.0);//bump up to 8 for finalization
        ref = float4(rdir, 16);

        alpha = 1;
    }

    color = TORQUE_TEXCUBELOD(cubeMap, ref);
    color *= Intensity;

    return float4(color.rgb, alpha);

    /*float3 wPos = worldPos.rgb;

    //box proj?
    float3 rdir = reflect(IN.wsEyeDir.rgb, normalize(wsNormal));

    //
    float3 nrdir = normalize(rdir);
    float3 rbmax = ((volumePosition + volumeSize) - wPos) / nrdir;
    float3 rbmin = (volumeStart - wPos) / nrdir;

    float3 rbminmax = (nrdir>0.0) ? rbmax : rbmin;
    float fa = min(min(rbminmax.x,rbminmax.y),rbminmax.z);

    float3 posOnBox = wPos + nrdir*fa;
    rdir = posOnBox - volumePosition;

    color = TORQUE_TEXCUBELOD(cubeMap, float4(rdir,1));*/

    //sphere projection?
    /*float3 EnvMapOffset = float3(1 / volumeSize.x, 1 / volumeSize.x, 1 / volumeSize.x);
    float3 reflectionVec = reflect(IN.wsEyeDir, float4(normalize(wsNormal),nDotL)).rgb;

    float3 ReflDirectionWS = EnvMapOffset * (worldPos.rgb - volumePosition) + reflectionVec;

    //float3 reflectionVec = reflect(rDir, float4(normalize(wsNormal),nDotL)).rgb;
    float smoothness = min((1.0 - matInfo.b)*11.0 + 1.0, 6.0);//bump up to 6 for finalization
    float4 ref = float4(ReflDirectionWS, smoothness);

    color = TORQUE_TEXCUBELOD(cubeMap, ref);*/

    //classic maths
    /*float3 reflectionVec = reflect(IN.wsEyeDir, float4(normalize(wsNormal),nDotL)).rgb;
    float smoothness = min((1.0 - matInfo.b)*11.0 + 1.0, 12.0);//bump up to 8 for finalization
    float4 ref = float4(reflectionVec, smoothness);
    color = TORQUE_TEXCUBELOD(cubeMap, ref);
    color.a = 1;
    color *= Intensity * 3;*/
}
