#pragma once
#include "src/vulkan/vkHeader.hpp"

class Buffer {
public:
	Buffer();
	~Buffer();

	void createBuffer(uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void createBuffer(uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	VkBuffer getHandle() { return m_handle; }
	VkDeviceMemory getMemory() { return m_memory; }
protected:
	VkBuffer m_handle = VK_NULL_HANDLE;
	VkDeviceMemory m_memory = VK_NULL_HANDLE;
};

class VertexBuffer : public Buffer {
public:
	VertexBuffer(uint32_t size, const void* vData);
};

class IndexBuffer : public Buffer {
public:
	IndexBuffer(uint32_t size, const void* vData);
};

class UniformBuffer : public Buffer {
public:
	UniformBuffer(uint32_t size);
	void setData(void* data, uint32_t size);
	uint32_t getSize() { return m_size; }
private:
	uint32_t m_size;
	void* m_mapped;
};