#include "src/vulkan/syncObjects.hpp"
#include "src/vulkan/device.hpp"

Semaphore::Semaphore()
{
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	if (vkCreateSemaphore(Device::getHandle(), &semaphoreInfo, nullptr, &m_handle) != VK_SUCCESS){
		throw std::runtime_error("failed to create semaphore");
	}
}

Semaphore::~Semaphore()
{
	vkDestroySemaphore(Device::getHandle(), m_handle, nullptr);
}

Fence::Fence()
{
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	if (vkCreateFence(Device::getHandle(), &fenceInfo, nullptr, &m_handle) != VK_SUCCESS) {
		throw std::runtime_error("failed to create fence");
	}
}

Fence::~Fence()
{
	vkDestroyFence(Device::getHandle(), m_handle, nullptr);
}

void Fence::wait()
{
	vkWaitForFences(Device::getHandle(), 1, &m_handle, VK_TRUE, UINT64_MAX);
}

void Fence::reset()
{
	vkResetFences(Device::getHandle(), 1, &m_handle);
}
