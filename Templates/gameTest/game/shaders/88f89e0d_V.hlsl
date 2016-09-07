//*****************************************************************************
// Torque -- HLSL procedural shader
//*****************************************************************************

// Dependencies:
#include "shaders/common/torque.hlsl"

// Features:
// Hardware Skinning
// Vert Position
// Base Texture
// Visibility
// HDR Output
// Forward Shaded Material
// Imposter
// Translucent

struct VertData
{
   float3 position        : POSITION;
   float tangentW        : TEXCOORD3;
   float3 normal          : NORMAL;
   float3 T               : TANGENT;
   float2 texCoord        : TEXCOORD0;
   uint4 blendIndices0   : BLENDINDICES0;
   float4 blendWeight0    : BLENDWEIGHT0;
   uint4 blendIndices1   : BLENDINDICES1;
   float4 blendWeight1    : BLENDWEIGHT1;
};


struct ConnectData
{
   float4 hpos            : POSITION;
   float2 out_texCoord    : TEXCOORD0;
};


//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
ConnectData main( VertData IN,
                  uniform float4x3 nodeTransforms[70] : register(C4),
                  uniform float4x4 modelview       : register(C0)
)
{
   ConnectData OUT;

   // Hardware Skinning
   float3 posePos = 0.0;
   float3 poseNormal = 0.0;
   float4x3 poseMat;
   float3x3 poseRotMat;
   int i;
   for (i=0; i<4; i++) {
      int poseIdx = int(IN.blendIndices0[i]);
      float poseWeight = IN.blendWeight0[i];
      poseMat = nodeTransforms[poseIdx];
      poseRotMat = (float3x3)poseMat;
      posePos += (mul(float4(IN.position, 1), poseMat)).xyz * poseWeight;
      poseNormal += (mul(IN.normal,poseRotMat) * poseWeight);
   }
   for (i=0; i<4; i++) {
      int poseIdx = int(IN.blendIndices1[i]);
      float poseWeight = IN.blendWeight1[i];
      poseMat = nodeTransforms[poseIdx];
      poseRotMat = (float3x3)poseMat;
      posePos += (mul(float4(IN.position, 1), poseMat)).xyz * poseWeight;
      poseNormal += (mul(IN.normal,poseRotMat) * poseWeight);
   }
   IN.position = posePos;
   IN.normal = normalize(poseNormal);
   
   // Vert Position
   OUT.hpos = mul(modelview, float4(IN.position.xyz,1));
   
   // Base Texture
   OUT.out_texCoord = (float2)IN.texCoord;
   
   // Visibility
   
   // HDR Output
   
   // Forward Shaded Material
   
   // Imposter
   
   // Translucent
   
   return OUT;
}
