#pragma once
#include "src/vulkan/vkHeader.hpp"

class CommandPool {
public:
	CommandPool();
	~CommandPool();

	VkCommandPool getHandle() { return m_handle; }

private:
	VkCommandPool m_handle;
};

class CommandBuffer {
public:
	CommandBuffer(VkCommandPool m_commandPool);
	~CommandBuffer();

	void beginRecording();
	void beginRenderpass(std::shared_ptr<RenderPass> renderPass, std::shared_ptr<Framebuffer> framebuffer, uint32_t width, uint32_t height);
	void endRecording();
	void endRenderPass();

	void bindPipeline(std::shared_ptr<Pipeline> pipeline);
	void updateViewport(uint32_t width, uint32_t height);

	void reset();
	void submit(bool semaphores = true);

	std::shared_ptr<Fence> getFence() { return m_fence; }

	VkCommandBuffer getHandle() { return m_handle; }

public:
	VkCommandBuffer m_handle;
	VkCommandPool m_commandPool;

	std::shared_ptr<RenderPass> m_renderPass;
	std::shared_ptr<Semaphore> m_imageAvailableSemaphores;
	std::shared_ptr<Semaphore> m_renderFinishedSemaphores;
	std::shared_ptr<Fence> m_fence;
};