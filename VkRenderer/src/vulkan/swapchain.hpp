#pragma once

#define MAX_FRAMES_IN_FLIGHT 2

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Texture2D;
class Window;
class Context;
class Device;
class CommandBuffer;
class CommandPool;

struct SwapchainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class Swapchain {
	struct FrameData {
		std::shared_ptr<CommandPool> commandPool;
		std::shared_ptr<CommandBuffer> commandBuffer;
	};

public:
	Swapchain(Window& window);
	~Swapchain();

	void present();
	void acquireNexImage();

	VkSwapchainKHR getSwapchain() const { return m_swapchain; }
	uint32_t getSwapchainTexturesCount() const { return m_swapchainTexturesCount; }
	uint32_t getCurrentFrameIndex() const { return m_currentFrameIndex; }
	uint32_t getCurrentImageIndex() const { return m_currentImageIndex; }
	std::shared_ptr<CommandBuffer> getCurrentCommandBuffer() const { return m_frameData[m_currentFrameIndex].commandBuffer; }
private:
	void createSwapchain();
	SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
public:
	uint32_t m_swapchainTexturesCount;

	VkSurfaceKHR m_surface = VK_NULL_HANDLE;
	VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
	VkFormat m_swapchainImageFormat;
	VkExtent2D m_swapchainExtent;

	uint32_t m_currentFrameIndex = 0;
	uint32_t m_currentImageIndex;

	std::vector<std::shared_ptr<Texture2D>> m_swapchainTextures;
	FrameData m_frameData[MAX_FRAMES_IN_FLIGHT];

	Window& m_window;
};