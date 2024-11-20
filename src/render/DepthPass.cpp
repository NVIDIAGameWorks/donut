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

#include <donut/render/DepthPass.h>
#include <donut/render/DrawStrategy.h>
#include <donut/engine/ShaderFactory.h>
#include <donut/engine/SceneTypes.h>
#include <donut/engine/CommonRenderPasses.h>
#include <donut/engine/View.h>
#include <donut/engine/MaterialBindingCache.h>
#include <nvrhi/utils.h>
#include <utility>

#if DONUT_WITH_STATIC_SHADERS
#if DONUT_WITH_DX11
#include "compiled_shaders/passes/depth_vs_input_assembler.dxbc.h"
#include "compiled_shaders/passes/depth_vs_buffer_loads.dxbc.h"
#include "compiled_shaders/passes/depth_ps.dxbc.h"
#endif
#if DONUT_WITH_DX12
#include "compiled_shaders/passes/depth_vs_input_assembler.dxil.h"
#include "compiled_shaders/passes/depth_vs_buffer_loads.dxil.h"
#include "compiled_shaders/passes/depth_ps.dxil.h"
#endif
#if DONUT_WITH_VULKAN
#include "compiled_shaders/passes/depth_vs_input_assembler.spirv.h"
#include "compiled_shaders/passes/depth_vs_buffer_loads.spirv.h"
#include "compiled_shaders/passes/depth_ps.spirv.h"
#endif
#endif

using namespace donut::math;
#include <donut/shaders/depth_cb.h>


using namespace donut::engine;
using namespace donut::render;

DepthPass::DepthPass(
    nvrhi::IDevice* device,
    std::shared_ptr<CommonRenderPasses> commonPasses)
    : m_Device(device)
    , m_CommonPasses(std::move(commonPasses))
{
    m_IsDX11 = m_Device->getGraphicsAPI() == nvrhi::GraphicsAPI::D3D11;
}

void DepthPass::Init(ShaderFactory& shaderFactory, const CreateParameters& params)
{
    m_UseInputAssembler = params.useInputAssembler;

    m_VertexShader = CreateVertexShader(shaderFactory, params);
    m_PixelShader = CreatePixelShader(shaderFactory, params);
    m_InputLayout = CreateInputLayout(m_VertexShader, params);
    m_InputBindingLayout = CreateInputBindingLayout();

    if (params.materialBindings)
        m_MaterialBindings = params.materialBindings;
    else
        m_MaterialBindings = CreateMaterialBindingCache(*m_CommonPasses);

    m_DepthCB = m_Device->createBuffer(nvrhi::utils::CreateVolatileConstantBufferDesc(sizeof(DepthPassConstants),
        "DepthPassConstants", params.numConstantBufferVersions));

    CreateViewBindings(m_ViewBindingLayout, m_ViewBindingSet, params);

    m_DepthBias = params.depthBias;
    m_DepthBiasClamp = params.depthBiasClamp;
    m_SlopeScaledDepthBias = params.slopeScaledDepthBias;
}

void DepthPass::ResetBindingCache()
{
    m_MaterialBindings->Clear();
    m_InputBindingSets.clear();
}

nvrhi::ShaderHandle DepthPass::CreateVertexShader(ShaderFactory& shaderFactory, const CreateParameters& params)
{
    char const* sourceFileName = "donut/passes/depth_vs.hlsl";

    if (params.useInputAssembler)
    {
        return shaderFactory.CreateAutoShader(sourceFileName, "input_assembler",
            DONUT_MAKE_PLATFORM_SHADER(g_depth_vs_input_assembler), nullptr, nvrhi::ShaderType::Vertex);
    }
    else
    {
        return shaderFactory.CreateAutoShader(sourceFileName, "buffer_loads",
            DONUT_MAKE_PLATFORM_SHADER(g_depth_vs_buffer_loads), nullptr, nvrhi::ShaderType::Vertex);
    }
}

