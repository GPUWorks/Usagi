﻿#include <Usagi/Engine/Runtime/Exception.hpp>
#include <Usagi/Engine/Runtime/Graphics/SPIRVShader.hpp>

#include "VulkanGraphicsPipeline.hpp"
#include "VulkanGraphicsDevice.hpp"
#include "VulkanImage.hpp"

yuki::VulkanGraphicsPipeline::VulkanGraphicsPipeline(VulkanGraphicsDevice *vulkan_graphics_device)
    : mVulkanGD { std::move(vulkan_graphics_device) }
{
}

vk::VertexInputRate yuki::VulkanGraphicsPipeline::translateVertexInputRate(yuki::VertexBufferBinding::InputRate rate)
{
    switch(rate)
    {
        case VertexBufferBinding::InputRate::PER_VERTEX: return vk::VertexInputRate::eVertex;
        case VertexBufferBinding::InputRate::PER_INSTANCE: return vk::VertexInputRate::eInstance;
        default: throw GraphicsAPIEnumTranslationException() << EnumInfo("vk::VertexInputRate");
    }
}

vk::PrimitiveTopology yuki::VulkanGraphicsPipeline::translateTopology(const InputAssembleState::PrimitiveTopology topology)
{
    switch(topology)
    {
        case InputAssembleState::PrimitiveTopology::POINT_LIST: return vk::PrimitiveTopology::ePointList;
        case InputAssembleState::PrimitiveTopology::LINE_LIST: return vk::PrimitiveTopology::eLineList;
        case InputAssembleState::PrimitiveTopology::LINE_STRIP: return vk::PrimitiveTopology::eLineStrip;
        case InputAssembleState::PrimitiveTopology::TRIANGLE_LIST: return vk::PrimitiveTopology::eTriangleList;
        case InputAssembleState::PrimitiveTopology::TRIANGLE_STRIP: return vk::PrimitiveTopology::eTriangleStrip;
        case InputAssembleState::PrimitiveTopology::TRIANGLE_FAN: return vk::PrimitiveTopology::eTriangleFan;
        default: throw GraphicsAPIEnumTranslationException() << EnumInfo("vk::VertexInputRate");
    }
}

vk::CullModeFlags yuki::VulkanGraphicsPipeline::translateCullMode(
    const RasterizationState::FaceCullingMode face_culling_mode)
{
    switch(face_culling_mode)
    {
        case RasterizationState::FaceCullingMode::NONE: return vk::CullModeFlagBits::eNone;
        case RasterizationState::FaceCullingMode::FRONT: return vk::CullModeFlagBits::eFront;
        case RasterizationState::FaceCullingMode::BACK: return vk::CullModeFlagBits::eBack;
        default: throw GraphicsAPIEnumTranslationException() << EnumInfo("vk::CullModeFlags");
    }
}

vk::FrontFace yuki::VulkanGraphicsPipeline::translateFrontFace(const RasterizationState::FrontFace front_face)
{
    switch(front_face)
    {
        case RasterizationState::FrontFace::CLOCKWISE: return vk::FrontFace::eClockwise;
        case RasterizationState::FrontFace::COUNTER_CLOCKWISE: return vk::FrontFace::eCounterClockwise;
        default: throw GraphicsAPIEnumTranslationException() << EnumInfo("vk::FrontFace");
    }
}

vk::PolygonMode yuki::VulkanGraphicsPipeline::translatePolygonMode(const RasterizationState::PolygonMode polygon_mode)
{
    switch(polygon_mode)
    {
        case RasterizationState::PolygonMode::FILL: return vk::PolygonMode::eFill;
        case RasterizationState::PolygonMode::LINE: return vk::PolygonMode::eLine;
        case RasterizationState::PolygonMode::POINT: return vk::PolygonMode::ePoint;
        default: throw GraphicsAPIEnumTranslationException() << EnumInfo("vk::PolygonMode");
    }
}

vk::UniqueShaderModule yuki::VulkanGraphicsPipeline::_createShaderModule(const SPIRVShader *shader_bytecode) const
{
    vk::ShaderModuleCreateInfo shader_module_create_info;
    shader_module_create_info.setCodeSize(shader_bytecode->getByteCodeSize());
    shader_module_create_info.setPCode(shader_bytecode->getByteCode());
    auto shader_module = mVulkanGD->_getDevice().createShaderModuleUnique(shader_module_create_info);
    return shader_module;
}

vk::BlendOp yuki::VulkanGraphicsPipeline::translateBlendOp(const ColorBlendState::BlendingOperation blending_operation)
{
    switch(blending_operation)
    {
        case ColorBlendState::BlendingOperation::ADD: return vk::BlendOp::eAdd;
        case ColorBlendState::BlendingOperation::SUBTRACT: return vk::BlendOp::eSubtract;
        case ColorBlendState::BlendingOperation::REVERSE_SUBTRACT: return vk::BlendOp::eReverseSubtract;
        case ColorBlendState::BlendingOperation::MIN: return vk::BlendOp::eMin;
        case ColorBlendState::BlendingOperation::MAX: return vk::BlendOp::eMax;
        default: throw GraphicsAPIEnumTranslationException() << EnumInfo("vk::BlendOp");
    }
}

