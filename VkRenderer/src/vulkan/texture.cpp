#include "src/vulkan/texture.hpp"

Texture2D::Texture2D(VkImage image, VkImageView imageView, uint32_t width, uint32_t height)
	: m_image(image), m_imageView(imageView), m_width(width), m_height(height){

}