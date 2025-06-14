#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Context;
class Swapchain;

class Window {
public:
	Window();
	~Window();

	GLFWwindow* getHandle() { return m_handle; }
	std::shared_ptr<Swapchain> getSwapchain(){ return m_swapchain; }

	struct Data {
		uint32_t width;
		uint32_t height;
		bool resized = false;
	};
	
	Data m_data;

private:
	GLFWwindow* m_handle;
	std::shared_ptr<Swapchain> m_swapchain;
};