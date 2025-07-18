#include "src/vulkan/buffer.hpp"
#include "src/vulkan/commandBuffer.hpp"
#include "src/vulkan/device.hpp"

Buffer::Buffer() {
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

	VK_CHECK(vkCreateBuffer(Device::getHandle(), &bufferInfo, nullptr, &buffer));

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(Device::getHandle(), buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = Context::get()->getDevice()->getPhysicalDevice().findMemoryType(memRequirements.memoryTypeBits, properties);

	VK_CHECK(vkAllocateMemory(Device::getHandle(), &allocInfo, nullptr, &bufferMemory));

	vkBindBufferMemory(Device::getHandle(), buffer, bufferMemory, 0);
}

void Buffer::createBuffer(uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
	createBuffer(size, usage, properties, m_handle, m_memory);
}

void Buffer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	CommandPool commandPool;
	CommandBuffer commandBuffer(commandPool.getHandle());
	commandBuffer.beginRecording();

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer.getHandle(), srcBuffer, dstBuffer, 1, &copyRegion);

	commandBuffer.endRecording();

	commandBuffer.submit(false);

	vkQueueWaitIdle(Device::get()->getGraphicsQueue()); // temp
}



// vertex buffer
VertexBuffer::VertexBuffer(uint32_t size, const void* vData) {
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

	// todo
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
IndexBuffer::IndexBuffer(uint32_t size, const void* vData) {
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
UniformBuffer::UniformBuffer( uint32_t size)
	: m_size(size) {
	VkDeviceSize bufferSize = size;

	createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_handle, m_memory);

	vkMapMemory(Device::getHandle(), m_memory, 0, bufferSize, 0, &m_mapped);
}

void UniformBuffer::setData(void* data, uint32_t size)
{
	memcpy(m_mapped, data, size);
}
