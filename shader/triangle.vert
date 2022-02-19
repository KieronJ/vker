#version 450

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec2 vTex;

layout (location = 0) out vec2 fTex;

layout (set = 0, binding = 0) uniform bufferVals {
    mat4 mvp;
} myBufferVals;

void main()
{
	gl_Position = myBufferVals.mvp * vec4(vPos, 1.0);
	fTex = vTex;
}