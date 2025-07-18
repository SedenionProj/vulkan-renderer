#pragma once
#include "src/vulkan/vkHeader.hpp"

class Framebuffer {
public:
	Framebuffer(const std::vector<std::shared_ptr<Texture>>& textures, std::shared_ptr<RenderPass> renderPass);
	~Framebuffer();

	VkFramebuffer getHandle() const { return m_handle; }

private:
	VkFramebuffer m_handle; // todo VkFramebuffer m_handle[MAX_FRAMES_IN_FLIGHT];
};
