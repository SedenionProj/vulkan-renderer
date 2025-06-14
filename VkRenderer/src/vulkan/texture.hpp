#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Device;

class Texture2D {
public:
	Texture2D(VkImage image, VkImageView imageView, uint32_t width, uint32_t height);
	Texture2D(std::filesystem::path path);
	Texture2D(uint32_t width, uint32_t height);
	~Texture2D();

	VkImageView getImageView() { return m_imageView; }
	VkSampler getSampler() { return m_sampler; }
	uint32_t getWidth() { return m_width; }
	uint32_t getHeight() { return m_height; }

	void createImage(VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
	void createImageView(VkFormat format, VkImageAspectFlags aspectFlags);
private:
	void createSampler();
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

private:
	VkImage m_image = VK_NULL_HANDLE;
	VkImageView m_imageView = VK_NULL_HANDLE;
	VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;
	VkSampler m_sampler = VK_NULL_HANDLE;

	uint32_t m_width;
	uint32_t m_height;
};