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

#ifndef MATERIAL_BINDINGS_HLSLI
#define MATERIAL_BINDINGS_HLSLI

#include "binding_helpers.hlsli"

// Bindings - can be overriden before including this file if necessary

#ifndef MATERIAL_CB_SLOT 
#define MATERIAL_CB_SLOT 0
#endif

#ifndef MATERIAL_DIFFUSE_SLOT 
#define MATERIAL_DIFFUSE_SLOT 0
#endif

#ifndef MATERIAL_SPECULAR_SLOT 
#define MATERIAL_SPECULAR_SLOT 1
#endif

#ifndef MATERIAL_NORMALS_SLOT 
#define MATERIAL_NORMALS_SLOT 2
#endif

#ifndef MATERIAL_EMISSIVE_SLOT 
#define MATERIAL_EMISSIVE_SLOT 3
#endif

#ifndef MATERIAL_OCCLUSION_SLOT 
#define MATERIAL_OCCLUSION_SLOT 4
#endif

#ifndef MATERIAL_TRANSMISSION_SLOT 
#define MATERIAL_TRANSMISSION_SLOT 5
#endif

#ifndef MATERIAL_OPACITY_SLOT 
#define MATERIAL_OPACITY_SLOT 6
#endif

#ifndef MATERIAL_SAMPLER_SLOT 
#define MATERIAL_SAMPLER_SLOT 0
#endif

#ifndef MATERIAL_REGISTER_SPACE
#define MATERIAL_REGISTER_SPACE 0
#endif

#ifndef MATERIAL_SAMPLER_REGISTER_SPACE
#define MATERIAL_SAMPLER_REGISTER_SPACE 0
#endif

cbuffer c_Material : REGISTER_CBUFFER(MATERIAL_CB_SLOT, MATERIAL_REGISTER_SPACE)
{
    MaterialConstants g_Material;
};

Texture2D t_BaseOrDiffuse        : REGISTER_SRV(MATERIAL_DIFFUSE_SLOT,       MATERIAL_REGISTER_SPACE);
Texture2D t_MetalRoughOrSpecular : REGISTER_SRV(MATERIAL_SPECULAR_SLOT,      MATERIAL_REGISTER_SPACE);
Texture2D t_Normal               : REGISTER_SRV(MATERIAL_NORMALS_SLOT,       MATERIAL_REGISTER_SPACE);
Texture2D t_Emissive             : REGISTER_SRV(MATERIAL_EMISSIVE_SLOT,      MATERIAL_REGISTER_SPACE);
Texture2D t_Occlusion            : REGISTER_SRV(MATERIAL_OCCLUSION_SLOT,     MATERIAL_REGISTER_SPACE);
Texture2D t_Transmission         : REGISTER_SRV(MATERIAL_TRANSMISSION_SLOT,  MATERIAL_REGISTER_SPACE);
Texture2D t_Opacity              : REGISTER_SRV(MATERIAL_OPACITY_SLOT,       MATERIAL_REGISTER_SPACE);
SamplerState s_MaterialSampler   : REGISTER_SAMPLER(MATERIAL_SAMPLER_SLOT,   MATERIAL_SAMPLER_REGISTER_SPACE);

MaterialTextureSample SampleMaterialTexturesAuto(float2 texCoord, float2 normalTexCoordScale)
{
    MaterialTextureSample values = DefaultMaterialTextures();

    if ((g_Material.flags & MaterialFlags_UseBaseOrDiffuseTexture) != 0)
    {
        values.baseOrDiffuse = t_BaseOrDiffuse.Sample(s_MaterialSampler, texCoord);
    }

    if ((g_Material.flags & MaterialFlags_UseMetalRoughOrSpecularTexture) != 0)
    {
        values.metalRoughOrSpecular = t_MetalRoughOrSpecular.Sample(s_MaterialSampler, texCoord);
    }

    if ((g_Material.flags & MaterialFlags_UseEmissiveTexture) != 0)
    {
        values.emissive = t_Emissive.Sample(s_MaterialSampler, texCoord);
    }

    if ((g_Material.flags & MaterialFlags_UseNormalTexture) != 0)
    {
        values.normal = t_Normal.Sample(s_MaterialSampler, texCoord * normalTexCoordScale);
    }

    if ((g_Material.flags & MaterialFlags_UseOcclusionTexture) != 0)
    {
        values.occlusion = t_Occlusion.Sample(s_MaterialSampler, texCoord);
    }

    if ((g_Material.flags & MaterialFlags_UseTransmissionTexture) != 0)
    {
        values.transmission = t_Transmission.Sample(s_MaterialSampler, texCoord);
    }

    if ((g_Material.flags & MaterialFlags_UseOpacityTexture) != 0)
    {
        values.opacity = t_Opacity.Sample(s_MaterialSampler, texCoord).r;
    }

    return values;
}

