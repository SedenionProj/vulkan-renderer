#include "src/vulkan/syncObjects.hpp"
#include "src/vulkan/device.hpp"

Semaphore::Semaphore()
{
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VK_CKECK(vkCreateSemaphore(Device::getHandle(), &semaphoreInfo, nullptr, &m_handle));
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
	VK_CKECK(vkCreateFence(Device::getHandle(), &fenceInfo, nullptr, &m_handle));
}

Fence::~Fence()
{
	vkDestroyFence(Device::getHandle(), m_handle, nullptr);
}

void Fence::wait()
{
	if (vkWaitForFences(Device::getHandle(), 1, &m_handle, VK_TRUE, UINT64_MAX) == VK_SUCCESS) {
		m_signaled = true;
	}
	else {
		m_signaled = false;
	}
}

void Fence::reset()
{
	if (m_signaled)
		vkResetFences(Device::getHandle(), 1, &m_handle);
	m_signaled = false;
}