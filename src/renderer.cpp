#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <vector>

#include <fmt/printf.h>
#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec3.hpp>

#define VMA_IMPLEMENTATION
#include "contrib/vk_mem_alloc.h"

#define STB_IMAGE_IMPLEMENTATION
#include "contrib/stb_image.h"

#include "buffer.h"
#include "camera.h"
#include "pipeline.h"
#include "renderer.h"
#include "shader.h"
#include "window.h"
#include "utils.h"
#include "vertex.h"

namespace vker {

Renderer::Renderer(const Window &window) : m_swapchain{}
{
	CreateInstance(window);
    window.CreateSurface(m_instance, &m_surface);

#ifndef NDEBUG
    CreateDebugMessenger();
#endif // NDEBUG

    EnumeratePhysicalDevices();
    SelectPhysicalDevice();
    CreateDevice();
    CreateAllocator();
    CreateSwapchain();
    CreateRenderPass();
    CreatePipeline();
    CreateDepthBuffers();
    CreateFramebuffers();
    CreateCommandPool();
    CreateCommandBuffers();

    CreateUniformBuffer();
    CreateTexture();
    CreateDescriptorPool();

    CreateSemaphores();
    CreateFences();
}

Renderer::~Renderer()
{
    vkDeviceWaitIdle(m_device);

    m_texture.Destroy();
    m_models.clear();

    for (auto& depth_buffer : m_depth_buffers) {
        depth_buffer.Destroy();
    }

    m_uniform_buffer.Unmap();
    m_uniform_buffer.Destroy();

    for (size_t i = 0; i < m_fences.size(); ++i) {
        vkDestroyFence(m_device, m_fences[i], nullptr);
    }

    for (size_t i = 0; i < m_image_available_semaphores.size(); ++i) {
        vkDestroySemaphore(m_device, m_image_available_semaphores[i], nullptr);
    }

    for (size_t i = 0; i < m_render_finished_semaphores.size(); ++i) {
        vkDestroySemaphore(m_device, m_render_finished_semaphores[i], nullptr);
    }

    vkDestroyCommandPool(m_device, m_command_pool, nullptr);

    vkDestroyDescriptorSetLayout(m_device, m_descriptor_set_layout, nullptr);
    vkDestroyDescriptorPool(m_device, m_descriptor_pool, nullptr);

    vkDestroyPipeline(m_device, m_pipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);

    vkDestroyRenderPass(m_device, m_render_pass, nullptr);

    for (auto& framebuffer : m_swapchain.framebuffers) vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    for (auto& image_view : m_swapchain.image_views) vkDestroyImageView(m_device, image_view, nullptr);

    vkDestroySwapchainKHR(m_device, m_swapchain.swapchain, nullptr);

    vmaDestroyAllocator(m_allocator);
    vkDestroyDevice(m_device, nullptr);

#ifndef NDEBUG
    DestroyDebugMessenger();
#endif // NDEBUG

    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

    vkDestroyInstance(m_instance, nullptr);
}

void Renderer::Draw(const Camera& cam)
{   
    if (!m_swapchain.valid) {
        vkDeviceWaitIdle(m_device);

        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, m_surface, &m_gpu.surface_caps));

        CreateSwapchain();
        CreateFramebuffers();

