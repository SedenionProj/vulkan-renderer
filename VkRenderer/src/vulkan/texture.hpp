#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Texture2D {
public:
	Texture2D(VkImage image, VkImageView imageView, uint32_t width, uint32_t height);

	VkImageView getImageView() { return m_imageView; }

	uint32_t getWidth() { return m_width; }
	uint32_t getHeight() { return m_height; }

private:
	VkImage m_image;
	VkImageView m_imageView;
	uint32_t m_width;
	uint32_t m_height;
};