#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define ASSETS_PATH "../../../VkRenderer/assets/"

struct DescriptorInfo {
	VkDescriptorType type;
	VkShaderStageFlags shaderStage;
	uint32_t size;
	uint32_t binding;
	uint32_t set;
};

class Shader {
public:
	Shader(const char* vertPath, const char* fragPath);
	~Shader();

	VkShaderModule createShaderModule(const std::vector<char>& code);
	std::vector<VkVertexInputAttributeDescription>& getAttributeDescriptions() { return m_attributeDescriptions; }
	std::vector<VkDescriptorSetLayout>& getDescriptorSetLayouts() { return m_descriptorSetLayouts; }
	std::vector<DescriptorInfo>& getDescriptorInfos() { return m_descriptorInfos; }
	uint32_t getVertexInputStride() { return m_vertexInputStride; }
	VkPipelineLayout getPipelineLayout() { return m_pipelineLayout; }
	
	VkPipelineShaderStageCreateInfo m_shaderStages[2];

private:
	void loadData(std::vector<char>& code, VkShaderStageFlags stage);
	void createPipelineLayout();

private:
	std::vector<VkVertexInputAttributeDescription> m_attributeDescriptions{};
	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;	// one layout per set
	std::vector<DescriptorInfo> m_descriptorInfos;
	uint32_t m_vertexInputStride = 0;

	VkPipelineLayout m_pipelineLayout;
};