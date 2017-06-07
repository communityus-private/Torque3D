#include "./postFx.hlsl"

#if WET == 0
static const float4 color = float4(0.0, 1.0, 0.0, 1.0);//rough
#elif WET == 1
static const float4 color = float4(0.0, 1.0, 1.0, 1.0);//smooth
#endif

float4 main(PFXVertToPix IN) : TORQUE_TARGET0
{
	return color;
}