MaterialTextureSample SampleMaterialTexturesLevel(float2 texCoord, float lod)
{
    MaterialTextureSample values = DefaultMaterialTextures();

    if ((g_Material.flags & MaterialFlags_UseBaseOrDiffuseTexture) != 0)
    {
        values.baseOrDiffuse = t_BaseOrDiffuse.SampleLevel(s_MaterialSampler, texCoord, lod);
    }

    if ((g_Material.flags & MaterialFlags_UseMetalRoughOrSpecularTexture) != 0)
    {
        values.metalRoughOrSpecular = t_MetalRoughOrSpecular.SampleLevel(s_MaterialSampler, texCoord, lod);
    }

    if ((g_Material.flags & MaterialFlags_UseEmissiveTexture) != 0)
    {
        values.emissive = t_Emissive.SampleLevel(s_MaterialSampler, texCoord, lod);
    }

    if ((g_Material.flags & MaterialFlags_UseNormalTexture) != 0)
    {
        values.normal = t_Normal.SampleLevel(s_MaterialSampler, texCoord, lod);
    }

    if ((g_Material.flags & MaterialFlags_UseOcclusionTexture) != 0)
    {
        values.occlusion = t_Occlusion.SampleLevel(s_MaterialSampler, texCoord, lod);
    }

    if ((g_Material.flags & MaterialFlags_UseTransmissionTexture) != 0)
    {
        values.transmission = t_Transmission.SampleLevel(s_MaterialSampler, texCoord, lod);
    }

    if ((g_Material.flags & MaterialFlags_UseOpacityTexture) != 0)
    {
        values.opacity = t_Opacity.SampleLevel(s_MaterialSampler, texCoord, lod).r;
    }

    return values;
}

MaterialTextureSample SampleMaterialTexturesGrad(float2 texCoord, float2 ddx, float2 ddy)
{
    MaterialTextureSample values = DefaultMaterialTextures();

    if ((g_Material.flags & MaterialFlags_UseBaseOrDiffuseTexture) != 0)
    {
        values.baseOrDiffuse = t_BaseOrDiffuse.SampleGrad(s_MaterialSampler, texCoord, ddx, ddy);
    }

    if ((g_Material.flags & MaterialFlags_UseMetalRoughOrSpecularTexture) != 0)
    {
        values.metalRoughOrSpecular = t_MetalRoughOrSpecular.SampleGrad(s_MaterialSampler, texCoord, ddx, ddy);
    }

    if ((g_Material.flags & MaterialFlags_UseEmissiveTexture) != 0)
    {
        values.emissive = t_Emissive.SampleGrad(s_MaterialSampler, texCoord, ddx, ddy);
    }

    if ((g_Material.flags & MaterialFlags_UseNormalTexture) != 0)
    {
        values.normal = t_Normal.SampleGrad(s_MaterialSampler, texCoord, ddx, ddy);
    }
    
    if ((g_Material.flags & MaterialFlags_UseOcclusionTexture) != 0)
    {
        values.occlusion = t_Occlusion.SampleGrad(s_MaterialSampler, texCoord, ddx, ddy);
    }

    if ((g_Material.flags & MaterialFlags_UseTransmissionTexture) != 0)
    {
        values.transmission = t_Transmission.SampleGrad(s_MaterialSampler, texCoord, ddx, ddy);
    }

    if ((g_Material.flags & MaterialFlags_UseOpacityTexture) != 0)
    {
        values.opacity = t_Opacity.SampleGrad(s_MaterialSampler, texCoord, ddx, ddy).r;
    }

    return values;
}

#endif