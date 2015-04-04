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

in vec4 IN_position;
in vec2 IN_uv0;
in vec3 IN_wsEyeRay;

uniform sampler3D lpvData;
uniform sampler2D prePassBuffer;

#ifdef USE_SSAO_MASK
uniform sampler2D ssaoMask;
#endif

uniform vec3 eyePosWorld;
uniform vec3 volumeStart;
uniform vec3 volumeSize;

out vec4 OUT_col;

void main()
{ 
   vec4 prepassSample = prepassUncondition( prePassBuffer, IN_uv0 );
   vec3 normal = prepassSample.rgb;
   float depth = prepassSample.a;

   // Use eye ray to get ws pos
   vec4 worldPos = vec4(eyePosWorld + IN_wsEyeRay * depth, 1.0f);

   vec3 volume_position = (worldPos.xyz - volumeStart) / volumeSize;
   if ( volume_position.x < 0 || volume_position.x > 1 || 
        volume_position.y < 0 || volume_position.y > 1 || 
        volume_position.z < 0 || volume_position.z > 1 )
   {
        OUT_col = vec4(0.0, 0.0, 0.0, 0.0);
		return;
   }
   
    vec4 color = texture(lpvData, volume_position);
    #ifdef USE_SSAO_MASK
		float ao = 1.0 - tex2D( ssaoMask, IN_uv0 ).r;
		color = color * ao;
	#endif
	OUT_col = vec4(color.rgb, 0.0);
}
