//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
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

#include "../../../gl/hlslCompat.glsl"
#include "shadergen:/autogenConditioners.h"
#include "../../../postFx/gl/postFX.glsl"
#include "../../../gl/torque.glsl"

uniform sampler2D colorBufferTex;
uniform sampler2D lightPrePassTex;
uniform sampler2D matInfoTex;
uniform sampler2D lightMapTex;

out vec4 OUT_col;

void main()
{        
   vec3 lightBuffer = texture( lightPrePassTex, uv0 ).rgb;
   vec3 colorBuffer = texture( colorBufferTex, uv0 ).rgb;
   vec3 lightMapBuffer = texture( lightMapTex, uv0 ).rgb;
   float metalness = texture( matInfoTex, uv0 ).a;
   
   vec3 diffuseColor = colorBuffer - (colorBuffer * metalness);
   vec3 reflectColor = mix( colorBuffer, lightMapBuffer, metalness);
   colorBuffer = diffuseColor + pow(reflectColor,vec3(2.2));
   colorBuffer *= lightBuffer;
   
   OUT_col = hdrEncode( vec4(colorBuffer, 1.0) );
}
