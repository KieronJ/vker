#include <fmt/printf.h>

#include "engine.h"
#include "window.h"

int main()
{
	vker::Engine engine{};
	engine.Setup();
	engine.Run();

	return 0;
}