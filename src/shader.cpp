#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

#include <vulkan/vulkan.h>

#include "shader.h"
#include "types.h"
#include "utils.h"

namespace vker::shader {

void Create(VkDevice device, VkShaderModule *shader, const std::filesystem::path& path)
{
	std::basic_ifstream<char> file{path, std::ios::binary};

	if (!file.is_open()) FatalError("unable to open shader file");

	std::vector<u8> data{std::istreambuf_iterator<char>{file}, {}};
	assert(data.size() % 4 == 0);

	VkShaderModuleCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = static_cast<u32>(data.size());
    create_info.pCode = reinterpret_cast<u32 *>(data.data());

	VK_CHECK(vkCreateShaderModule(device, &create_info, nullptr, shader));
}

void Destroy(VkDevice device, VkShaderModule shader)
{
	vkDestroyShaderModule(device, shader, nullptr);
}

} // namespace vker