        m_swapchain.valid = true;
    }

    const VkSemaphore image_available_sema = m_image_available_semaphores[m_semaphores_index];
    const VkSemaphore render_finished_sema = m_render_finished_semaphores[m_semaphores_index];

    VK_CHECK(vkAcquireNextImageKHR(m_device, m_swapchain.swapchain, UINT64_MAX, image_available_sema, VK_NULL_HANDLE, &m_frame_index));

    const VkFence fence = m_fences[m_frame_index];

    VK_CHECK(vkWaitForFences(m_device, 1, &fence, VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(m_device, 1, &fence));

    VkCommandBuffer buffer = m_command_buffers[m_frame_index];

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(buffer, &begin_info);

    VkClearValue clear_values[2];
    clear_values[0].color = {{ 119.0f / 255.0f, 41.0f / 255.0f, 83.0f / 255.0f, 1.0f }};
    clear_values[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo render_pass_begin_info{};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = m_render_pass;
    render_pass_begin_info.framebuffer = m_swapchain.framebuffers[m_frame_index];
    render_pass_begin_info.renderArea.offset.x = 0;
    render_pass_begin_info.renderArea.offset.y = 0;
    render_pass_begin_info.renderArea.extent = m_swapchain.extent;
    render_pass_begin_info.clearValueCount = 2;
    render_pass_begin_info.pClearValues = clear_values;

    const glm::mat4 mvp = cam.GetProjectionMatrix() * cam.GetViewMatrix() * glm::mat4(1.0f);
    std::memcpy(m_uniform_buffer_addr, &mvp, sizeof(mvp));

    vkCmdBeginRenderPass(buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &m_uniform_descriptor_set, 0, nullptr);

    for (const auto& model : m_models) {
        model.Draw(buffer);
    }

    vkCmdEndRenderPass(buffer);

    vkEndCommandBuffer(buffer);

    const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &image_available_sema;
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &render_finished_sema;

    vkQueueSubmit(m_queue, 1, &submit_info, fence);
}

void Renderer::Present()
{
    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &m_render_finished_semaphores[m_semaphores_index];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &m_swapchain.swapchain;
    present_info.pImageIndices = &m_frame_index;

    VK_CHECK(vkQueuePresentKHR(m_queue, &present_info));

    m_semaphores_index = (m_semaphores_index + 1) % m_swapchain.image_count;
}

void Renderer::CreateInstance(const Window &window)
{
    std::vector<const char *> layers;
    std::vector<const char *> extensions;

#ifndef NDEBUG
    layers.push_back("VK_LAYER_KHRONOS_validation");
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // NDEBUG

    u32 wextension_count = 0;
    auto wextensions = window.QueryInstanceExtensions(wextension_count);

    for (u32 i = 0; i < wextension_count; ++i) {
        extensions.push_back(wextensions[i]);
    }

    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "vker";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName = "vker";
    app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledLayerCount = static_cast<u32>(layers.size());
    create_info.ppEnabledLayerNames = layers.data();
    create_info.enabledExtensionCount = static_cast<u32>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

    VK_CHECK(vkCreateInstance(&create_info, nullptr, &m_instance));
}

#ifndef NDEBUG
VkBool32 VKAPI_CALL Renderer::DebugMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data)
{
    fmt::print(stderr, "{}\n", callback_data->pMessage);
    return VK_FALSE;
}

void Renderer::CreateDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = &DebugMessengerCallback;

    auto create_debug_messenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
    assert(create_debug_messenger);

    VK_CHECK(create_debug_messenger(m_instance, &create_info, nullptr, &m_debug_messenger));
}

void Renderer::DestroyDebugMessenger()
{
    auto destroy_debug_messenger = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
    assert(destroy_debug_messenger);

    destroy_debug_messenger(m_instance, m_debug_messenger, nullptr);
}
#endif // NDEBUG

void Renderer::EnumeratePhysicalDevices()
{
    u32 device_count;
    VK_CHECK(vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr));
    assert(device_count != 0);

    std::vector<VkPhysicalDevice> devices{ device_count };

    VK_CHECK(vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data()));
    assert(device_count != 0);

    m_gpus.resize(device_count);

    for (u32 i = 0; i < device_count; ++i) {
        gpu_info& gpu = m_gpus[i];
        gpu.device = devices[i];

        vkGetPhysicalDeviceProperties(devices[i], &gpu.props);
        vkGetPhysicalDeviceMemoryProperties(devices[i], &gpu.memory_props);

    {
        u32 extension_props;
        VK_CHECK(vkEnumerateDeviceExtensionProperties(devices[i], nullptr, &extension_props, nullptr));
        assert(extension_props != 0);

        gpu.extension_props.resize(extension_props);

        VK_CHECK(vkEnumerateDeviceExtensionProperties(devices[i], nullptr, &extension_props, gpu.extension_props.data()));
        assert(extension_props != 0);
    }

    {
        u32 queue_family_props;
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queue_family_props, nullptr);
        assert(queue_family_props != 0);

        gpu.queue_family_props.resize(queue_family_props);

        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queue_family_props, gpu.queue_family_props.data());
        assert(queue_family_props != 0);
    }

        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devices[i], m_surface, &gpu.surface_caps));

    {
        u32 surface_formats;
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(devices[i], m_surface, &surface_formats, nullptr));
        assert(surface_formats != 0);

        gpu.surface_formats.resize(surface_formats);

        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(devices[i], m_surface, &surface_formats, gpu.surface_formats.data()));
        assert(surface_formats != 0);
    }

    {
        u32 surface_present_modes;
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(devices[i], m_surface, &surface_present_modes, nullptr));
        assert(surface_present_modes != 0);

        gpu.surface_present_modes.resize(surface_present_modes);

        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(devices[i], m_surface, &surface_present_modes, gpu.surface_present_modes.data()));
        assert(surface_present_modes != 0);
    }

    }
}

