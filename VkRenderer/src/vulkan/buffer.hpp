#pragma once

#include "vulkan/vulkan.h"

class Device;

class Buffer {
public:
	Buffer(std::shared_ptr<Device> device);
	~Buffer();

	void createBuffer(uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	VkBuffer getHandle() { return m_handle; }
	VkDeviceMemory getMemory() { return m_memory; }
protected:
	VkBuffer m_handle = VK_NULL_HANDLE;
	VkDeviceMemory m_memory = VK_NULL_HANDLE;

	std::shared_ptr<Device> m_device;
};

class VertexBuffer : public Buffer {
public:
	VertexBuffer(std::shared_ptr<Device> device, uint32_t size, const void* vData);
};

class IndexBuffer : public Buffer {
public:
	IndexBuffer(std::shared_ptr<Device> device, uint32_t size, const void* vData);
};

class UniformBuffer : public Buffer {
public:
	UniformBuffer(std::shared_ptr<Device> device, uint32_t size);
	void setData(void* data, uint32_t size);
private:
	void* m_mapped;
};