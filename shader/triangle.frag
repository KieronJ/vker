#version 450

layout (location = 0) in vec2 fTex;

layout (location = 0) out vec4 oCol;

layout (binding = 1) uniform sampler2D textureSampler;

void main()
{
	oCol = texture(textureSampler, fTex);
}