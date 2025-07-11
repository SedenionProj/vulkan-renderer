#include "src/vulkan/texture.hpp"
#include "src/vulkan/buffer.hpp"
#include "src/vulkan/device.hpp"
#include "src/vulkan/commandBuffer.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Texture::~Texture() {
	vkDestroySampler(Device::getHandle(), m_sampler, nullptr);
	vkDestroyImageView(Device::getHandle(), m_imageView, nullptr);
	if(m_type!=TextureType::SWAPCHAIN)
		vkDestroyImage(Device::getHandle(), m_image, nullptr);
	vkFreeMemory(Device::getHandle(), m_imageMemory, nullptr);
}

void Texture::createImage(VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties) {
	// todo; use vma
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = m_width;
	imageInfo.extent.height = m_height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = m_mipLevels;
	imageInfo.format = m_format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = m_sampleCount;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.arrayLayers = m_layerCount;
	if (m_layerCount > 1) {
		imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}

	VK_CKECK(vkCreateImage(Device::getHandle(), &imageInfo, nullptr, &m_image));

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(Device::getHandle(), m_image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = Device::get()->getPhysicalDevice().findMemoryType(memRequirements.memoryTypeBits, properties);

	VK_CKECK(vkAllocateMemory(Device::getHandle(), &allocInfo, nullptr, &m_imageMemory));

	vkBindImageMemory(Device::getHandle(), m_image, m_imageMemory, 0);
}

void Texture::createImageView(VkImageAspectFlags aspectFlags) {
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = m_format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = m_mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = m_layerCount;
	if (m_layerCount > 1) {
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	}

	VK_CKECK(vkCreateImageView(Device::getHandle(), &viewInfo, nullptr, &m_imageView));
}

void Texture::transitionImageLayout(VkImageLayout newLayout) {
	CommandPool commandPool;
	CommandBuffer commandBuffer(commandPool.getHandle());
	commandBuffer.beginRecording();

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = m_layout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = m_image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = m_mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = m_layerCount;
	if (m_layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (m_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		DEBUG_ERROR("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer.getHandle(),
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	m_layout = newLayout;

	commandBuffer.endRecording();
	commandBuffer.submit(false);
	vkQueueWaitIdle(Device::get()->getGraphicsQueue()); // temp
}

void Texture::createSampler() {
	VkPhysicalDeviceProperties properties{}; // todo : move inside physical device
	vkGetPhysicalDeviceProperties(Device::get()->getPhysicalDevice().getHandle(), &properties);

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = static_cast<float>(m_mipLevels);

	VK_CKECK(vkCreateSampler(Device::getHandle(), &samplerInfo, nullptr, &m_sampler));
}

void Texture::copyBufferToImage(VkBuffer buffer, uint32_t width, uint32_t height) {
	CommandPool commandPool;
	CommandBuffer commandBuffer(commandPool.getHandle());
	commandBuffer.beginRecording();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = m_layerCount;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(
		commandBuffer.getHandle(),
		buffer,
		m_image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	commandBuffer.endRecording();
	commandBuffer.submit(false);
	vkQueueWaitIdle(Device::get()->getGraphicsQueue()); // temp
}

Texture2D::Texture2D(TextureType type, VkImage image, VkImageView imageView, uint32_t width, uint32_t height, VkFormat format) {
	m_width = width;
	m_height = height;
	m_image = image;
	m_imageView = imageView;
	m_format = format;
	m_type = type;
}

Texture2D::Texture2D(std::filesystem::path path) {
	// load texture
	int width, height, texChannels;
	stbi_uc* pixels = stbi_load(path.string().c_str(), &width, &height, &texChannels, STBI_rgb_alpha);

	m_type = TextureType::COLOR;
	m_width = static_cast<uint32_t>(width);
	m_height = static_cast<uint32_t>(height);
	m_format = VK_FORMAT_R8G8B8A8_SRGB;
	m_mipLevels = static_cast<uint32_t>(glm::floor(glm::log2(std::max(static_cast<float>(width), static_cast<float>(height))))) + 1;

	VkDeviceSize imageSize = width * height * 4;

	DEBUG_ASSERT(pixels, "failed to load texture image \"%s\"", path.string().c_str());

	Buffer stagingBuffer;
	stagingBuffer.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void* data;
	vkMapMemory(Device::getHandle(), stagingBuffer.getMemory(), 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(Device::getHandle(), stagingBuffer.getMemory());

	stbi_image_free(pixels);


	createImage(
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	// copy
	transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(stagingBuffer.getHandle(), m_width, m_height);

	createImageView( VK_IMAGE_ASPECT_COLOR_BIT);

	createSampler();

	generateMipmaps();
}

Texture2D::Texture2D(TextureType type, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits sampleCount) {
	m_width = width;
	m_height = height;
	m_format = format;
	m_sampleCount = sampleCount;
	m_type = type;
}

void Texture2D::generateMipmaps() {
	CommandPool commandPool;
	CommandBuffer commandBuffer(commandPool.getHandle());
	commandBuffer.beginRecording();

	int32_t mipWidth = m_width;
	int32_t mipHeight = m_height;

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = m_image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;
	
	for (uint32_t i = 1; i < m_mipLevels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer.getHandle(),
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer.getHandle(),
			m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer.getHandle(),
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = m_mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer.getHandle(),
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	commandBuffer.endRecording();
	commandBuffer.submit(false);
	vkQueueWaitIdle(Device::get()->getGraphicsQueue()); // temp
}

CubeMap::CubeMap(const char** paths) {
	m_type = TextureType::CUBEMAP;

	stbi_uc* textureData[6];

	for (int i = 0; i < 6; i++) {
		int width, height, texChannels;

		std::string fullPath = std::string(ASSETS_PATH) + paths[i];
		stbi_uc* pixels = stbi_load(fullPath.c_str(), &width, &height, &texChannels, STBI_rgb_alpha);
		
		DEBUG_ASSERT(pixels, "failed to load texture image \"%s\"", fullPath.c_str());

		textureData[i] = pixels;

		m_width = static_cast<uint32_t>(width);
		m_height = static_cast<uint32_t>(height);
	}
	m_layerCount = 6;
	m_format = VK_FORMAT_R8G8B8A8_SRGB;

	VkDeviceSize layerSize = m_width * m_height * 4;
	VkDeviceSize imageSize = layerSize * m_layerCount;

	Buffer stagingBuffer;
	stagingBuffer.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void* data;
	vkMapMemory(Device::getHandle(), stagingBuffer.getMemory(), 0, imageSize, 0, &data);
	for (int i = 0; i < 6; i++)
		memcpy(static_cast<uint8_t*>(data) + (layerSize * i), textureData[i], static_cast<size_t>(layerSize));
	vkUnmapMemory(Device::getHandle(), stagingBuffer.getMemory());

	for (int i = 0; i < 6; i++)
		stbi_image_free(textureData[i]);

	createImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(stagingBuffer.getHandle(), m_width, m_height);
	transitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

	createSampler();
}

DepthTexture::DepthTexture(uint32_t width, uint32_t height, VkSampleCountFlagBits sampleCount)
	: Texture2D(TextureType::DEPTH, width, height, Device::get()->getPhysicalDevice().findDepthFormat(), sampleCount) {
	createImage(VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	createImageView(VK_IMAGE_ASPECT_DEPTH_BIT);
}