#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Device;

enum class TextureType {
	COLOR,
	DEPTH
};

class Texture {
public:
	Texture() = default;
	virtual ~Texture() {}

	uint32_t getWidth() const { return m_width; }
	uint32_t getHeight() const { return m_height; }
	VkImageView getImageView() const { return m_imageView; }
	VkSampler getSampler() const { return m_sampler; }
	VkSampleCountFlagBits getSampleCount() const { return m_sampleCount; }
	VkFormat getFormat() const { return m_format; }

	void createImage(VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
	void createImageView(VkImageAspectFlags aspectFlags);
	void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
	void createSampler();
protected:
	void copyBufferToImage(VkBuffer buffer, uint32_t width, uint32_t height);

	uint32_t m_width = 0;
	uint32_t m_height = 0;
	VkImage m_image = VK_NULL_HANDLE;
	VkImageView m_imageView = VK_NULL_HANDLE;
	VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;
	VkSampler m_sampler = VK_NULL_HANDLE;

	uint32_t m_mipLevels = 1;
	uint32_t m_layerCount = 1;
	VkFormat m_format = VK_FORMAT_UNDEFINED;
	VkSampleCountFlagBits m_sampleCount = VK_SAMPLE_COUNT_1_BIT;
};

class Texture2D : public Texture {
public:
	Texture2D(VkImage image, VkImageView imageView, uint32_t width, uint32_t height, VkFormat format);
	Texture2D(std::filesystem::path path);
	Texture2D(uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT);
	~Texture2D();



	
	void generateMipmaps();
};

class CubeMap : public Texture {
public:
	CubeMap(const char** paths);
};