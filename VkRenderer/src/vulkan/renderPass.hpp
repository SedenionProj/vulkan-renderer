#pragma once

#include "src/vulkan/swapchain.hpp"

class Texture;

struct Attachment {
	enum class Type {
		NONE,
		COLOR,
		DEPTH,
		PRESENT
	};

	std::shared_ptr<Texture> texture;
	Type type;
	uint32_t binding;
	bool resolve = false;
};

class RenderPass {
public:
	RenderPass(std::initializer_list<Attachment> attachmentInfos, bool clear);
	~RenderPass();

	VkRenderPass getHandle() const { return m_handle; }

private:
	VkRenderPass m_handle;
};