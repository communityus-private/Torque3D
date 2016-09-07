//*****************************************************************************
// Torque -- HLSL procedural shader
//*****************************************************************************

// Dependencies:
#include "shaders/common/torque.hlsl"

// Features:
// Hardware Skinning
// Vert Position
// Base Texture
// Visibility
// HDR Output
// Forward Shaded Material
// Imposter
// Translucent

struct ConnectData
{
   float2 texCoord        : TEXCOORD0;
};


struct Fragout
{
   float4 col : COLOR0;
};


//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Fragout main( ConnectData IN,
              uniform sampler2D diffuseMap      : register(S0),
              uniform float     visibility      : register(C0)
)
{
   Fragout OUT;

   // Hardware Skinning
   
   // Vert Position
   
   // Base Texture
   float4 diffuseColor = tex2D(diffuseMap, IN.texCoord);
   OUT.col = diffuseColor;
   
   // Visibility
   OUT.col.a *= visibility;
   
   // HDR Output
   OUT.col = hdrEncode( OUT.col );
   
   // Forward Shaded Material
   
   // Imposter
   
   // Translucent
   

   return OUT;
}
