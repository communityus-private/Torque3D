//-----------------------------------------------------------------------------
// Torque 3D
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "shadergen:/autogenConditioners.h"
#include "./../postFx.hlsl"
#include "./../../torque.hlsl"

uniform sampler2D prepassTex        : register(S0);
uniform sampler2D lightPrePassTex   : register(S1);
uniform sampler2D splashNormalMap   : register(S2);

uniform float accumTime             : register(C1);

uniform float3    eyePosWorld;
uniform float     splashRandom;
uniform float     splashAlpha;

//----------------------------------------------
// Splash Shader
//----------------------------------------------
float4 main( PFXVertToPix In ) : COLOR
{
   //return float4(0, 0, 0, 0);
   // Define an output variable
   float4 color;
   float3 lightcolor, splashNorm;   
   float nl_Att, specular, splashSpec1, splashSpec2;
   
   // Get the prepass texture
   float4 prepass = prepassUncondition( prepassTex, In.uv0 );
   float3 normal = prepass.rgb; // Get the normals
   float depth = prepass.a; // Get the depth
      
   // Early out if too far away
   if ( depth > 0.99999999 )
      return float4(0, 0, 0, 0);
      
   // Get the light information of the lightPrePassTex
   lightinfoUncondition( tex2D( lightPrePassTex, In.uv0 ), lightcolor, nl_Att, specular );
   
   // Get the UV we want to apply out texture on (rainUV)
   // and assign our rain splash texture to a float3 (splashColor)
   float3 rainUV = eyePosWorld + ( In.wsEyeRay * depth );
   
   // Filter out walls and similar
   float ang = dot(float(normal.x), float(normal.z));
   if(ang != 0)
   {
      ang = normal.z;
      if(ang <= 0.9)
         return float4(0, 0, 0, 0);
   }
      
   
   // Randomize splahes
   if(splashRandom == 0)
      rainUV = float3(rainUV.x - accumTime * 2,
                      rainUV.y - accumTime * 2,
                      rainUV.z);
   else if(splashRandom == 1)
      rainUV = float3(rainUV.x + accumTime * 2,
                      rainUV.y - accumTime * 2,
                      rainUV.z);
   else if(splashRandom == 2)
      rainUV = float3(rainUV.x - accumTime * 2,
                      rainUV.y + accumTime * 2,
                      rainUV.z);
   else if(splashRandom == 3)
      rainUV = float3(rainUV.x + accumTime * 2,
                      rainUV.y + accumTime * 2,
                      rainUV.z);
   
   // Get splash normals from normal map
   float3 splashNormal = tex2D( splashNormalMap, rainUV.xy ).rgb;
   
   // Get the splash normals
   splashNorm = normalize( splashNormal );
   splashNorm.xy *= 3.5;
   splashNorm = normalize( splashNorm );
   
   // Modifies specularity
   splashSpec1 = saturate(dot(splashNorm, normalize(float3(In.wsEyeRay))));
   splashSpec1 = pow( splashSpec1, 1 ); // Increase/decrease specularity power
   
   // Modifies specularity
   splashSpec2 = saturate(dot(splashNorm, normalize(float3(-In.wsEyeRay))));
   splashSpec2 = pow( splashSpec2, 1 ); // Increase/decrease specularity power
   
   // Apply the wetness specularity to our output color
   color.rgb = specular;
   color.rgb *= splashSpec1 + splashSpec2*5; //BUG: splashSpec2 is less visible
   color.rgb *= lightcolor; // Add light color to the specular
   color.a = splashAlpha; // Assign an alpah to the output texture
   
   return color;
}