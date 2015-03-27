//-----------------------------------------------------------------------------
// Copyright (c) 2015 Andrew Mac
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

#include "shadergen:/autogenConditioners.h"
#include "torque.hlsl"
#include "lighting.hlsl"

struct Conn
{
   float4 position : POSITION;
   float2 uv0      : TEXCOORD0;
   float3 wsEyeRay : TEXCOORD1;
};

uniform sampler3D lpvData : register(S0);
uniform sampler2D prePassBuffer : register(S1);
uniform sampler2D matInfoBuffer : register(S2);
uniform float4x4 invViewMat;
uniform float3 eyePosWorld;
uniform float3 volumeStart;
uniform float3 volumeSize;
uniform float voxelSize;
 
float4 main( Conn IN ) : COLOR0
{ 
   float4 prepassSample = prepassUncondition( prePassBuffer, IN.uv0 );
   float3 normal = prepassSample.rgb;
   float depth = prepassSample.a;

   // Use eye ray to get ws pos
   float4 worldPos = float4(eyePosWorld + IN.wsEyeRay * depth, 1.0f);

   // Need world-space normal.
   float3 wsNormal = mul(normal, invViewMat);

   // Calculate angle to potential light
   float3 normalEyeRay = normalize(eyePosWorld + IN.wsEyeRay);
   float3 reflected = normalize(reflect(normalEyeRay, wsNormal));

   float4 matInfoSample = tex2D( matInfoBuffer, IN.uv0 );
   matInfoSample.b= (matInfoSample.b*0.8)+0.2; //need a minimum to avoid asymptotic results
   
   // Make 'raycast' into the grid in search of color!
   float4 final_color = float4(0, 0, 0, 0);
   float3 curPos, volume_position;
   float4 voxelcolor;
   float leng = pow(length(volumeSize),2);
   
   //make sure that each step leads to a new voxel
   float step = ( 0.5 * voxelSize );
   float coeff = 1/(voxelSize);
   
   for(int i = leng/(length(volumeSize)*2); i < leng; i++)
   {
           curPos = worldPos.rgb + (reflected * i * step );
		   volume_position = round((curPos - volumeStart)*coeff)/coeff;
           volume_position /= volumeSize;
       if ( volume_position.x < 0 || volume_position.x > 1 ||
            volume_position.y < 0 || volume_position.y > 1 ||
            volume_position.z < 0 || volume_position.z > 1 )
       {
            break; 
       }

       voxelcolor = tex3Dlod(lpvData, float4(volume_position,matInfoSample.b));
           
       // if we want to add this voxels color
       if ( voxelcolor.a > 0 )
       {
            float dist = length( curPos - worldPos.rgb );
            float3 reweightedColor = voxelcolor.rgb;// / ( 1 + ( dist ) );
            final_color += float4( reweightedColor, voxelcolor.a );
			
			if ( final_color.a >= 1 )
            {
                break;
            }
       }
   }
      
   final_color = pow(final_color,2.2); //linearize diffused reflections 
   
   final_color.rgb = AL_CalcSpecular( float3(1,1,1), final_color.rgb, reflected, wsNormal, normalEyeRay, matInfoSample.b, matInfoSample.a );
   return float4(saturate(final_color.rgb), 0.0);
}
