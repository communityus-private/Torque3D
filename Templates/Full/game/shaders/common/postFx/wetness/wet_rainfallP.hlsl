//-----------------------------------------------------------------------------
// Torque 3D
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "shadergen:/autogenConditioners.h"
#include "./../postFx.hlsl"
#include "./../../torque.hlsl"

uniform sampler2D prepassTex        : register(S0);
uniform sampler2D lightPrePassTex   : register(S1);
uniform sampler2D rainfall          : register(S2);

uniform float accumTime             : register(C1);

uniform float3    eyePosWorld;

//----------------------------------------------
// Downhill Shader
//----------------------------------------------
float4 main( PFXVertToPix In ) : COLOR
{
   //return float4(0, 0, 0, 0);
   // Define an output variable
   float4 color;
   float3 lightcolor;   
   float nl_Att, specular;
   
   // Get the speculariry of the object we're interacting with
   lightinfoUncondition( tex2D( lightPrePassTex, In.uv0 ), lightcolor, nl_Att, specular );
   
   // Get the UV we want to apply the out color texture on
   float2 wetUV = In.uv0;
   
   // Animate UVs
   wetUV = float2(wetUV.x + accumTime, wetUV.y - accumTime * 2); // Animate our UV
   
   // Get the primary wetness normal
   float3 raincolor = tex2D( rainfall, wetUV * 2 ).rgb;
   
   // Apply the wetness specularity to our output color
   color.rgb = raincolor;
   color.rgb *= lightcolor; // Add light color to the specular
   
   // Strenghten/wanish the output texture
   color.rgb *= 0.5;
   color.a = 1.0; // Assign an alpah to the output texture
   
   return hdrEncode( color ); 
}