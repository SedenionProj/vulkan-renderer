#pragma once
#include "src/vulkan/vkHeader.hpp"

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

	void destroy();

	VkShaderModule createShaderModule(const std::vector<char>& code);
	void pushConstants(VkCommandBuffer cmdBuf, const void* fullDataBlock);

	std::vector<VkVertexInputAttributeDescription>& getAttributeDescriptions() { return m_attributeDescriptions; }
	std::vector<VkDescriptorSetLayout>& getDescriptorSetLayouts() { return m_descriptorSetLayouts; }
	std::vector<DescriptorInfo>& getDescriptorInfos() { return m_descriptorInfos; }
	uint32_t getVertexInputStride() const { return m_vertexInputStride; }
	VkPipelineLayout getPipelineLayout() const { return m_pipelineLayout; }
	
	VkPipelineShaderStageCreateInfo m_shaderStages[2];

private:
	void loadData(std::vector<char>& code, VkShaderStageFlags stage);
	void createPipelineLayout();

private:
	std::vector<VkVertexInputAttributeDescription> m_attributeDescriptions{};
	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;	// one layout per set
	std::vector<DescriptorInfo> m_descriptorInfos;
	std::vector<VkPushConstantRange> m_pushConstantRanges;
	uint32_t m_vertexInputStride = 0;

	VkPipelineLayout m_pipelineLayout;
};