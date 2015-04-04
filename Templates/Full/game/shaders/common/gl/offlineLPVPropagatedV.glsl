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

// This is the shader input vertex structure.
in vec4 IN_position;
in vec2 IN_uv0;
in vec3 IN_wsEyeRay;

// This is the shader output data.
out vec2 OUT_uv0;
out vec3 OUT_wsEyeRay;

// Render Target Paramaters (width/height of depth/light buffers)
uniform vec4 rtParams0;
uniform mat4  modelview;

void main()	         
{   
   OUT_uv0 = viewportCoordToRenderTarget( IN_uv0, rtParams0 );
   OUT_wsEyeRay = IN_wsEyeRay;
   gl_Position = IN_position;
   correctSSP(gl_Position);
}

