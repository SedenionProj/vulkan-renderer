#pragma once
#include <vulkan/vulkan.h>
#include "src/vulkan/buffer.hpp"

class Shader;
class CommandBuffer;
class Texture;

class DescriptorSet {
public:
	DescriptorSet(std::shared_ptr<Shader> shader, uint32_t set);
	VkDescriptorSet getHandle(uint32_t i) { return m_descriptorSets[i]; }

	void update(std::vector<UniformBuffer>& uniformBuffers, std::shared_ptr<Texture> texture, uint32_t i);

	void setUniform(std::vector<UniformBuffer>& uniformBuffers, uint32_t binding);
	void setUniform(std::vector<UniformBuffer>& uniformBuffers, uint32_t binding, uint32_t frameIndex);
	void setTexture(std::shared_ptr<Texture> texture, uint32_t binding);
	//void setTexture(std::shared_ptr<Texture2D> texture, uint32_t binding, uint32_t frameIndex);

	std::shared_ptr<Shader> getShader() { return m_shader; }

private:
	void createDescriptorPool();
	void createDescriptorSets();

	VkDescriptorPool m_descriptorPool;
	std::vector<VkDescriptorSet> m_descriptorSets;
	std::shared_ptr<Shader> m_shader;

	uint32_t m_set;
};