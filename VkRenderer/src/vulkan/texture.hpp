#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Device;

class Texture2D {
public:
	Texture2D(VkImage image, VkImageView imageView, uint32_t width, uint32_t height);
	Texture2D(std::filesystem::path path);
	Texture2D(uint32_t width, uint32_t height, VkFormat format);
	~Texture2D();

	VkImageView getImageView() { return m_imageView; }
	VkSampler getSampler() { return m_sampler; }
	uint32_t getWidth() { return m_width; }
	uint32_t getHeight() { return m_height; }

	void createImage(VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT);
	void createImageView( VkImageAspectFlags aspectFlags);
	void generateMipmaps();
private:
	void createSampler();
	void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(VkBuffer buffer, uint32_t width, uint32_t height);

private:
	VkImage m_image = VK_NULL_HANDLE;
	VkImageView m_imageView = VK_NULL_HANDLE;
	VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;
	VkSampler m_sampler = VK_NULL_HANDLE;

	uint32_t m_width;
	uint32_t m_height;

	uint32_t m_mipLevels = 1;

	VkFormat m_format = VK_FORMAT_UNDEFINED;
};