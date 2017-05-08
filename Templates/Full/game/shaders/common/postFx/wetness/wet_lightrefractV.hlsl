//-----------------------------------------------------------------------------
// Torque 3D
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "./../postFx.hlsl"
#include "./../../torque.hlsl"

uniform float time;
uniform float accumTime             : register(C1);

struct ConnectData
{
	float4 pos : POSITION0;
	float2 texCoord  : TEXCOORD0;
	float4 noiseCoord : TEXCOORD1;
};

               
void convToImageSpace(float4 position, out float4 oPosition, out float2 oTexCoord)
{
	position.xy = sign(position.xy);
	
	oPosition = float4(position.xy, 0, 1);
	oTexCoord.x = 0.5 * (1 + position.x) + 0.5 / 1024;
	oTexCoord.y = 0.5 * (1 - position.y) + 0.5 / 768;
}
				
ConnectData main (float4 position : POSITION)
{
	ConnectData OUT;
	convToImageSpace(position, OUT.pos, OUT.texCoord);

	OUT.noiseCoord.xy = OUT.texCoord + float2(sin(0.25f * accumTime * 0.5), -0.25f * accumTime);
	OUT.noiseCoord.zw = OUT.texCoord - float2(0.0f, accumTime * 0.5);
	
	return OUT;
}