#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Device;

enum class TextureType {
	COLOR,
	DEPTH
};

class Texture2D {
public:
	Texture2D(VkImage image, VkImageView imageView, uint32_t width, uint32_t height, VkFormat format);
	Texture2D(std::filesystem::path path);
	Texture2D(uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT);
	~Texture2D();

	VkImageView getImageView() const { return m_imageView; }
	VkSampler getSampler() const { return m_sampler; }
	uint32_t getWidth() const { return m_width; }
	uint32_t getHeight() const { return m_height; }
	VkSampleCountFlagBits getSampleCount() const { return m_sampleCount; }
	VkFormat getFormat() const { return m_format; }

	void createSampler();
	void createImage(VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
	void createImageView( VkImageAspectFlags aspectFlags);
	void generateMipmaps();
private:
	
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
	VkSampleCountFlagBits m_sampleCount = VK_SAMPLE_COUNT_1_BIT;
};