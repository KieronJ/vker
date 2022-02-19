#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "contrib/vk_mem_alloc.h"

#include "buffer.h"
#include "image.h"
#include "types.h"
#include "vertex.h"

namespace vker {

class Model {
public:
    Model() = default;
    ~Model();

    Model(VmaAllocator allocator);

    void BuildBuffers();

    void Draw(VkCommandBuffer cmd) const;

    std::vector<u32> indices;
    std::vector<Vertex> vertices;

private:
    VmaAllocator m_allocator = VK_NULL_HANDLE;

    bool m_buffers_built = false;
    Buffer m_index_buffer;
    Buffer m_vertex_buffer;
    Image m_texture;
};

} // namespace vker