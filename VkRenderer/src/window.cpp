#include "src/window.hpp"
#include "src/vulkan/context.hpp"
#include "src/vulkan/swapchain.hpp"

#include <GLFW/glfw3.h>

Window::Window() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_handle = glfwCreateWindow(1280, 720, "Renderer", nullptr, nullptr);

	int w = 0, h = 0;
	glfwGetFramebufferSize(m_handle, &w, &h);

	m_data.width = w;
	m_data.height = h;

	glfwSetWindowUserPointer(m_handle, &m_data);
	glfwSetWindowSizeCallback(m_handle, [](GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<Window::Data*>(glfwGetWindowUserPointer(window));
		int w, h;
		glfwGetFramebufferSize(window, &w, &h);
		app->width = w;
		app->height = h;
		app->resized = true;
		});

	Context::create();
	m_swapchain = std::make_shared<Swapchain>(*this);
}

Window::~Window() {
	m_swapchain.reset();
	delete Context::get();
	glfwDestroyWindow(m_handle);
	glfwTerminate();
}