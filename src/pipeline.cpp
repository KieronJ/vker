#include <vulkan/vulkan.h>

#include "pipeline.h"
#include "types.h"
#include "utils.h"

namespace vker {

PipelineBuilder::PipelineBuilder()
{
    m_input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    m_input_assembly.pNext = nullptr;
    m_input_assembly.flags = {};
}

void PipelineBuilder::AddShader(VkShaderStageFlagBits stage, VkShaderModule module)
{
    VkPipelineShaderStageCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    create_info.stage = stage;
    create_info.module = module;
    create_info.pName = "main";
    create_info.pSpecializationInfo = nullptr;

    m_shaders.push_back(create_info);
}

void PipelineBuilder::AddVertexBinding(u32 binding, u32 stride, VkVertexInputRate rate)
{
    VkVertexInputBindingDescription vertex_binding_descriptor{};
    vertex_binding_descriptor.binding = binding;
    vertex_binding_descriptor.stride = stride;
    vertex_binding_descriptor.inputRate = rate;

    m_vertex_bindings.push_back(vertex_binding_descriptor);
}

void PipelineBuilder::AddVertexAttribute(u32 location, u32 binding, VkFormat format, u32 offset)
{
    VkVertexInputAttributeDescription vertex_attribute_descriptor{};
    vertex_attribute_descriptor.location = location;
    vertex_attribute_descriptor.binding = binding;
    vertex_attribute_descriptor.format = format;
    vertex_attribute_descriptor.offset = offset;

    m_vertex_attributes.push_back(vertex_attribute_descriptor);
}

void PipelineBuilder::SetInputAssembly(VkPrimitiveTopology topology, VkBool32 restart)
{
    m_input_assembly.topology = topology;
    m_input_assembly.primitiveRestartEnable = restart;
}

void PipelineBuilder::AddViewport(VkViewport viewport)
{
    m_viewports.push_back(viewport);
}

void PipelineBuilder::AddScissor(VkRect2D scissor)
{
    m_scissors.push_back(scissor);
}

VkPipeline PipelineBuilder::Build(VkDevice device, VkPipelineLayout layout, VkRenderPass pass)
{
    VkPipelineVertexInputStateCreateInfo vertex_input{};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertexBindingDescriptionCount = static_cast<u32>(m_vertex_bindings.size());
    vertex_input.pVertexBindingDescriptions = m_vertex_bindings.data();
    vertex_input.vertexAttributeDescriptionCount = static_cast<u32>(m_vertex_attributes.size());
    vertex_input.pVertexAttributeDescriptions = m_vertex_attributes.data();

    VkPipelineViewportStateCreateInfo viewport{};
    viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

    viewport.viewportCount = static_cast<u32>(m_viewports.size());
    viewport.pViewports = m_viewports.data();
    viewport.scissorCount = static_cast<u32>(m_scissors.size());
    viewport.pScissors = m_scissors.data();

    // TODO
    VkPipelineRasterizationStateCreateInfo rasterization{};
    rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization.depthClampEnable = VK_FALSE;
    rasterization.rasterizerDiscardEnable = VK_FALSE;
    rasterization.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization.depthBiasEnable = VK_FALSE;
    rasterization.depthBiasConstantFactor = 0.0f;
    rasterization.depthBiasClamp = 0.0f;
    rasterization.depthBiasSlopeFactor = 0.0f;
    rasterization.lineWidth = 1.0f;

    // TODO
    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample.sampleShadingEnable = VK_FALSE;
    multisample.minSampleShading = 0.0f;
    multisample.pSampleMask = nullptr;
    multisample.alphaToCoverageEnable = VK_FALSE;
    multisample.alphaToOneEnable = VK_FALSE;

    // TODO
    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable = VK_FALSE;
    depth_stencil.front.failOp = VK_STENCIL_OP_KEEP;
    depth_stencil.front.passOp = VK_STENCIL_OP_KEEP;
    depth_stencil.front.depthFailOp = VK_STENCIL_OP_KEEP;
    depth_stencil.front.compareOp = VK_COMPARE_OP_NEVER;
    depth_stencil.front.compareMask = 0;
    depth_stencil.front.writeMask = 0;
    depth_stencil.front.reference = 0;
    depth_stencil.back.failOp = VK_STENCIL_OP_KEEP;
    depth_stencil.back.passOp = VK_STENCIL_OP_KEEP;
    depth_stencil.back.depthFailOp = VK_STENCIL_OP_KEEP;
    depth_stencil.back.compareOp = VK_COMPARE_OP_NEVER;
    depth_stencil.back.compareMask = 0;
    depth_stencil.back.writeMask = 0;
    depth_stencil.back.reference = 0;
    depth_stencil.minDepthBounds = 0.0f;
    depth_stencil.maxDepthBounds = 0.0f;

    // TODO
    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    // TODO
    VkPipelineColorBlendStateCreateInfo color_blend{};
    color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend.logicOpEnable = VK_FALSE;
    color_blend.logicOp = VK_LOGIC_OP_CLEAR;
    color_blend.attachmentCount = 1;
    color_blend.pAttachments = &color_blend_attachment;
    color_blend.blendConstants[0] = 0.0f;
    color_blend.blendConstants[1] = 0.0f;
    color_blend.blendConstants[2] = 0.0f;
    color_blend.blendConstants[3] = 0.0f;

    VkGraphicsPipelineCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    create_info.stageCount = static_cast<u32>(m_shaders.size());
    create_info.pStages = m_shaders.data();
    create_info.pVertexInputState = &vertex_input;
    create_info.pInputAssemblyState = &m_input_assembly;
    create_info.pTessellationState = nullptr;
    create_info.pViewportState = &viewport;
    create_info.pRasterizationState = &rasterization;
    create_info.pMultisampleState = &multisample;
    create_info.pDepthStencilState = &depth_stencil;
    create_info.pColorBlendState = &color_blend;
    create_info.pDynamicState = nullptr;
    create_info.layout = layout;
    create_info.renderPass = pass;
    create_info.subpass = 0;
    create_info.basePipelineHandle = VK_NULL_HANDLE;
    create_info.basePipelineIndex = 0;

    VkPipeline pipeline;
    VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &create_info, nullptr, &pipeline));

    return pipeline;
}

void PipelineLayoutBuilder::AddDescriptor(VkDescriptorSetLayout descriptor)
{
    m_descriptors.push_back(descriptor);
}

void PipelineLayoutBuilder::AddPushConstant(VkShaderStageFlags stages, u32 offset, u32 size)
{
    VkPushConstantRange push_constant{};
    push_constant.stageFlags = stages;
    push_constant.offset = offset;
    push_constant.size = size;

    m_push_constants.push_back(push_constant);
}

VkPipelineLayout PipelineLayoutBuilder::Build(VkDevice device)
{
    VkPipelineLayoutCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    create_info.setLayoutCount = static_cast<u32>(m_descriptors.size());
    create_info.pSetLayouts = m_descriptors.data();
    create_info.pushConstantRangeCount = static_cast<u32>(m_push_constants.size());
    create_info.pPushConstantRanges = m_push_constants.data();

    VkPipelineLayout layout;
    VK_CHECK(vkCreatePipelineLayout(device, &create_info, nullptr, &layout));

    return layout;
}

} // namespace vker