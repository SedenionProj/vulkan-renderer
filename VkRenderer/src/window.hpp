#pragma once

class Context;
class Swapchain;
struct GLFWwindow;

class Window {
public:
	Window();
	~Window();

	GLFWwindow* getHandle() const { return m_handle; }
	std::shared_ptr<Swapchain> getSwapchain() const { return m_swapchain; }

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