nvrhi::ShaderHandle DepthPass::CreatePixelShader(ShaderFactory& shaderFactory, const CreateParameters& params)
{
    return shaderFactory.CreateAutoShader("donut/passes/depth_ps.hlsl", "main", DONUT_MAKE_PLATFORM_SHADER(g_depth_ps), nullptr, nvrhi::ShaderType::Pixel);
}

nvrhi::InputLayoutHandle DepthPass::CreateInputLayout(nvrhi::IShader* vertexShader, const CreateParameters& params)
{
    if (params.useInputAssembler)
    {
        nvrhi::VertexAttributeDesc aInputDescs[] =
        {
            GetVertexAttributeDesc(VertexAttribute::Position, "POSITION", 0),
            GetVertexAttributeDesc(VertexAttribute::TexCoord1, "TEXCOORD", 1),
            GetVertexAttributeDesc(VertexAttribute::Transform, "TRANSFORM", 2)
        };

        return m_Device->createInputLayout(aInputDescs, dim(aInputDescs), vertexShader);
    }

    return nullptr;
}

void DepthPass::CreateViewBindings(nvrhi::BindingLayoutHandle& layout, nvrhi::BindingSetHandle& set, const CreateParameters& params)
{
    auto bindingLayoutDesc = nvrhi::BindingLayoutDesc()
        .setVisibility(nvrhi::ShaderType::Vertex | nvrhi::ShaderType::Pixel)
        .setRegisterSpace(m_IsDX11 ? 0 : DEPTH_SPACE_VIEW)
        .setRegisterSpaceIsDescriptorSet(!m_IsDX11)
        .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(DEPTH_BINDING_VIEW_CONSTANTS))
        .addItem(nvrhi::BindingLayoutItem::Sampler(DEPTH_BINDING_MATERIAL_SAMPLER));

    layout = m_Device->createBindingLayout(bindingLayoutDesc);

    auto bindingSetDesc = nvrhi::BindingSetDesc()
        .setTrackLiveness(params.trackLiveness)
        .addItem(nvrhi::BindingSetItem::ConstantBuffer(DEPTH_BINDING_VIEW_CONSTANTS, m_DepthCB))
        .addItem(nvrhi::BindingSetItem::Sampler(DEPTH_BINDING_MATERIAL_SAMPLER,
            m_CommonPasses->m_AnisotropicWrapSampler));

    set = m_Device->createBindingSet(bindingSetDesc, layout);
}

std::shared_ptr<MaterialBindingCache> DepthPass::CreateMaterialBindingCache(CommonRenderPasses& commonPasses)
{
    std::vector<MaterialResourceBinding> materialBindings = {
        { MaterialResource::DiffuseTexture, DEPTH_BINDING_MATERIAL_DIFFUSE_TEXTURE },
        { MaterialResource::OpacityTexture, DEPTH_BINDING_MATERIAL_OPACITY_TEXTURE },
        { MaterialResource::ConstantBuffer, DEPTH_BINDING_MATERIAL_CONSTANTS }
    };

    return std::make_shared<MaterialBindingCache>(
        m_Device,
        nvrhi::ShaderType::Pixel,
        /* registerSpace = */ m_IsDX11 ? 0 : DEPTH_SPACE_MATERIAL,
        /* registerSpaceIsDescriptorSet = */ !m_IsDX11,
        materialBindings,
        commonPasses.m_AnisotropicWrapSampler,
        commonPasses.m_GrayTexture,
        commonPasses.m_BlackTexture);
}

