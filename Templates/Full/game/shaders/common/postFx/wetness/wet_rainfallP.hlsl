//-----------------------------------------------------------------------------
// Torque 3D
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "../../ShaderModelAutoGen.hlsl"
#include "./../postFx.hlsl"
#include "shaders/common/torque.hlsl"

TORQUE_UNIFORM_SAMPLER2D(deferredTex        ,0);
TORQUE_UNIFORM_SAMPLER2D(directLightingBuffer   ,1);
TORQUE_UNIFORM_SAMPLER2D(rainfall          ,2);

uniform float accumTime             : register(C1);

uniform float3    eyePosWorld;

//----------------------------------------------
// Downhill Shader
//----------------------------------------------
float4 main( PFXVertToPix IN ) : TORQUE_TARGET0
{
   //return float4(0, 0, 0, 0);
   // Define an output variable
   float4 color;
   float3 lightcolor;   
   float nl_Att, specular;
   
   // Get the speculariry of the object we're interacting with
   float4 directLighting = TORQUE_TEX2D( directLightingBuffer, IN.uv0 ); //shadowmap*specular
   lightcolor = directLighting.rgb;
   specular = directLighting.a;
   
   // Get the UV we want to apply the out color texture on
   float2 wetUV = IN.uv0;
   
   // Animate UVs
   wetUV = float2(wetUV.x + accumTime, wetUV.y - accumTime * 2); // Animate our UV
   
   // Get the primary wetness normal
   float3 raincolor = TORQUE_TEX2D( rainfall, wetUV * 2 ).rgb;
   
   // Apply the wetness specularity to our output color
   color.rgb = raincolor;
   color.rgb *= lightcolor; // Add light color to the specular
   
   // Strenghten/wanish the output texture
   color.rgb *= 0.5;
   color.a = 1.0; // Assign an alpah to the output texture
   
   return hdrEncode( color ); 
}