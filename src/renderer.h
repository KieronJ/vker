#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "contrib/vk_mem_alloc.h"

#include "buffer.h"
#include "camera.h"
#include "image.h"
#include "model.h"
#include "types.h"
#include "window.h"

namespace vker {

class Renderer {
public:
	Renderer(const Window &window);
	~Renderer();

	void Draw(const Camera& cam);
	void Present();

	inline void InvalidateSwapchain() { m_swapchain.valid = false; }

	inline Model& CreateModel() { return m_models.emplace_back(m_allocator); }

private:
	void CreateInstance(const Window &window);

#ifndef NDEBUG
	static VkBool32 VKAPI_CALL DebugMessengerCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT type,
		const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
		void* user_data);

	void CreateDebugMessenger();
	void DestroyDebugMessenger();

	VkDebugUtilsMessengerEXT m_debug_messenger;
#endif // NDEBUG

	void EnumeratePhysicalDevices();

	void SelectPhysicalDevice();
	void CreateDevice();
	void CreateAllocator();
	void CreateSwapchain();
	void CreateRenderPass();
	void CreateDescriptorPool();
	void CreatePipeline();
	void CreateFramebuffers();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateSemaphores();
	void CreateFences();

	void CreateUniformBuffer();
	void CreateDepthBuffers();

	Image m_texture;
	void CreateTexture();

	void SelectOptimalPhysicalDevice(VkPhysicalDeviceType type);

	VkSurfaceFormatKHR SelectOptimalSwapchainFormat();
	VkExtent2D SelectOptimalSwapchainExtent();
	u32 SelectOptimalSwapchainImageCount();

	Buffer m_uniform_buffer;
	void *m_uniform_buffer_addr;

	std::vector<Image> m_depth_buffers;

	std::vector<Model> m_models;

	VkInstance m_instance;
	VkSurfaceKHR m_surface;

	struct gpu_info {
		VkPhysicalDevice device;

		VkPhysicalDeviceProperties props;
		VkPhysicalDeviceMemoryProperties memory_props;

		std::vector<VkExtensionProperties> extension_props;
		std::vector<VkQueueFamilyProperties> queue_family_props;

		VkSurfaceCapabilitiesKHR surface_caps;
		std::vector<VkSurfaceFormatKHR> surface_formats;
		std::vector<VkPresentModeKHR> surface_present_modes;
	};

	std::vector<gpu_info> m_gpus;

	gpu_info m_gpu;
	VkPhysicalDevice m_physical_device;
	u32 m_queue_family;

	VkDevice m_device;
	VkQueue m_queue;

	VmaAllocator m_allocator;

	struct Swapchain {
		VkSwapchainKHR swapchain;

		std::vector<VkImage> images;
		std::vector<VkImageView> image_views;
		std::vector<VkFramebuffer> framebuffers;

		VkExtent2D extent;
		VkSurfaceFormatKHR format;
		u32 image_count;

		bool valid;
	};

	u32 m_frame_index;

	Swapchain m_swapchain;
	
	VkRenderPass m_render_pass;

	VkPipelineLayout m_pipeline_layout;
	VkPipeline m_pipeline;
	VkDescriptorSetLayout m_descriptor_set_layout;

	VkDescriptorPool m_descriptor_pool;
	VkDescriptorSet m_uniform_descriptor_set;

	VkCommandPool m_command_pool;
	std::vector<VkCommandBuffer> m_command_buffers;

	u32 m_semaphores_index = 0;
	std::vector<VkSemaphore> m_image_available_semaphores;
	std::vector<VkSemaphore> m_render_finished_semaphores;
	std::vector<VkFence> m_fences;
};

} // namespace vker