#pragma once

#include "src/vulkan/swapchain.hpp"

class RenderPass {
public:
	RenderPass(std::shared_ptr<Device> device);
	~RenderPass();

	VkRenderPass getHandle() { return m_handle; }

private:
	void createRenderPass();

private:
	VkRenderPass m_handle;
	std::shared_ptr<Device> m_device;
};