#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
class Shader {
public:
	Shader();
	~Shader();

	VkShaderModule createShaderModule(const std::vector<char>& code);

	VkPipelineShaderStageCreateInfo shaderStages[2];
};