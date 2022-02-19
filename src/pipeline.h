#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "types.h"

namespace vker {

class PipelineBuilder {
public:
    PipelineBuilder();

    void AddShader(VkShaderStageFlagBits stage, VkShaderModule module);

    void AddVertexBinding(u32 binding, u32 stride, VkVertexInputRate rate);
    void AddVertexAttribute(u32 location, u32 binding, VkFormat format, u32 offset);

    void SetInputAssembly(VkPrimitiveTopology topology, VkBool32 restart);

    void AddViewport(VkViewport viewport);
    void AddScissor(VkRect2D scissor);

	VkPipeline Build(VkDevice device, VkPipelineLayout layout, VkRenderPass pass);

private:
    std::vector<VkPipelineShaderStageCreateInfo> m_shaders;

    std::vector<VkVertexInputBindingDescription> m_vertex_bindings;
    std::vector<VkVertexInputAttributeDescription> m_vertex_attributes;

    VkPipelineInputAssemblyStateCreateInfo m_input_assembly;

    std::vector<VkViewport> m_viewports;
    std::vector<VkRect2D> m_scissors;

    std::vector<VkDescriptorSetLayout> m_descriptor_sets;
    std::vector<VkPushConstantRange> m_push_constant_ranges;
};

class PipelineLayoutBuilder {
public:
    void AddDescriptor(VkDescriptorSetLayout descriptor);
    void AddPushConstant(VkShaderStageFlags stages, u32 offset, u32 size);

    VkPipelineLayout Build(VkDevice device);

private:
    std::vector<VkDescriptorSetLayout> m_descriptors;
    std::vector<VkPushConstantRange> m_push_constants;
};

} // namespace vker