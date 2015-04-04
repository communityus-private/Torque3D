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

#include "hlslCompat.glsl"
#include "shadergen:/autogenConditioners.h"
#include "torque.glsl"
#include "lighting.glsl"

in vec2 IN_uv0;
in vec3 IN_wsEyeRay;

uniform sampler3D lpvData;
uniform sampler2D prePassBuffer;
uniform sampler2D matInfoBuffer;
uniform mat4 invViewMat;
uniform vec3 eyePosWorld;
uniform vec3 volumeStart;
uniform vec3 volumeSize;
uniform float voxelSize;

out vec4 OUT_col;
void main()
{ 
   vec4 prepassSample = prepassUncondition( prePassBuffer, IN_uv0 );
   vec3 normal = prepassSample.rgb;
   float depth = prepassSample.a;

   // Use eye ray to get ws pos
   vec4 worldPos = vec4(eyePosWorld + IN_wsEyeRay * depth, 1.0f);

   // Need world-space normal.
   vec3 wsNormal = tMul(vec4(normal,0), invViewMat).rgb;

   // Calculate angle to potential light
   vec3 normalEyeRay = normalize(eyePosWorld + IN_wsEyeRay);
   vec3 reflected = normalize(reflect(normalEyeRay, wsNormal));

   vec4 matInfoSample = texture( matInfoBuffer, IN_uv0 );
   matInfoSample.b= (matInfoSample.b*0.8)+0.2; //need a minimum to avoid asymptotic results
   
   // Make 'raycast' into the grid in search of color!
   vec4 final_color = vec4(0, 0, 0, 0);
   vec3 curPos, volume_position;
   vec4 voxelcolor;
   float leng = pow(length(volumeSize),2);
   
   //make sure that each step leads to a new voxel
   float step = ( 0.5 * voxelSize );
   float coeff = 1/(voxelSize);
   
   for(float i = leng/(length(volumeSize)*2); i < leng; i++)
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

       voxelcolor = texture(lpvData, volume_position, matInfoSample.b);
           
       // if we want to add this voxels color
       if ( voxelcolor.a > 0 )
       {
            float dist = length( curPos - worldPos.rgb );
            vec3 reweightedColor = voxelcolor.rgb / ( 1 + dist );
            final_color += vec4( reweightedColor, voxelcolor.a );
			
			if ( final_color.a >= 1 )
            {
                break;
            }
       }
   }
   
   final_color.rgb = AL_CalcSpecular( vec3(1.0,1.0,1.0), final_color.rgb, reflected, wsNormal, normalEyeRay, matInfoSample.b, matInfoSample.a );
   OUT_col = vec4(saturate(final_color.rgb), 0.0);
}
