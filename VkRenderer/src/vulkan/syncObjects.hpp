#pragma once
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
class Semaphore {
public:
	Semaphore();
	~Semaphore();

	VkSemaphore getHandle() { return m_handle; }

private:
	VkSemaphore m_handle;
};

class Fence {
public:
	Fence();
	~Fence();

	VkFence getHandle() { return m_handle; }

	bool isSignaled() { return m_signaled; }

	void wait();
	void reset();

private:
	VkFence m_handle;

	bool m_signaled= true;
};