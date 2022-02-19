#pragma once

#include <cassert>

#include <vulkan/vulkan.h>

#include "contrib/vk_mem_alloc.h"

#include "types.h"

namespace vker {

class Image {
public:
	Image() = default;

	void Setup(VkDevice device, VmaAllocator allocator, VkExtent2D size, VkFormat format, bool depth);
	void Destroy();

	inline VkImage Handle()
	{
		assert(m_init);
		return m_image;
	};

	inline VkImageView View()
	{
		assert(m_init);
		return m_image_view;
	}

	inline VkSampler Sampler()
	{
		assert(m_init);
		return m_sampler;
	}

private:
	bool m_init = false;

	VkDevice m_device;
	VmaAllocator m_allocator;

	VkImage m_image;
	VkImageView m_image_view;
	VkSampler m_sampler;
	VmaAllocation m_allocation;
};

} // namespace vker