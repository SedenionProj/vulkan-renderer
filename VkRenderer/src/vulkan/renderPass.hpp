#pragma once

#include "src/vulkan/swapchain.hpp"

class RenderPass {
public:
	RenderPass();
	~RenderPass();

	VkRenderPass getHandle() { return m_handle; }

private:
	void createRenderPass();

private:
	VkRenderPass m_handle;
};