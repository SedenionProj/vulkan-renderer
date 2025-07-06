#include "src/vulkan/framebuffer.hpp"
#include "src/vulkan/device.hpp"

Framebuffer::Framebuffer(const std::vector< std::shared_ptr<Texture>>& textures, std::shared_ptr<RenderPass> renderPass) {

	std::vector<VkImageView> attachments;
	attachments.reserve(textures.size());
	for (auto tex : textures) {
		attachments.emplace_back(tex->getImageView());
	}

	VkFramebufferCreateInfo framebufferInfo{};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = renderPass->getHandle();
	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	framebufferInfo.pAttachments = attachments.data();
	framebufferInfo.width = textures.back()->getWidth();
	framebufferInfo.height = textures.back()->getHeight();
	framebufferInfo.layers = 1;

	if (vkCreateFramebuffer(Device::getHandle(), &framebufferInfo, nullptr, &m_handle) != VK_SUCCESS) {
		throw std::runtime_error("failed to create framebuffer!");
	}
}

Framebuffer::~Framebuffer() {
	vkDestroyFramebuffer(Device::getHandle(), m_handle, nullptr);
}