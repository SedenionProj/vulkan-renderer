#pragma once
#include "src/vulkan/vkHeader.hpp"

class Texture {
public:
	Texture() = default;
	virtual ~Texture();

	uint32_t getWidth() const { return m_width; }
	uint32_t getHeight() const { return m_height; }
	VkImageView getImageView() const { return m_imageView; }
	VkSampler getSampler() const { return m_sampler; }
	VkSampleCountFlagBits getSampleCount() const { return m_sampleCount; }
	VkFormat getFormat() const { return m_format; }
	VkImageLayout getLayout() const { return m_layout; }
	TextureType getType() const { return m_type; }

	void createImage(VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
	void createImageView(VkImageAspectFlags aspectFlags);
	void transitionImageLayout(VkImageLayout newLayout);
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
	VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	TextureType m_type = TextureType::NONE;
};

class Texture2D : public Texture {
public:
	Texture2D(TextureType type, VkImage image, VkImageView imageView, uint32_t width, uint32_t height, VkFormat format);
	Texture2D(std::filesystem::path path);
	Texture2D(TextureType type, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT);
	
	void generateMipmaps();
};

class DepthTexture : public Texture2D {
public:
	DepthTexture(uint32_t width, uint32_t height, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT);
};

class CubeMap : public Texture {
public:
	CubeMap(const char** paths);
};