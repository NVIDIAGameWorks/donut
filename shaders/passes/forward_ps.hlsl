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

#include <donut/shaders/forward_cb.h>

// Declare the constants that drive material bindings in 'material_bindings.hlsli'
// to match the bindings explicitly declared in 'forward_cb.h'.

#define MATERIAL_REGISTER_SPACE     FORWARD_SPACE_MATERIAL
#define MATERIAL_CB_SLOT            FORWARD_BINDING_MATERIAL_CONSTANTS
#define MATERIAL_DIFFUSE_SLOT       FORWARD_BINDING_MATERIAL_DIFFUSE_TEXTURE
#define MATERIAL_SPECULAR_SLOT      FORWARD_BINDING_MATERIAL_SPECULAR_TEXTURE
#define MATERIAL_NORMALS_SLOT       FORWARD_BINDING_MATERIAL_NORMAL_TEXTURE
#define MATERIAL_EMISSIVE_SLOT      FORWARD_BINDING_MATERIAL_EMISSIVE_TEXTURE
#define MATERIAL_OCCLUSION_SLOT     FORWARD_BINDING_MATERIAL_OCCLUSION_TEXTURE
#define MATERIAL_TRANSMISSION_SLOT  FORWARD_BINDING_MATERIAL_TRANSMISSION_TEXTURE
#define MATERIAL_OPACITY_SLOT       FORWARD_BINDING_MATERIAL_OPACITY_TEXTURE

#define MATERIAL_SAMPLER_REGISTER_SPACE FORWARD_SPACE_SHADING
#define MATERIAL_SAMPLER_SLOT           FORWARD_BINDING_MATERIAL_SAMPLER

#include <donut/shaders/scene_material.hlsli>
#include <donut/shaders/material_bindings.hlsli>
#include <donut/shaders/forward_vertex.hlsli>
#include <donut/shaders/lighting.hlsli>
#include <donut/shaders/shadows.hlsli>
#include <donut/shaders/binding_helpers.hlsli>

DECLARE_CBUFFER(ForwardShadingViewConstants, g_ForwardView, FORWARD_BINDING_VIEW_CONSTANTS,         FORWARD_SPACE_VIEW);
DECLARE_CBUFFER(ForwardShadingLightConstants, g_ForwardLight, FORWARD_BINDING_LIGHT_CONSTANTS,      FORWARD_SPACE_SHADING);

Texture2DArray t_ShadowMapArray       : REGISTER_SRV(FORWARD_BINDING_SHADOW_MAP_TEXTURE,            FORWARD_SPACE_SHADING);
TextureCubeArray t_DiffuseLightProbe  : REGISTER_SRV(FORWARD_BINDING_DIFFUSE_LIGHT_PROBE_TEXTURE,   FORWARD_SPACE_SHADING);
TextureCubeArray t_SpecularLightProbe : REGISTER_SRV(FORWARD_BINDING_SPECULAR_LIGHT_PROBE_TEXTURE,  FORWARD_SPACE_SHADING);
Texture2D t_EnvironmentBrdf           : REGISTER_SRV(FORWARD_BINDING_ENVIRONMENT_BRDF_TEXTURE,      FORWARD_SPACE_SHADING);

SamplerState s_ShadowSampler          : REGISTER_SAMPLER(FORWARD_BINDING_SHADOW_MAP_SAMPLER,        FORWARD_SPACE_SHADING);
SamplerState s_LightProbeSampler      : REGISTER_SAMPLER(FORWARD_BINDING_LIGHT_PROBE_SAMPLER,       FORWARD_SPACE_SHADING);
SamplerState s_BrdfSampler            : REGISTER_SAMPLER(FORWARD_BINDING_ENVIRONMENT_BRDF_SAMPLER,  FORWARD_SPACE_SHADING);

float3 GetIncidentVector(float4 directionOrPosition, float3 surfacePos)
{
    if (directionOrPosition.w > 0)
        return normalize(surfacePos.xyz - directionOrPosition.xyz);
    else
        return directionOrPosition.xyz;
}

