#pragma once

#include <vulkan/vulkan.h>

#include "contrib/vk_mem_alloc.h"

#include "image.h"
#include "types.h"
#include "utils.h"

namespace vker {

void Image::Setup(VkDevice device, VmaAllocator allocator, VkExtent2D size, VkFormat format, bool depth)
{
    m_device = device;
    m_allocator = allocator;

    {
        VkImageCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ci.imageType = VK_IMAGE_TYPE_2D;
        ci.format = format;
        ci.extent = { size.width, size.height, 1 };
        ci.mipLevels = 1;
        ci.arrayLayers = 1;
        ci.samples = VK_SAMPLE_COUNT_1_BIT;

        if (depth) {
            ci.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        } else {
            ci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        }

        ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo alloc_ci{};
        alloc_ci.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        VK_CHECK(vmaCreateImage(allocator, &ci, &alloc_ci, &m_image, &m_allocation, nullptr));
    }

    {
        VkImageViewCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ci.image = m_image;
        ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ci.format = format;
        ci.subresourceRange.aspectMask = depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        ci.subresourceRange.baseMipLevel = 0;
        ci.subresourceRange.levelCount = 1;
        ci.subresourceRange.baseArrayLayer = 0;
        ci.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(device, &ci, nullptr, &m_image_view));
    }

    {
        VkSamplerCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        ci.magFilter = VK_FILTER_LINEAR;
        ci.minFilter = VK_FILTER_LINEAR;
        ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        ci.mipLodBias = 1.0f;
        ci.anisotropyEnable = VK_FALSE;
        ci.maxAnisotropy = 1.0f;
        ci.compareEnable = VK_FALSE;
        ci.minLod = 1.0f;
        ci.maxLod = 1.0f;
        ci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        ci.unnormalizedCoordinates = VK_FALSE;

        VK_CHECK(vkCreateSampler(device, &ci, nullptr, &m_sampler));
    }

    m_init = true;
}

void Image::Destroy()
{
    assert(m_init);
    vkDestroySampler(m_device, m_sampler, nullptr);
    vkDestroyImageView(m_device, m_image_view, nullptr);
    vmaDestroyImage(m_allocator, m_image, m_allocation);
    m_init = false;
}

} // namespace vker