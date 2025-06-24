#include "src/vulkan/descriptorSet.hpp"
#include "src/vulkan/swapchain.hpp"
#include "src/vulkan/device.hpp"
#include "src/vulkan/shader.hpp"
#include "src/vulkan/buffer.hpp"
#include "src/vulkan/texture.hpp"

DescriptorSet::DescriptorSet(std::shared_ptr<Shader> shader, uint32_t set)
	: m_shader(shader), m_set(set) {
	createDescriptorPool();
	createDescriptorSets();
}

void DescriptorSet::createDescriptorPool() {
	std::vector<VkDescriptorPoolSize> poolSizes;
	for (auto& info : m_shader->getDescriptorInfos()) {
		if (info.set == m_set) {
			VkDescriptorPoolSize poolSize{};
			poolSize.type = info.type;
			poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

			poolSizes.push_back(poolSize);
		}
	}

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	if (vkCreateDescriptorPool(Device::getHandle(), &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void DescriptorSet::createDescriptorSets() {
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_shader->getDescriptorSetLayouts()[m_set]);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(Device::getHandle(), &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}
}

void DescriptorSet::update(std::vector<UniformBuffer>& uniformBuffers, std::shared_ptr<Texture2D> texture, uint32_t i) {
	
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffers[i].getHandle();
		bufferInfo.offset = 0;
		bufferInfo.range = uniformBuffers[i].getSize();

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = texture->getImageView();
		imageInfo.sampler = texture->getSampler();

		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = m_descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = m_descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(Device::getHandle(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void DescriptorSet::setUniform(std::vector<UniformBuffer>& uniformBuffers, uint32_t binding)
{
	for (int i = 0; i < m_descriptorSets.size(); i++) {
		setUniform(uniformBuffers, binding, i);
	}
}

void DescriptorSet::setUniform(std::vector<UniformBuffer>& uniformBuffers, uint32_t binding, uint32_t frameIndex)
{
	VkDescriptorBufferInfo info{};
	info.buffer = uniformBuffers[frameIndex].getHandle();
	info.offset = 0;
	info.range = uniformBuffers[frameIndex].getSize();

	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = m_descriptorSets[frameIndex];
	descriptorWrite.dstBinding = binding;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo = &info;
	vkUpdateDescriptorSets(Device::getHandle(), 1, &descriptorWrite, 0, nullptr);
}

void DescriptorSet::setTexture(std::shared_ptr<Texture2D> texture, uint32_t binding)
{
	for (int i = 0; i < m_descriptorSets.size(); i++) {
		VkDescriptorImageInfo info{};
		info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		info.imageView = texture->getImageView();
		info.sampler = texture->getSampler();

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_descriptorSets[i];
		descriptorWrite.dstBinding = binding;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = &info;
		vkUpdateDescriptorSets(Device::getHandle(), 1, &descriptorWrite, 0, nullptr);
	}
}