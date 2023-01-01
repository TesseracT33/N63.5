#version 450
#extension GL_KHR_vulkan_glsl: enable

layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 0) uniform sampler2D uTexture;
layout(location = 0) in vec2 vUV;

void main()
{
	FragColor = textureLod(uTexture, vUV, 0.0);
}