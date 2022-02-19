#pragma once

#include <cassert>

#include <vulkan/vulkan.h>

#include "contrib/vk_mem_alloc.h"

#include "types.h"

namespace vker {

class Buffer {
public:
	Buffer() = default;

	void Setup(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags mem_properties);
	void Destroy();

	void * Map();
	void Unmap();

	inline VkBuffer Handle() const
	{
		assert(m_init);
		return m_buffer;
	};

private:
	bool m_init = false;

	VkDevice m_device;
	VmaAllocator m_allocator;

	VkBuffer m_buffer;
	VmaAllocation m_allocation;
};

} // namespace vker