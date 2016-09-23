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

<<<<<<< HEAD
=======
#include "../../shaderModelAutoGen.hlsl"
>>>>>>> d93423ad510ce66434b84ece061254124d2f7db1
#include "../../postfx/postFx.hlsl"
#include "../../shaderModelAutoGen.hlsl"

<<<<<<< HEAD
TORQUE_UNIFORM_SAMPLER2D(lightPrePassTex, 0);


float4 main( PFXVertToPix IN ) : TORQUE_TARGET0
{   
   float3 lightcolor;   
   float nl_Att, specular;   
   lightinfoUncondition( TORQUE_TEX2D( lightPrePassTex, IN.uv0 ), lightcolor, nl_Att, specular );   
   return float4( lightcolor, 1.0 );   
=======
TORQUE_UNIFORM_SAMPLER2D(lightPrePassTex,0);

float4 main( PFXVertToPix IN ) : TORQUE_TARGET0
{   
   float4 lightColor = TORQUE_TEX2D( lightPrePassTex, IN.uv0 );    
   return float4( lightColor.rgb, 1.0 );   
>>>>>>> d93423ad510ce66434b84ece061254124d2f7db1
}
