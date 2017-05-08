//-----------------------------------------------------------------------------
// Torque 3D
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "../../ShaderModelAutoGen.hlsl"
#include "./../postFx.hlsl"
#include "shaders/common/torque.hlsl"

TORQUE_UNIFORM_SAMPLER2D(deferredTex        ,0);
TORQUE_UNIFORM_SAMPLER2D(directLightingBuffer   ,1);
TORQUE_UNIFORM_SAMPLER2D(wetMap            ,2);
TORQUE_UNIFORM_SAMPLER2D(colorBufferTex    ,3);

uniform float accumTime             : register(C1);

uniform float3    eyePosWorld;

//----------------------------------------------
// Downhill Shader
//----------------------------------------------
float4 main( PFXVertToPix IN ) : TORQUE_TARGET0
{
   //return float4(0, 0, 0, 0);
   // Define variables
   float4 color;
   float3 lightcolor, wetNorm1, wetNorm2,
          wetness;   
   float nl_Att, specular, wetSpec1,
         wetSpec2, animate;
   
   // Get the prepass texture for uv channel 0
   float4 prepass = TORQUE_DEFERRED_UNCONDITION( deferredTex, IN.uv0 );
   float3 normal = prepass.rgb; // Get the normal from the prepass texture
   float depth = prepass.a; // Get the depth form the prepass
   
   // Early out if too far away
   if ( depth > 0.99999999 )
      return float4(0, 0, 0, 0);
      
   // Get world space normals
   float ang = dot(float(normal.x), float(normal.z));
   
   animate = 1;
   if(ang == 0 || ang <= 0.2)
      animate = 0;
   
   // Animate UVs
   float3 wetUV = eyePosWorld + ( IN.wsEyeRay * depth );
   if(animate == 1)
   {
      wetUV = float3(wetUV.x,
                     wetUV.y,
                     wetUV.z + accumTime); // Animate our UV
      wetness = TORQUE_TEX2D( wetMap, (wetUV.xz + wetUV.yz) * 0.2 ).rgb;
   }
   else
   {
      wetUV = float3(wetUV.x,
                     wetUV.y,
                     wetUV.z + accumTime * 0.1); // Animate our UV
      wetness = TORQUE_TEX2D( wetMap, (wetUV.xz + wetUV.yz + wetUV.xy) * 0.2 ).rgb;
   }
   
   // Get the wetness normal map
   wetNorm1 = normalize( wetness );
   wetNorm1.xy *= 3.5;
   wetNorm1 = normalize( wetNorm1 );
      
   // Modifies specularity
   wetSpec1 = saturate(dot(wetNorm1, normalize(float3(IN.wsEyeRay))));
   wetSpec1 = pow( wetSpec1, 1 );
   
   // Modifies specularity
   wetSpec2 = saturate(dot(wetNorm1, normalize(float3(-IN.wsEyeRay))));
   wetSpec2 = pow( wetSpec2, 1 );
   
   // Get the light information of the directLightingBuffer
   float4 directLighting = TORQUE_TEX2D( directLightingBuffer, IN.uv0 ); //shadowmap*specular
   lightcolor = directLighting.rgb;
   specular = directLighting.a;
   color = TORQUE_TEX2D( colorBufferTex, IN.uv0 );
   
   float wetsum = (wetSpec1 + wetSpec2);
   float3 diffuseColor = color.rgb - (color.rgb * 0.92 * wetsum);
   lightcolor.rgb = lerp( 0.08 * lightcolor.rgb, diffuseColor, wetsum );
   color.rgb =  diffuseColor + lightcolor.rgb;   
   
   return hdrEncode( color ); 
}