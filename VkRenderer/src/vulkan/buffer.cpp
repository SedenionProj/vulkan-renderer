#include "src/vulkan/buffer.hpp"
#include "src/vulkan/device.hpp"
#include "src/vulkan/commandBuffer.hpp"


Buffer::Buffer(std::shared_ptr<Device> device)
	: m_device(device) {
}

Buffer::~Buffer() {
	vkDestroyBuffer(Device::getHandle(), m_handle, nullptr);
	vkFreeMemory(Device::getHandle(), m_memory, nullptr);
}

void Buffer::createBuffer(uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(Device::getHandle(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(Device::getHandle(), buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(Device::getHandle(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(Device::getHandle(), buffer, bufferMemory, 0);
}

void Buffer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	CommandPool commandPool(m_device);
	CommandBuffer commandBuffer(commandPool.getHandle());
	commandBuffer.beginRecording();

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer.getHandle(), srcBuffer, dstBuffer, 1, &copyRegion);

	commandBuffer.endRecording();

	commandBuffer.submit(m_device, false);

	vkQueueWaitIdle(m_device->m_graphicsQueue); // temp
}

uint32_t Buffer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_device->getPhysicalDevice().getHandle(), &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

// vertex buffer
VertexBuffer::VertexBuffer(std::shared_ptr<Device> device, uint32_t size, const void* vData)
	: Buffer(device) {
	VkDeviceSize bufferSize = size;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory
	);

	void* data;
	vkMapMemory(Device::getHandle(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vData, (size_t)bufferSize);
	vkUnmapMemory(Device::getHandle(), stagingBufferMemory);

	createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_handle,
		m_memory
	);

	copyBuffer(stagingBuffer, m_handle, bufferSize);
	vkDestroyBuffer(Device::getHandle(), stagingBuffer, nullptr);
	vkFreeMemory(Device::getHandle(), stagingBufferMemory, nullptr);
}

// index buffer
IndexBuffer::IndexBuffer(std::shared_ptr<Device> device, uint32_t size, const void* vData)
	: Buffer(device) {
	VkDeviceSize bufferSize = size;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory
	);

	void* data;
	vkMapMemory(Device::getHandle(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vData, (size_t)bufferSize);
	vkUnmapMemory(Device::getHandle(), stagingBufferMemory);

	createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_handle,
		m_memory
	);

	copyBuffer(stagingBuffer, m_handle, bufferSize);
	vkDestroyBuffer(Device::getHandle(), stagingBuffer, nullptr);
	vkFreeMemory(Device::getHandle(), stagingBufferMemory, nullptr);
}

// uniform buffer
UniformBuffer::UniformBuffer(std::shared_ptr<Device> device, uint32_t size)
	: Buffer(device) {
	VkDeviceSize bufferSize = size;

	createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_handle, m_memory);

	vkMapMemory(Device::getHandle(), m_memory, 0, bufferSize, 0, &m_mapped);
}

void UniformBuffer::setData(void* data, uint32_t size)
{
	memcpy(m_mapped, data, size);
}