nvrhi::GraphicsPipelineHandle DepthPass::CreateGraphicsPipeline(PipelineKey key, nvrhi::IFramebuffer* framebuffer)
{
    nvrhi::GraphicsPipelineDesc pipelineDesc;
    pipelineDesc.inputLayout = m_InputLayout;
    pipelineDesc.VS = m_VertexShader;
    pipelineDesc.PS = nullptr;
    pipelineDesc.renderState.rasterState.depthBias = m_DepthBias;
    pipelineDesc.renderState.rasterState.depthBiasClamp = m_DepthBiasClamp;
    pipelineDesc.renderState.rasterState.slopeScaledDepthBias = m_SlopeScaledDepthBias;
    pipelineDesc.renderState.rasterState.frontCounterClockwise = key.bits.frontCounterClockwise;
    pipelineDesc.renderState.rasterState.cullMode = key.bits.cullMode;
    pipelineDesc.renderState.depthStencilState.depthFunc = key.bits.reverseDepth
        ? nvrhi::ComparisonFunc::GreaterOrEqual
        : nvrhi::ComparisonFunc::LessOrEqual;

    pipelineDesc.bindingLayouts = { m_ViewBindingLayout };

    if (key.bits.alphaTested)
    {
        pipelineDesc.PS = m_PixelShader;
        pipelineDesc.bindingLayouts.push_back(m_MaterialBindings->GetLayout());
    }

    if (!m_UseInputAssembler)
        pipelineDesc.bindingLayouts.push_back(m_InputBindingLayout);

    return m_Device->createGraphicsPipeline(pipelineDesc, framebuffer);
}

nvrhi::BindingLayoutHandle DepthPass::CreateInputBindingLayout()
{
    if (m_UseInputAssembler)
        return nullptr;

    auto bindingLayoutDesc = nvrhi::BindingLayoutDesc()
        .setVisibility(nvrhi::ShaderType::Vertex)
        .setRegisterSpace(m_IsDX11 ? 0 : DEPTH_SPACE_INPUT)
        .setRegisterSpaceIsDescriptorSet(!m_IsDX11)
        .addItem(m_IsDX11
            ? nvrhi::BindingLayoutItem::RawBuffer_SRV(DEPTH_BINDING_INSTANCE_BUFFER)
            : nvrhi::BindingLayoutItem::StructuredBuffer_SRV(DEPTH_BINDING_INSTANCE_BUFFER))
        .addItem(nvrhi::BindingLayoutItem::RawBuffer_SRV(DEPTH_BINDING_VERTEX_BUFFER))
        .addItem(nvrhi::BindingLayoutItem::PushConstants(DEPTH_BINDING_PUSH_CONSTANTS, sizeof(DepthPushConstants)));
        
    return m_Device->createBindingLayout(bindingLayoutDesc);
}

nvrhi::BindingSetHandle DepthPass::CreateInputBindingSet(const BufferGroup* bufferGroup)
{
    auto bindingSetDesc = nvrhi::BindingSetDesc()
        .addItem(m_IsDX11
            ? nvrhi::BindingSetItem::RawBuffer_SRV(DEPTH_BINDING_INSTANCE_BUFFER, bufferGroup->instanceBuffer)
            : nvrhi::BindingSetItem::StructuredBuffer_SRV(DEPTH_BINDING_INSTANCE_BUFFER, bufferGroup->instanceBuffer))
        .addItem(nvrhi::BindingSetItem::RawBuffer_SRV(DEPTH_BINDING_VERTEX_BUFFER, bufferGroup->vertexBuffer))
        .addItem(nvrhi::BindingSetItem::PushConstants(DEPTH_BINDING_PUSH_CONSTANTS, sizeof(DepthPushConstants)));

    return m_Device->createBindingSet(bindingSetDesc, m_InputBindingLayout);
}

nvrhi::BindingSetHandle DepthPass::GetOrCreateInputBindingSet(const BufferGroup* bufferGroup)
{
    auto it = m_InputBindingSets.find(bufferGroup);
    if (it == m_InputBindingSets.end())
    {
        auto bindingSet = CreateInputBindingSet(bufferGroup);
        m_InputBindingSets[bufferGroup] = bindingSet;
        return bindingSet;
    }

    return it->second;
}

void DepthPass::SetPushConstants(
    donut::render::GeometryPassContext& abstractContext,
    nvrhi::ICommandList* commandList,
    nvrhi::GraphicsState& state,
    nvrhi::DrawArguments& args)
{
    if (m_UseInputAssembler)
        return;
        
    auto& context = static_cast<Context&>(abstractContext);

    DepthPushConstants constants;
    constants.startInstanceLocation = args.startInstanceLocation;
    constants.startVertexLocation = args.startVertexLocation;
    constants.positionOffset = context.positionOffset;
    constants.texCoordOffset = context.texCoordOffset;

    commandList->setPushConstants(&constants, sizeof(constants));

    args.startInstanceLocation = 0;
    args.startVertexLocation = 0;
}