void Renderer::SelectPhysicalDevice()
{
    SelectOptimalPhysicalDevice(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
    assert(m_physical_device);

    fmt::print("selected physical device {}\n", m_gpu.props.deviceName);

    u32 max_queue_count{};

    // Select the largest queue family supporting both graphics and present
    for (u32 i = 0; i < static_cast<u32>(m_gpu.queue_family_props.size()); ++i) {
        const auto& props = m_gpu.queue_family_props[i];

        if ((props.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) continue;

        VkBool32 supported;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(m_physical_device, i, m_surface, &supported));

        if (!supported) continue;

        if (props.queueCount > max_queue_count) {
            max_queue_count = props.queueCount;
            m_queue_family = i;
        }
    }

    assert(max_queue_count != 0);

    fmt::print("selected queue family {}\n", m_queue_family);
}

void Renderer::CreateDevice()
{
    std::vector<const char *> extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    const float priorities[] = { 1.f };

    VkDeviceQueueCreateInfo device_queue_create_info{};
    device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    device_queue_create_info.queueFamilyIndex = m_queue_family;
    device_queue_create_info.queueCount = 1;
    device_queue_create_info.pQueuePriorities = priorities;

    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = 1;
    create_info.pQueueCreateInfos = &device_queue_create_info;
    create_info.enabledExtensionCount = static_cast<u32>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

    VK_CHECK(vkCreateDevice(m_physical_device, &create_info, nullptr, &m_device));
    vkGetDeviceQueue(m_device, m_queue_family, 0, &m_queue);
}

void Renderer::CreateAllocator()
{
    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.physicalDevice = m_physical_device;
    allocator_info.device = m_device;
    allocator_info.instance = m_instance;
    allocator_info.vulkanApiVersion = VK_API_VERSION_1_0;

    VK_CHECK(vmaCreateAllocator(&allocator_info, &m_allocator));
}

void Renderer::CreateSwapchain()
{
    const auto caps = m_gpu.surface_caps;

    m_swapchain.format = SelectOptimalSwapchainFormat();
    m_swapchain.extent = SelectOptimalSwapchainExtent();
    m_swapchain.image_count = SelectOptimalSwapchainImageCount();

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = m_surface;
    create_info.minImageCount = m_swapchain.image_count;
    create_info.imageFormat = m_swapchain.format.format;
    create_info.imageColorSpace = m_swapchain.format.colorSpace;
    create_info.imageExtent = m_swapchain.extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.preTransform = caps.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = m_swapchain.swapchain;

    VK_CHECK(vkCreateSwapchainKHR(m_device, &create_info, nullptr, &m_swapchain.swapchain));

    for (size_t i = 0; i < m_swapchain.image_views.size(); ++i) {
        vkDestroyImageView(m_device, m_swapchain.image_views[i], nullptr);
    }

    u32 swapchain_images;
    VK_CHECK(vkGetSwapchainImagesKHR(m_device, m_swapchain.swapchain, &swapchain_images, nullptr));
    assert(swapchain_images != 0);

    m_swapchain.images.resize(swapchain_images);
    m_swapchain.image_views.resize(swapchain_images);

    VK_CHECK(vkGetSwapchainImagesKHR(m_device, m_swapchain.swapchain, &swapchain_images, m_swapchain.images.data()));
    assert(swapchain_images != 0);

    for (size_t i = 0; i < m_swapchain.images.size(); ++i) {
        VkImageViewCreateInfo iv_create_info{};
        iv_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        iv_create_info.image = m_swapchain.images[i];
        iv_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        iv_create_info.format = m_swapchain.format.format;
        iv_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        iv_create_info.subresourceRange.baseMipLevel = 0;
        iv_create_info.subresourceRange.levelCount = 1;
        iv_create_info.subresourceRange.baseArrayLayer = 0;
        iv_create_info.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(m_device, &iv_create_info, nullptr, &m_swapchain.image_views[i]));
    }
}

