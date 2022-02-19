#pragma once

namespace vker {

struct Vertex {
	struct {
		float x, y, z;
	} pos;

	struct {
		float u, v;
	} tex;
};

} // namespace vker