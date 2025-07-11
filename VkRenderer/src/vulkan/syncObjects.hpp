#pragma once
#include "src/vulkan/vkHeader.hpp"

class Semaphore {
public:
	Semaphore();
	~Semaphore();

	VkSemaphore getHandle() const { return m_handle; }

private:
	VkSemaphore m_handle;
};

class Fence {
public:
	Fence();
	~Fence();

	VkFence getHandle() const { return m_handle; }

	bool isSignaled() const { return m_signaled; }

	void wait();
	void reset();

private:
	VkFence m_handle;

	bool m_signaled = true;
};