void Renderer::CreateRenderPass()
{
    VkAttachmentDescription attachment_descriptions[2] = {};
    attachment_descriptions[0].format = m_swapchain.format.format;
    attachment_descriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_descriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_descriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_descriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_descriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachment_descriptions[1].format = VK_FORMAT_D32_SFLOAT_S8_UINT;
    attachment_descriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_descriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_descriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_descriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_reference{};
    color_reference.attachment = 0;
    color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_reference{};
    depth_reference.attachment = 1;
    depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_reference;
    subpass.pDepthStencilAttachment = &depth_reference;

    VkSubpassDependency subpass_depencency{};
    subpass_depencency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_depencency.dstSubpass = 0;
    subpass_depencency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_depencency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_depencency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.attachmentCount = 2;
    create_info.pAttachments = attachment_descriptions;
    create_info.subpassCount = 1;
    create_info.pSubpasses = &subpass;
    create_info.dependencyCount = 1;
    create_info.pDependencies = &subpass_depencency;

    VK_CHECK(vkCreateRenderPass(m_device, &create_info, nullptr, &m_render_pass));
}

void Renderer::CreatePipeline()
{
    VkDescriptorSetLayoutBinding layout_bindings[2]{};
    layout_bindings[0].binding = 0;
    layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout_bindings[0].descriptorCount = 1;
    layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    layout_bindings[1].binding = 1;
    layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layout_bindings[1].descriptorCount = 1;
    layout_bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.bindingCount = 2;
    create_info.pBindings = layout_bindings;

    vkCreateDescriptorSetLayout(m_device, &create_info, nullptr, &m_descriptor_set_layout);

    PipelineLayoutBuilder layout_builder;
    layout_builder.AddDescriptor(m_descriptor_set_layout);

    PipelineBuilder pipeline_builder;

    VkShaderModule vert;
    VkShaderModule frag;

    shader::Create(m_device, &vert, "../../../shader/triangle.vert.spv");
    shader::Create(m_device, &frag, "../../../shader/triangle.frag.spv");

    pipeline_builder.AddShader(VK_SHADER_STAGE_VERTEX_BIT, vert);
    pipeline_builder.AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, frag);

    pipeline_builder.AddVertexBinding(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);

    pipeline_builder.AddVertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos));
    pipeline_builder.AddVertexAttribute(1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, tex));

    pipeline_builder.SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapchain.extent.width);
    viewport.height = static_cast<float>(m_swapchain.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    pipeline_builder.AddViewport(viewport);

    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = m_swapchain.extent;

    pipeline_builder.AddScissor(scissor);

    m_pipeline_layout = layout_builder.Build(m_device);
    m_pipeline = pipeline_builder.Build(m_device, m_pipeline_layout, m_render_pass);

    shader::Destroy(m_device, vert);
    shader::Destroy(m_device, frag);
}

