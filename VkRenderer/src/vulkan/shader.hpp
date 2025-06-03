#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

struct Vertex {
	glm::vec2 pos;
	glm::vec3 color;
};

class Shader {
public:
	Shader();
	~Shader();

	VkShaderModule createShaderModule(const std::vector<char>& code);
	std::vector<VkVertexInputAttributeDescription>& getAttributeDescriptions() { return m_attributeDescriptions; }
	VkDescriptorSetLayout getDescriptorSetLayout() { return m_descriptorSetLayout; }
	uint32_t getVertexInputStride() { return m_vertexInputStride; }
	VkPipelineLayout getPipelineLayout() { return m_pipelineLayout; }

	void createPipelineLayout();

	VkPipelineShaderStageCreateInfo m_shaderStages[2];
private:
	std::vector<VkVertexInputAttributeDescription> m_attributeDescriptions{};
	uint32_t m_vertexInputStride = 0;

	VkDescriptorSetLayout m_descriptorSetLayout;
	VkPipelineLayout m_pipelineLayout;
};