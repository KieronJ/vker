#pragma once

#include <filesystem>

#include <vulkan/vulkan.h>

namespace vker::shader {

void Create(VkDevice device, VkShaderModule* shader, const std::filesystem::path& path);
void Destroy(VkDevice device, VkShaderModule shader);

} //namespace vker