#pragma once

#include <functional>
#include <string>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "types.h"

namespace vker {

class Window {
public:
	Window(int width, int height, const std::string& title);
	~Window();

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	void Update();

	inline bool ShouldClose() const { return glfwWindowShouldClose(m_window); }

	const char ** QueryInstanceExtensions(u32 &count) const;
	void CreateSurface(VkInstance instance, VkSurfaceKHR *surface) const;

	int GetKeyState(int key) const;
	int GetMouseButton(int button) const;

	void GetCursor(double& x, double& y) const;

	void SetInputMode(int mode, int value);

	using ResizeCb = std::function<void(int, int)>;

	inline ResizeCb GetResizeCallback() const { return m_resize_cb; }
	inline void SetResizeCallback(ResizeCb cb) { m_resize_cb = cb; }

private:
	GLFWwindow *m_window;
	ResizeCb m_resize_cb;
};

} // namespace vker