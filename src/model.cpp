#include <cassert>

#include "contrib/vk_mem_alloc.h"

#include "model.h"

namespace vker {

Model::Model(VmaAllocator allocator) : m_allocator{allocator} {}

Model::~Model()
{
    if (m_buffers_built) {
        m_index_buffer.Destroy();
        m_vertex_buffer.Destroy();
    }
}

void Model::BuildBuffers()
{
    const size_t indices_size = indices.size() * sizeof(indices[0]);
    m_index_buffer.Setup(m_allocator, indices_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* address = m_index_buffer.Map();
    std::memcpy(address, indices.data(), indices_size);
    m_index_buffer.Unmap();

    const size_t vertices_size = vertices.size() * sizeof(vertices[0]);
    m_vertex_buffer.Setup(m_allocator, vertices_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    address = m_vertex_buffer.Map();
    std::memcpy(address, vertices.data(), vertices_size);
    m_vertex_buffer.Unmap();

    m_buffers_built = true;
}

void Model::Draw(VkCommandBuffer cmd) const
{
    assert(m_buffers_built);

    const VkBuffer vertex_buffer = m_vertex_buffer.Handle();
    const VkDeviceSize offset = 0;

    vkCmdBindIndexBuffer(cmd, m_index_buffer.Handle(), 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertex_buffer, &offset);

    vkCmdDrawIndexed(cmd, (u32)vertices.size(), (u32)indices.size(), 0, 0, 0);
}

} // namespace vker