ViewType::Enum DepthPass::GetSupportedViewTypes() const
{
    return ViewType::PLANAR;
}

void DepthPass::SetupView(GeometryPassContext& abstractContext, nvrhi::ICommandList* commandList, const engine::IView* view, const engine::IView* viewPrev)
{
    auto& context = static_cast<Context&>(abstractContext);
    
    DepthPassConstants depthConstants = {};
    depthConstants.matWorldToClip = view->GetViewProjectionMatrix();
    commandList->writeBuffer(m_DepthCB, &depthConstants, sizeof(depthConstants));

    context.keyTemplate.bits.frontCounterClockwise = view->IsMirrored();
    context.keyTemplate.bits.reverseDepth = view->IsReverseDepth();
}

bool DepthPass::SetupMaterial(GeometryPassContext& abstractContext, const engine::Material* material, nvrhi::RasterCullMode cullMode, nvrhi::GraphicsState& state)
{
    auto& context = static_cast<Context&>(abstractContext);

    PipelineKey key = context.keyTemplate;
    key.bits.cullMode = cullMode;

    bool const hasBaseOrDiffuseTexture = material->baseOrDiffuseTexture
        && material->baseOrDiffuseTexture->texture
        && material->enableBaseOrDiffuseTexture;

    bool const hasOpacityTexture = material->opacityTexture
        && material->opacityTexture->texture
        && material->enableOpacityTexture;
        
    if (material->domain == MaterialDomain::AlphaTested && (hasBaseOrDiffuseTexture || hasOpacityTexture))
    {
        nvrhi::IBindingSet* materialBindingSet = m_MaterialBindings->GetMaterialBindingSet(material);

        if (!materialBindingSet)
            return false;
        
        state.bindings = { m_ViewBindingSet, materialBindingSet };
        key.bits.alphaTested = true;
    }
    else if (material->domain == MaterialDomain::Opaque)
    {
        state.bindings = { m_ViewBindingSet };
        key.bits.alphaTested = false;
    }
    else
    {
        return false;
    }
    
    if (!m_UseInputAssembler)
        state.bindings.push_back(context.inputBindingSet);

    nvrhi::GraphicsPipelineHandle& pipeline = m_Pipelines[key.value];

    if (!pipeline)
    {
        std::lock_guard<std::mutex> lockGuard(m_Mutex);

        if (!pipeline)
            pipeline = CreateGraphicsPipeline(key, state.framebuffer);

        if (!pipeline)
            return false;
    }

    assert(pipeline->getFramebufferInfo() == state.framebuffer->getFramebufferInfo());

    state.pipeline = pipeline;
    return true;
}

void DepthPass::SetupInputBuffers(GeometryPassContext& abstractContext, const engine::BufferGroup* buffers, nvrhi::GraphicsState& state)
{
    auto& context = static_cast<Context&>(abstractContext);

    state.indexBuffer = { buffers->indexBuffer, nvrhi::Format::R32_UINT, 0 };

    if (m_UseInputAssembler)
    {
        state.vertexBuffers = {
            { buffers->vertexBuffer, 0, buffers->getVertexBufferRange(VertexAttribute::Position).byteOffset },
            { buffers->vertexBuffer, 1, buffers->getVertexBufferRange(VertexAttribute::TexCoord1).byteOffset },
            { buffers->instanceBuffer, 2, 0 }
        };
    }
    else
    {
        context.inputBindingSet = GetOrCreateInputBindingSet(buffers);
        context.positionOffset = uint32_t(buffers->getVertexBufferRange(VertexAttribute::Position).byteOffset);
        context.texCoordOffset = uint32_t(buffers->getVertexBufferRange(VertexAttribute::TexCoord1).byteOffset);
    }
}
