#pragma once

#include "renderer.h"
#include "window.h"

namespace vker {

class Engine {
public:
	Engine();

	void Setup();
	void Run();

private:
	Window m_window;
	Renderer m_renderer;
};

} // namespace vker