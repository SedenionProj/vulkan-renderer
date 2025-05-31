#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "src/vulkan/window.hpp"
#include "src/vulkan/context.hpp"
#include "src/vulkan/device.hpp"

class Texture2D;

struct SwapchainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class Swapchain {
public:
	Swapchain(std::shared_ptr<Context> ctx, std::shared_ptr<Window> window, std::shared_ptr<Device> device);
	~Swapchain();

	VkSwapchainKHR getSwapchain() { return m_swapchain; }
	uint32_t getSwapchainTexturesCount() { return m_swapchainTexturesCount; }
private:
	void createSwapchain();
	SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
public:
	uint32_t m_swapchainTexturesCount;

	VkSurfaceKHR m_surface;
	VkSwapchainKHR m_swapchain;

	std::vector<std::shared_ptr<Texture2D>> m_swapchainTextures;

	VkFormat m_swapchainImageFormat;
	VkExtent2D m_swapchainExtent;

	std::shared_ptr<Context> m_ctx;
	std::shared_ptr<Window> m_window;
	std::shared_ptr<Device> m_device;
};