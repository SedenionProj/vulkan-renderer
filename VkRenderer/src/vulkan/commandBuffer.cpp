#include "src/vulkan/commandBuffer.hpp"
#include "src/vulkan/syncObjects.hpp"

CommandPool::CommandPool(std::shared_ptr<Device> device) {
	QueueFamilyIndices queueFamilyIndices = device->getPhysicalDevice().findQueueFamilies(device->getPhysicalDevice().getHandle());

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	if (vkCreateCommandPool(Device::getHandle(), &poolInfo, nullptr, &m_handle) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
}

CommandPool::~CommandPool() {
	vkDestroyCommandPool(Device::getHandle(), m_handle, nullptr);
}

CommandBuffer::CommandBuffer(VkCommandPool commandPool)
	: m_commandPool(commandPool) {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(Device::getHandle(), &allocInfo, &m_handle) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers");
	}

	m_imageAvailableSemaphores = std::make_unique<Semaphore>();
	m_renderFinishedSemaphores = std::make_unique<Semaphore>();
	m_fence = std::make_unique<Fence>();
}

CommandBuffer::~CommandBuffer() {

}

void CommandBuffer::beginRecording() {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(m_handle, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer");
	}
}

void CommandBuffer::beginRenderpass(std::shared_ptr<RenderPass> renderPass, std::shared_ptr<Framebuffer> framebuffer, uint32_t width, uint32_t height)
{
	m_renderPass = renderPass;
	VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass->getHandle();
	renderPassInfo.framebuffer = framebuffer->getHandle();
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = { width, height };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(m_handle, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

}

void CommandBuffer::endRecording()
{
	if (vkEndCommandBuffer(m_handle) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer");
	}
}

void CommandBuffer::endRenderPass()
{
	vkCmdEndRenderPass(m_handle);
}

void CommandBuffer::reset()
{
	vkResetCommandBuffer(m_handle, 0);
}

void CommandBuffer::submit(std::shared_ptr<Device> device)
{
	VkSemaphore waitSemaphore = m_imageAvailableSemaphores->getHandle();
	VkSemaphore signalSemaphore = m_renderFinishedSemaphores->getHandle();

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &waitSemaphore;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &signalSemaphore;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_handle;

	if (vkQueueSubmit(device->m_graphicsQueue, 1, &submitInfo, m_fence->getHandle()) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}
}

void CommandBuffer::bindPipeline(std::shared_ptr<Pipeline> pipeline)
{
	vkCmdBindPipeline(m_handle, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getHandle());

}

void CommandBuffer::updateViewport(uint32_t width, uint32_t height)
{
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(width);
	viewport.height = static_cast<float>(height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = { width, height };


	vkCmdSetViewport(m_handle, 0, 1, &viewport);
	vkCmdSetScissor(m_handle, 0, 1, &scissor);
}
