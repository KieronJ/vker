#include <functional>
#include <string>

#include <fmt/printf.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include "types.h"
#include "window.h"
#include "utils.h"

namespace vker {

Window::Window(int width, int height, const std::string& title)
{
	glfwSetErrorCallback([](int error, const char *description) {
		fmt::print(stderr, "glfw error {}: {}\n", error, description);
	});

	if (!glfwInit()) FatalError("unable to init glfw");
	if (!glfwVulkanSupported()) FatalError("glfw does not support Vulkan");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

	if (!m_window) FatalError("unable to create window");
	glfwSetWindowUserPointer(m_window, this);

	glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow *window, int w, int h) {
		while (w == 0 || h == 0) {
			glfwWaitEvents();
			glfwGetFramebufferSize(window, &w, &h);
		}

		const auto wnd = static_cast<Window *>(glfwGetWindowUserPointer(window));
		if (wnd->GetResizeCallback()) wnd->GetResizeCallback()(w, h);
	});
}

Window::~Window()
{
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Window::Update()
{
	glfwPollEvents();

	if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(m_window, GLFW_TRUE);
	}
}

const char ** Window::QueryInstanceExtensions(u32 &count) const
{
	return glfwGetRequiredInstanceExtensions(&count);
}

void Window::CreateSurface(VkInstance instance, VkSurfaceKHR *surface) const
{
	VK_CHECK(glfwCreateWindowSurface(instance, m_window, nullptr, surface));
}

int Window::GetKeyState(int key) const
{
	return glfwGetKey(m_window, key);
}

int Window::GetMouseButton(int button) const
{
	return glfwGetMouseButton(m_window, button);
}

void Window::GetCursor(double& x, double& y) const
{
	glfwGetCursorPos(m_window, &x, &y);
}

void Window::SetInputMode(int mode, int value)
{
	glfwSetInputMode(m_window, mode, value);
}

} // namespace vker