void main(
    in float4 i_position : SV_Position,
    in SceneVertex i_vtx,
    in bool i_isFrontFace : SV_IsFrontFace,
    VK_LOCATION_INDEX(0, 0) out float4 o_color : SV_Target0
#if TRANSMISSIVE_MATERIAL
    , VK_LOCATION_INDEX(0, 1) out float4 o_backgroundBlendFactor : SV_Target1
#endif
)
{
    MaterialTextureSample textures = SampleMaterialTexturesAuto(i_vtx.texCoord, g_Material.normalTextureTransformScale);

    MaterialSample surfaceMaterial = EvaluateSceneMaterial(i_vtx.normal, i_vtx.tangent, g_Material, textures);
    float3 surfaceWorldPos = i_vtx.pos;

    if (!i_isFrontFace)
        surfaceMaterial.shadingNormal = -surfaceMaterial.shadingNormal;

    if (g_Material.domain != MaterialDomain_Opaque)
        clip(surfaceMaterial.opacity - g_Material.alphaCutoff);

    float4 cameraDirectionOrPosition = g_ForwardView.view.cameraDirectionOrPosition;
    float3 viewIncident = GetIncidentVector(cameraDirectionOrPosition, surfaceWorldPos);

    float3 diffuseTerm = 0;
    float3 specularTerm = 0;

    [loop]
    for(uint nLight = 0; nLight < g_ForwardLight.numLights; nLight++)
    {
        LightConstants light = g_ForwardLight.lights[nLight];

        float2 shadow = 0;
        for (int cascade = 0; cascade < 4; cascade++)
        {
            if (light.shadowCascades[cascade] >= 0)
            {
                float2 cascadeShadow = EvaluateShadowGather16(t_ShadowMapArray, s_ShadowSampler, g_ForwardLight.shadows[light.shadowCascades[cascade]], surfaceWorldPos, g_ForwardLight.shadowMapTextureSize);

                shadow = saturate(shadow + cascadeShadow * (1.0001 - shadow.y));

                if (shadow.y == 1)
                    break;
            }
            else
                break;
        }

        shadow.x += (1 - shadow.y) * light.outOfBoundsShadow;

        float objectShadow = 1;

        for (int object = 0; object < 4; object++)
        {
            if (light.perObjectShadows[object] >= 0)
            {
                float2 thisObjectShadow = EvaluateShadowGather16(t_ShadowMapArray, s_ShadowSampler, g_ForwardLight.shadows[light.perObjectShadows[object]], surfaceWorldPos, g_ForwardLight.shadowMapTextureSize);

                objectShadow *= saturate(thisObjectShadow.x + (1 - thisObjectShadow.y));
            }
        }

        shadow.x *= objectShadow;

        float3 diffuseRadiance, specularRadiance;
        ShadeSurface(light, surfaceMaterial, surfaceWorldPos, viewIncident, diffuseRadiance, specularRadiance);

        diffuseTerm += (shadow.x * diffuseRadiance) * light.color;
        specularTerm += (shadow.x * specularRadiance) * light.color;
    }

    float NdotV = saturate(-dot(surfaceMaterial.shadingNormal, viewIncident));

    if(g_ForwardLight.numLightProbes > 0)
    {
        float3 N = surfaceMaterial.shadingNormal;
        float3 R = reflect(viewIncident, N);

        float2 environmentBrdf = t_EnvironmentBrdf.SampleLevel(s_BrdfSampler, float2(NdotV, surfaceMaterial.roughness), 0).xy;

        float lightProbeWeight = 0;
        float3 lightProbeDiffuse = 0;
        float3 lightProbeSpecular = 0;

        [loop]
        for (uint nProbe = 0; nProbe < g_ForwardLight.numLightProbes; nProbe++)
        {
            LightProbeConstants lightProbe = g_ForwardLight.lightProbes[nProbe];

            float weight = GetLightProbeWeight(lightProbe, surfaceWorldPos);

            if (weight == 0)
                continue;

            float specularMipLevel = sqrt(saturate(surfaceMaterial.roughness)) * (lightProbe.mipLevels - 1);
            float3 diffuseProbe = t_DiffuseLightProbe.SampleLevel(s_LightProbeSampler, float4(N.xyz, lightProbe.diffuseArrayIndex), 0).rgb;
            float3 specularProbe = t_SpecularLightProbe.SampleLevel(s_LightProbeSampler, float4(R.xyz, lightProbe.specularArrayIndex), specularMipLevel).rgb;

            lightProbeDiffuse += (weight * lightProbe.diffuseScale) * diffuseProbe;
            lightProbeSpecular += (weight * lightProbe.specularScale) * specularProbe;
            lightProbeWeight += weight;
        }

        if (lightProbeWeight > 1)
        {
            float invWeight = rcp(lightProbeWeight);
            lightProbeDiffuse *= invWeight;
            lightProbeSpecular *= invWeight;
        }

        diffuseTerm += lightProbeDiffuse * surfaceMaterial.diffuseAlbedo * surfaceMaterial.occlusion;
        specularTerm += lightProbeSpecular * (surfaceMaterial.specularF0 * environmentBrdf.x + environmentBrdf.y) * surfaceMaterial.occlusion;
    }

    {
        float3 ambientColor = lerp(g_ForwardLight.ambientColorBottom.rgb, g_ForwardLight.ambientColorTop.rgb, surfaceMaterial.shadingNormal.y * 0.5 + 0.5);

        diffuseTerm += ambientColor * surfaceMaterial.diffuseAlbedo * surfaceMaterial.occlusion;
        specularTerm += ambientColor * surfaceMaterial.specularF0 * surfaceMaterial.occlusion;
    }
    
#if TRANSMISSIVE_MATERIAL
    
    // See https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_materials_transmission/README.md#transmission-btdf

    float dielectricFresnel = Schlick_Fresnel(0.04, NdotV);
    
    o_color.rgb = diffuseTerm * (1.0 - surfaceMaterial.transmission)
        + specularTerm
        + surfaceMaterial.emissiveColor;

    o_color.a = 1.0;

    float backgroundScalar = surfaceMaterial.transmission
        * (1.0 - dielectricFresnel);

    if (g_Material.domain == MaterialDomain_TransmissiveAlphaBlended)
        backgroundScalar *= (1.0 - surfaceMaterial.opacity);
    
    o_backgroundBlendFactor.rgb = backgroundScalar;

    if (surfaceMaterial.hasMetalRoughParams)
    {
        // Only apply the base color and metalness parameters if the surface is using the metal-rough model.
        // Transmissive behavoir is undefined on specular-gloss materials by the glTF spec, but it is
        // possible that the application creates such material regardless.

        o_backgroundBlendFactor.rgb *= surfaceMaterial.baseColor * (1.0 - surfaceMaterial.metalness);
    }

    o_backgroundBlendFactor.a = 1.0;

#else // TRANSMISSIVE_MATERIAL

    o_color.rgb = diffuseTerm
        + specularTerm
        + surfaceMaterial.emissiveColor;

    if (g_Material.domain == MaterialDomain_AlphaTested)
    {
        // Fix the fuzzy edges on alpha tested geometry.
        // See https://bgolus.medium.com/anti-aliased-alpha-test-the-esoteric-alpha-to-coverage-8b177335ae4f
        // Improved filtering quality by multiplying fwidth by sqrt(2).
        o_color.a = saturate((surfaceMaterial.opacity - g_Material.alphaCutoff)
            / max(fwidth(surfaceMaterial.opacity) * 1.4142, 0.0001) + 0.5);
    }
    else
        o_color.a = surfaceMaterial.opacity;

#endif // TRANSMISSIVE_MATERIAL
}
