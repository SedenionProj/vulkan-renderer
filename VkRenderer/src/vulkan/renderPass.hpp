#pragma once

#include "src/vulkan/swapchain.hpp"

class Texture2D;

struct Attachment {
	enum class Type {
		NONE,
		COLOR,
		DEPTH,
		PRESENT,
		RESOLVE
	};

	std::shared_ptr<Texture2D> texture;
	Type type;
	uint32_t binding;
	bool resolve = false;
};

class RenderPass {
public:
	RenderPass(std::initializer_list<Attachment> attachmentInfos);
	~RenderPass();

	VkRenderPass getHandle() const { return m_handle; }

private:
	VkRenderPass m_handle;
};