void Renderer::CreateUniformBuffer()
{
    m_uniform_buffer.Setup(m_allocator, sizeof(glm::mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    m_uniform_buffer_addr = m_uniform_buffer.Map();
}

void Renderer::CreateDepthBuffers()
{
    m_depth_buffers.resize(m_swapchain.images.size());

    for (auto& depth_buffer : m_depth_buffers) {
        depth_buffer.Setup(m_device, m_allocator, m_swapchain.extent, VK_FORMAT_D32_SFLOAT_S8_UINT, true);
    }
}

void Renderer::CreateDescriptorPool()
{
    VkDescriptorPoolSize pool_size[2];
    pool_size[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size[0].descriptorCount = 1;

    pool_size[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_size[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    //ci.flags = ...;
    ci.maxSets = 2;
    ci.poolSizeCount = 2;
    ci.pPoolSizes = pool_size;

    VK_CHECK(vkCreateDescriptorPool(m_device, &ci, nullptr, &m_descriptor_pool));

    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = m_descriptor_pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &m_descriptor_set_layout;

    VK_CHECK(vkAllocateDescriptorSets(m_device, &alloc_info, &m_uniform_descriptor_set));

    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = m_uniform_buffer.Handle();
    buffer_info.offset = 0;
    buffer_info.range = VK_WHOLE_SIZE;

    VkDescriptorImageInfo image_info{};
    image_info.sampler = m_texture.Sampler();
    image_info.imageView = m_texture.View();
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet writes[2]{};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = m_uniform_descriptor_set;
    writes[0].dstBinding = 0;
    writes[0].dstArrayElement = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pBufferInfo = &buffer_info;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = m_uniform_descriptor_set;
    writes[1].dstBinding = 1;
    writes[1].dstArrayElement = 0;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].pImageInfo = &image_info;

    vkUpdateDescriptorSets(m_device, 2, writes, 0, nullptr);
}

void Renderer::CreateFramebuffers()
{
    for (size_t i = 0; i < m_swapchain.framebuffers.size(); ++i) {
        vkDestroyFramebuffer(m_device, m_swapchain.framebuffers[i], nullptr);
    }

    m_swapchain.framebuffers.resize(m_swapchain.image_views.size());

    for (size_t i = 0; i < m_swapchain.image_views.size(); ++i) {
        VkImageView attachments[2] = {
            m_swapchain.image_views[i],
            m_depth_buffers[i].View()
        };

        VkFramebufferCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = m_render_pass;
        create_info.attachmentCount = 2;
        create_info.pAttachments = attachments;
        create_info.width = m_swapchain.extent.width;
        create_info.height = m_swapchain.extent.height;
        create_info.layers = 1;

        VK_CHECK(vkCreateFramebuffer(m_device, &create_info, nullptr, &m_swapchain.framebuffers[i]));
    }
}

void Renderer::CreateCommandPool()
{
    VkCommandPoolCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    create_info.queueFamilyIndex = m_queue_family;

    VK_CHECK(vkCreateCommandPool(m_device, &create_info, nullptr, &m_command_pool));
}

void Renderer::CreateCommandBuffers()
{
    m_command_buffers.resize(m_swapchain.framebuffers.size());

    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = m_command_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = static_cast<u32>(m_command_buffers.size());

    VK_CHECK(vkAllocateCommandBuffers(m_device, &allocate_info, m_command_buffers.data()));
}

void Renderer::CreateSemaphores()
{
    VkSemaphoreCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    m_image_available_semaphores.resize(m_swapchain.image_count);

    for (size_t i = 0; i < m_image_available_semaphores.size(); ++i) {
        VK_CHECK(vkCreateSemaphore(m_device, &create_info, nullptr, &m_image_available_semaphores[i]));
    }

    m_render_finished_semaphores.resize(m_swapchain.image_count);

    for (size_t i = 0; i < m_render_finished_semaphores.size(); ++i) {
        VK_CHECK(vkCreateSemaphore(m_device, &create_info, nullptr, &m_render_finished_semaphores[i]));
    }

    m_semaphores_index = 0;
}

void Renderer::CreateFences()
{
    VkFenceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    m_fences.resize(m_swapchain.image_count);

    for (size_t i = 0; i < m_fences.size(); ++i) {
        VK_CHECK(vkCreateFence(m_device, &create_info, nullptr, &m_fences[i]));
    }
}

void Renderer::CreateTexture()
{
    int width, height, channels;
    stbi_uc* pixels = stbi_load("../../../asset/texture/viking_room.png", &width, &height, &channels, STBI_rgb_alpha);
    assert(pixels);

    Buffer staging_buffer;
    staging_buffer.Setup(m_allocator, width * height * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    void *address = staging_buffer.Map();
    std::memcpy(address, pixels, width * height * 4);
    staging_buffer.Unmap();

    VkExtent2D size{ width, height };
    m_texture.Setup(m_device, m_allocator, size, VK_FORMAT_R8G8B8A8_UNORM, false);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    const VkCommandBuffer cmd = m_command_buffers[0];

    VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));

    {
        VkImageMemoryBarrier image_barrier{};
        image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_barrier.srcAccessMask = 0;
        image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        image_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        image_barrier.image = m_texture.Handle();
        image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_barrier.subresourceRange.baseMipLevel = 0;
        image_barrier.subresourceRange.levelCount = 1;
        image_barrier.subresourceRange.baseArrayLayer = 0;
        image_barrier.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier);
    }

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageSubresource.mipLevel = 0;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { size.width, size.height, 1 };

    vkCmdCopyBufferToImage(cmd, staging_buffer.Handle(), m_texture.Handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    {
        VkImageMemoryBarrier image_barrier{};
        image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        image_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        image_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        image_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_barrier.image = m_texture.Handle();
        image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_barrier.subresourceRange.baseMipLevel = 0;
        image_barrier.subresourceRange.levelCount = 1;
        image_barrier.subresourceRange.baseArrayLayer = 0;
        image_barrier.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier);
    }

    VK_CHECK(vkEndCommandBuffer(cmd));

    const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_command_buffers[0];

    vkQueueSubmit(m_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_queue);

    staging_buffer.Destroy();
    stbi_image_free(pixels);
}

void Renderer::SelectOptimalPhysicalDevice(VkPhysicalDeviceType type)
{
    VkDeviceSize max_vram{};

    // Attempt to select the GPU with the largest VRAM pool
    for (const auto& gpu : m_gpus) {
        if (gpu.props.deviceType != type) continue;
        if (gpu.surface_formats.size() == 0) continue;
        if (gpu.surface_present_modes.size() == 0) continue;

        VkDeviceSize vram{};

        // Calculate the total amount of device local memory (VRAM)
        for (u32 i = 0; i < gpu.memory_props.memoryHeapCount; ++i) {
            const auto& heap = gpu.memory_props.memoryHeaps[i];
            if ((heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0) vram += heap.size;
        }

        if (vram <= max_vram) continue;

        max_vram = vram;

        m_gpu = gpu;
        m_physical_device = gpu.device;
    }
}

VkSurfaceFormatKHR Renderer::SelectOptimalSwapchainFormat()
{
    const VkSurfaceFormatKHR optimal = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

    // Attempt to find B8G8R8A8 format with SRGB nonlinear color space
    for (const auto& format : m_gpu.surface_formats) {
        if (format.format == optimal.format && format.colorSpace == optimal.colorSpace) return optimal;
    }

    // Just return the first available format otherwise, it will probably be fine. Maybe?
    return m_gpu.surface_formats[0];
}

VkExtent2D Renderer::SelectOptimalSwapchainExtent()
{
    const auto caps = m_gpu.surface_caps;

    // A width and height of UINT32_MAX is a special case which indicates
    // that the surface extent will be determined by the swapchain extent
    if (caps.currentExtent.width == UINT32_MAX && caps.currentExtent.height) return caps.maxImageExtent;

    return caps.currentExtent;
}

u32 Renderer::SelectOptimalSwapchainImageCount()
{
    const auto caps = m_gpu.surface_caps;

    // A maximum image count of zero is a special case which indicates
    // that there is no maximum image limit, therefore we use (minimum + 1)
    if (caps.maxImageCount == 0) return caps.minImageCount + 1;

    // We will attempt to use (minimum + 1) images so that we will always have
    // a free image available for rendering
    return std::min(caps.minImageCount + 1, caps.maxImageCount);
}

} // namespace vker