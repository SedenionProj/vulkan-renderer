#pragma once

#include "src/vulkan/swapchain.hpp"

class Texture2D;

struct Attachment {
	enum class Type {
		COLOR,
		DEPTH,
		PRESENT
	};

	std::shared_ptr<Texture2D> texture;
	Type type;
	uint32_t binding;
};

class RenderPass {
public:
	RenderPass(std::initializer_list<Attachment> attachmentInfos);
	~RenderPass();

	VkRenderPass getHandle() const { return m_handle; }

private:
	VkRenderPass m_handle;
};