vk::BlendFactor yuki::VulkanGraphicsPipeline::translateColorBlendFactor(
    const ColorBlendState::BlendingFactor blending_factor)
{
    switch(blending_factor)
    {
        case ColorBlendState::BlendingFactor::ZERO: return vk::BlendFactor::eZero;
        case ColorBlendState::BlendingFactor::ONE: return vk::BlendFactor::eOne;
        case ColorBlendState::BlendingFactor::SOURCE_ALPHA: return vk::BlendFactor::eSrcAlpha;
        case ColorBlendState::BlendingFactor::ONE_MINUS_SOURCE_ALPHA: return vk::BlendFactor::eOneMinusSrcAlpha;
        default: throw GraphicsAPIEnumTranslationException() << EnumInfo("vk::BlendFactor");
    }
}

vk::RenderPass yuki::VulkanGraphicsPipeline::_getRenderPass() const
{
    return mRenderPass.get();
}

vk::Pipeline yuki::VulkanGraphicsPipeline::_getPipeline() const
{
    return mPipeline.get();
}

void yuki::VulkanGraphicsPipeline::init(const GraphicsPipelineCreateInfo &info)
{
    vk::GraphicsPipelineCreateInfo pipeline_create_info;

    // shader stages
    enum ShaderStage
    {
        VERTEX, FRAGMENT, SHADER_STAGE_COUNT
    };
    vk::PipelineShaderStageCreateInfo shader_stage_create_info[SHADER_STAGE_COUNT];
    auto vertex_shader = _createShaderModule(info.vertex_shader.get());
    shader_stage_create_info[VERTEX].setModule(vertex_shader.get());
    shader_stage_create_info[VERTEX].setPName("main"); // entry point name
    shader_stage_create_info[VERTEX].setStage(vk::ShaderStageFlagBits::eVertex);
    auto fragment_shader = _createShaderModule(info.fragment_shader.get());
    shader_stage_create_info[FRAGMENT].setModule(fragment_shader.get());
    shader_stage_create_info[FRAGMENT].setPName("main"); // entry point name
    shader_stage_create_info[FRAGMENT].setStage(vk::ShaderStageFlagBits::eFragment);
    pipeline_create_info.setStageCount(SHADER_STAGE_COUNT);
    pipeline_create_info.setPStages(shader_stage_create_info);

    // vertex input
    //vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info;
    //std::vector<vk::VertexInputBindingDescription> vertex_input_binding;
    //for(auto &&binding : info.vertex_input.bindings)
    //{
    //    vk::VertexInputBindingDescription vulkan_binding;
    //    vulkan_binding.binding = binding.binding_slot;
    //    vulkan_binding.stride = binding.stride;
    //    vulkan_binding.inputRate = _translateVertexInputRate(binding.input_rate);
    //    vertex_input_binding.push_back(std::move(vulkan_binding));
    //}
    //std::vector<vk::VertexInputAttributeDescription> vertex_input_attribute;
    //for(auto &&attr : info.vertex_input.attributes)
    //{
    //    vk::VertexInputAttributeDescription vulkan_attr;
    //    vulkan_attr.location = attr.location;
    //    vulkan_attr.binding = attr.binding_slot;
    //    vulkan_attr.offset = attr.offset;
    //    vulkan_attr.format = _translatSourceFormat(attr.source_format);
    //    vertex_input_attribute.push_back(std::move(vulkan_attr));
    //}
    //vertex_input_state_create_info.vertexBindingDescriptionCount = vertex_input_binding.size();
    //vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_input_binding[0];
    //vertex_input_state_create_info.vertexAttributeDescriptionCount = vertex_input_attribute.size();
    //vertex_input_state_create_info.pVertexAttributeDescriptions = &vertex_input_attribute[0];
    //pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
    vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info;
    pipeline_create_info.setPVertexInputState(&vertex_input_state_create_info);

    // input assembly
    vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_create_info;
    input_assembly_state_create_info.setTopology(translateTopology(info.input_assembly.topology));
    pipeline_create_info.setPInputAssemblyState(&input_assembly_state_create_info);

    // tessellation

    // viewport
    vk::PipelineViewportStateCreateInfo viewport_state_create_info;
    vk::Viewport viewport;
    // todo: link with window size
    viewport.setX(0);
    viewport.setY(0);
    viewport.setWidth(1280);
    viewport.setHeight(720);
    viewport.setMinDepth(0);
    viewport.setMaxDepth(1);
    viewport_state_create_info.setViewportCount(1);
    viewport_state_create_info.setPViewports(&viewport);
    vk::Rect2D scissor;
    // todo: window size
    scissor.setExtent({ 1280, 720 });
    scissor.setOffset({ 0, 0 });
    viewport_state_create_info.setScissorCount(1);
    viewport_state_create_info.setPScissors(&scissor);
    pipeline_create_info.setPViewportState(&viewport_state_create_info);

    // rasterization
    vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info;
    rasterization_state_create_info.setDepthBiasEnable(false);
    rasterization_state_create_info.setCullMode(translateCullMode(info.rasterization.face_culling_mode));
    rasterization_state_create_info.setFrontFace(translateFrontFace(info.rasterization.front_face));
    rasterization_state_create_info.setPolygonMode(translatePolygonMode(info.rasterization.polygon_mode));
    rasterization_state_create_info.setLineWidth(1.f);
    pipeline_create_info.setPRasterizationState(&rasterization_state_create_info);

    // multi-sampling
    // todo: anti-aliasing
    vk::PipelineMultisampleStateCreateInfo multisample_state_create_info;
    multisample_state_create_info.setRasterizationSamples(vk::SampleCountFlagBits::e1);
    multisample_state_create_info.setMinSampleShading(1.f);
    pipeline_create_info.setPMultisampleState(&multisample_state_create_info);

    // depth stencil

    // color blend
    vk::PipelineColorBlendAttachmentState color_blend_attachment_state;
    color_blend_attachment_state.setColorBlendOp(translateBlendOp(info.color_blend.color_blend_op));
    color_blend_attachment_state.setAlphaBlendOp(translateBlendOp(info.color_blend.alpha_blend_op));
    color_blend_attachment_state.setSrcColorBlendFactor(translateColorBlendFactor(info.color_blend.src_color_factor));
    color_blend_attachment_state.setDstColorBlendFactor(translateColorBlendFactor(info.color_blend.dst_color_factor));
    color_blend_attachment_state.setSrcAlphaBlendFactor(translateColorBlendFactor(info.color_blend.src_alpha_factor));
    color_blend_attachment_state.setDstAlphaBlendFactor(translateColorBlendFactor(info.color_blend.dst_alpha_factor));
    color_blend_attachment_state.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
    vk::PipelineColorBlendStateCreateInfo color_blend_state_create_info;
    color_blend_state_create_info.setAttachmentCount(1);
    color_blend_state_create_info.setPAttachments(&color_blend_attachment_state);
    pipeline_create_info.setPColorBlendState(&color_blend_state_create_info);

    // dynamic states
    // viewport and scissor should be set after binding the pipeline
    std::array<vk::DynamicState, 2> dynamic_state {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };
    vk::PipelineDynamicStateCreateInfo dynamic_state_create_info;
    dynamic_state_create_info.dynamicStateCount = dynamic_state.size();
    dynamic_state_create_info.pDynamicStates = dynamic_state.data();
    pipeline_create_info.pDynamicState = &dynamic_state_create_info;

    // pipeline layout
    vk::PipelineLayoutCreateInfo pipeline_layout_create_info;
    mPipelineLayout = mVulkanGD->_getDevice().createPipelineLayoutUnique(pipeline_layout_create_info);
    pipeline_create_info.setLayout(mPipelineLayout.get());

    // render pass
    std::vector<vk::AttachmentDescription> attachment_descriptions;
    std::vector<vk::AttachmentReference> attachment_references;
    for(auto &&attachment : info.attachment_usages)
    {
        vk::AttachmentDescription attachment_description; // all framebuffers available to the whole pass
        attachment_description.setFormat(static_cast<vk::Format>(attachment.native_format));
        attachment_description.setSamples(vk::SampleCountFlagBits::e1);
        attachment_description.setLoadOp(vk::AttachmentLoadOp::eClear);
        attachment_description.setStoreOp(vk::AttachmentStoreOp::eStore);
        attachment_description.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
        attachment_description.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
        attachment_description.setInitialLayout(VulkanImage::translateImageLayout(attachment.initial_layout));
        attachment_description.setFinalLayout(VulkanImage::translateImageLayout(attachment.final_layout));
        vk::AttachmentReference attachment_reference { static_cast<uint32_t>(attachment_descriptions.size()), vk::ImageLayout::eColorAttachmentOptimal };
        attachment_descriptions.push_back(std::move(attachment_description));
        attachment_references.push_back(std::move(attachment_reference));
    }

    vk::SubpassDescription subpass_description;
    subpass_description.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
    subpass_description.setColorAttachmentCount(attachment_references.size());
    subpass_description.setPColorAttachments(attachment_references.data());
    
    vk::RenderPassCreateInfo render_pass_create_info;
    render_pass_create_info.setAttachmentCount(attachment_descriptions.size());
    render_pass_create_info.setPAttachments(attachment_descriptions.data());
    render_pass_create_info.setSubpassCount(1);
    render_pass_create_info.setPSubpasses(&subpass_description);
    mRenderPass = mVulkanGD->_getDevice().createRenderPassUnique(render_pass_create_info);
    
    pipeline_create_info.setRenderPass(mRenderPass.get());
    pipeline_create_info.setSubpass(0);

    mPipeline = mVulkanGD->_getDevice().createGraphicsPipelineUnique({ }, pipeline_create_info);
}
