#pragma once
#include "src/vulkan/vkHeader.hpp"

struct Attachment {
	std::shared_ptr<Texture> texture;
	bool resolve = false;
};

class RenderPass {
public:
	RenderPass(std::initializer_list<Attachment> attachmentInfos, bool clear, glm::vec4 clearColor);
	~RenderPass();

	VkRenderPass getHandle() const { return m_handle; }
	std::vector<VkClearValue>& getClearValues() { return m_clearValues; }

private:
	VkRenderPass m_handle;
	std::vector<VkClearValue> m_clearValues;
};