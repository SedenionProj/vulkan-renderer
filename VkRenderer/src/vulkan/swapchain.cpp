#include "src/window.hpp"
#include "src/vulkan/swapchain.hpp"
#include "src/vulkan/texture.hpp"
#include "src/vulkan/device.hpp"
#include "src/vulkan/commandBuffer.hpp"
#include "src/vulkan/syncObjects.hpp"

#include <GLFW/glfw3.h>

Swapchain::Swapchain(Window& window)
	:  m_window(window) {
	VK_CKECK(glfwCreateWindowSurface(Context::get()->getInstance(), window.getHandle(), nullptr, &m_surface));
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_frameData[i].commandPool = std::make_shared<CommandPool>();
		m_frameData[i].commandBuffer = std::make_shared<CommandBuffer>(m_frameData[i].commandPool->getHandle());
	}
	//VkBool32 presentSupport = false; // todo
	//vkGetPhysicalDeviceSurfaceSupportKHR(device->getPhysicalDevice().get(), i, m_surface, &presentSupport);
	createSwapchain();
}

Swapchain::~Swapchain() {
	vkDestroySwapchainKHR(Device::getHandle(), m_swapchain, nullptr);
	vkDestroySurfaceKHR(Context::get()->getInstance(), m_surface, nullptr);
}

void Swapchain::present() {	
	VkSemaphore waitSemaphore = getCurrentCommandBuffer()->m_renderFinishedSemaphores->getHandle();

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &waitSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_swapchain;
	presentInfo.pImageIndices = &m_currentImageIndex;

	VkResult result = vkQueuePresentKHR(Device::get()->getPresentQueue(), &presentInfo);

	m_currentFrameIndex = (m_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;

}

void Swapchain::acquireNexImage() {
	vkAcquireNextImageKHR(Device::getHandle(), m_swapchain, UINT64_MAX,getCurrentCommandBuffer()->m_imageAvailableSemaphores->getHandle(), VK_NULL_HANDLE, &m_currentImageIndex);
}

void Swapchain::createSwapchain() {
	PhysicalDevice& fd = Device::get()->getPhysicalDevice();

	SwapchainSupportDetails swapchainSupport = querySwapchainSupport(fd.getHandle());

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapchainSupport.capabilities);

	uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;

	if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount) {
		imageCount = swapchainSupport.capabilities.maxImageCount;
	}

	QueueFamilyIndices indices = fd.findQueueFamilies(fd.getHandle());
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };


	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VK_CKECK(vkCreateSwapchainKHR(Device::getHandle(), &createInfo, nullptr, &m_swapchain));

	std::vector<VkImage> swapchainImages;
	vkGetSwapchainImagesKHR(Device::getHandle(), m_swapchain, &imageCount, nullptr);
	swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(Device::getHandle(), m_swapchain, &imageCount, swapchainImages.data());

	m_swapchainImageFormat = surfaceFormat.format;
	m_swapchainExtent = extent;

	// create swapchain textures

	m_swapchainTexturesCount = swapchainImages.size();

	for (size_t i = 0; i < m_swapchainTexturesCount; i++) {
		VkImageView imageView; // todo: use helper function

		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapchainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_swapchainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		VK_CKECK(vkCreateImageView(Device::getHandle(), &createInfo, nullptr, &imageView));

		m_swapchainTextures.emplace_back(std::make_shared<Texture2D>(TextureType::SWAPCHAIN, swapchainImages[i], imageView, m_swapchainExtent.width, m_swapchainExtent.height, m_swapchainImageFormat));
	}
}

SwapchainSupportDetails Swapchain::querySwapchainSupport(VkPhysicalDevice device) {
	SwapchainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

VkSurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR Swapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	} else {

		VkExtent2D actualExtent = {
			m_window.m_data.width,
			m_window.m_data.height
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

