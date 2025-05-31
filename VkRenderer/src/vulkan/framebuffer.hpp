#pragma once

#include "src/vulkan/texture.hpp"
#include "src/vulkan/renderPass.hpp"

class Framebuffer {
public:
	Framebuffer(const std::vector< std::shared_ptr<Texture2D>>& textures, std::shared_ptr<RenderPass> renderPass);
	~Framebuffer();

	VkFramebuffer getHandle() { return m_handle; }

private:
	VkFramebuffer m_handle;
};
