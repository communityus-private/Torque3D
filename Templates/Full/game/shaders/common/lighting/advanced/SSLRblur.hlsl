// Originating work by By Morgan McGuire and Michael Mara at Williams College 2014
// Released as open source under the BSD 2-Clause License
// http://opensource.org/licenses/BSD-2-Clause
//
// Copyright (c) 2014, Morgan McGuire and Michael Mara
// All rights reserved.
//
// From McGuire and Mara, Efficient GPU Screen-Space Ray Tracing,
// Journal of Computer Graphics Techniques, 2014
//
// This software is open source under the "BSD 2-clause license":
//
// Redistribution and use in source and binary forms, with or
// without modification, are permitted provided that the following
// conditions are met:
//
// 1. Redistributions of source code must retain the above
// copyright notice, this list of conditions and the following
// disclaimer.
//
// 2. Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following
// disclaimer in the documentation and/or other materials provided
// with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
// CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
// USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
// AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
// IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
/**
 * The ray tracing step of the SSLR implementation.
 * Modified version of the work stated above.
 */
/**
 * The Convolution shader body.
 * Requires either CONVOLVE_VERTICAL or CONVOLVE_HORIZONTAL
 * to be defined.
 */
#ifndef CONVOLUTIONPS_HLSLI
#define CONVOLUTIONPS_HLSLI

#include "../../shaderModelAutoGen.hlsl"
#include "../../postfx/postFx.hlsl"
#include "shaders/common/torque.hlsl"
#include "SSLRConstantBuffer.hlsl"

TORQUE_UNIFORM_SAMPLER2D(colorBufferTex,0);

#if CONVOLVE_HORIZONTAL
static const int2 offsets[7] = {{-3, 0}, {-2, 0}, {-1, 0}, {0, 0}, {1, 0}, {2, 0}, {3, 0}};
#elif CONVOLVE_VERTICAL
static const int2 offsets[7] = {{0, -3}, {0, -2}, {0, -1}, {0, 0}, {0, 1}, {0, 2}, {0, 3}};
#endif
static const float weights[7] = {0.001f, 0.028f, 0.233f, 0.474f, 0.233f, 0.028f, 0.001f};

float4 main(PFXVertToPix IN) : TORQUE_TARGET0
{
    float4 color = float4(0.0f, 0.0f, 0.0f, 1.0f);
    [unroll]
    for(uint i = 0u; i < 7u; ++i)
    {
        color += TORQUE_TEX2D(colorBufferTex, IN.uv0+offsets[i]) * weights[i];
    }
    return float4(color.rgb, 1.0f);
}

#endif