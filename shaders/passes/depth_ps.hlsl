/*
* Copyright (c) 2014-2024, NVIDIA CORPORATION. All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

#pragma pack_matrix(row_major)

#include <donut/shaders/depth_cb.h>
#include <donut/shaders/material_cb.h>
#include <donut/shaders/scene_material.hlsli>
#include <donut/shaders/binding_helpers.hlsli>

DECLARE_CBUFFER(MaterialConstants, g_Material, DEPTH_BINDING_MATERIAL_CONSTANTS, DEPTH_SPACE_MATERIAL);

Texture2D t_BaseOrDiffuse       : REGISTER_SRV(DEPTH_BINDING_MATERIAL_DIFFUSE_TEXTURE, DEPTH_SPACE_MATERIAL);
Texture2D t_Opacity             : REGISTER_SRV(DEPTH_BINDING_MATERIAL_OPACITY_TEXTURE, DEPTH_SPACE_MATERIAL);
SamplerState s_MaterialSampler  : REGISTER_SAMPLER(DEPTH_BINDING_MATERIAL_SAMPLER, DEPTH_SPACE_VIEW);

void main(
    in float4 i_position : SV_Position,
	in float2 i_texCoord : TEXCOORD
)
{
    MaterialTextureSample textures = DefaultMaterialTextures();
    if ((g_Material.flags & MaterialFlags_UseBaseOrDiffuseTexture) != 0)
        textures.baseOrDiffuse = t_BaseOrDiffuse.Sample(s_MaterialSampler, i_texCoord);
    if ((g_Material.flags & MaterialFlags_UseOpacityTexture) != 0)
        textures.opacity = t_Opacity.Sample(s_MaterialSampler, i_texCoord).r;

    MaterialSample materialSample = EvaluateSceneMaterial(/* normal = */ float3(1, 0, 0),
        /* tangent = */ float4(0, 1, 0, 0), g_Material, textures);

    clip(materialSample.opacity - g_Material.alphaCutoff);
}
