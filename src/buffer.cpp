#pragma once

#include <vulkan/vulkan.h>

#include "contrib/vk_mem_alloc.h"

#include "buffer.h"
#include "types.h"
#include "utils.h"

namespace vker {

void Buffer::Setup(VmaAllocator allocator, VkDeviceSize size,
    VkBufferUsageFlags usage, VkMemoryPropertyFlags mem_properties)
{
    m_allocator = allocator;

    {
        VkBufferCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        ci.size = size;
        ci.usage = usage;
        ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo alloc_ci{};
        alloc_ci.requiredFlags = mem_properties;

        VK_CHECK(vmaCreateBuffer(allocator, &ci, &alloc_ci, &m_buffer, &m_allocation, nullptr));
    }

    m_init = true;
}

void Buffer::Destroy()
{
    assert(m_init);
    vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
    m_init = false;
}

void * Buffer::Map()
{
    assert(m_init);

    void *address;
    VK_CHECK(vmaMapMemory(m_allocator, m_allocation, &address));
    return address;
}

void Buffer::Unmap()
{
    assert(m_init);
    vmaUnmapMemory(m_allocator, m_allocation);
}

} // namespace vker