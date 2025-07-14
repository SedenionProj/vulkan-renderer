#include "src/vulkan/commandBuffer.hpp"
#include "src/vulkan/syncObjects.hpp"
#include "src/vulkan/device.hpp"
#include "src/vulkan/framebuffer.hpp"
#include "src/vulkan/pipeline.hpp"
#include "src/vulkan/renderPass.hpp"

CommandPool::CommandPool() {
	auto& pDevice = Device::get()->getPhysicalDevice();
	QueueFamilyIndices queueFamilyIndices = pDevice.findQueueFamilies(pDevice.getHandle());

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	VK_CHECK(vkCreateCommandPool(Device::getHandle(), &poolInfo, nullptr, &m_handle));
}

CommandPool::~CommandPool() {
	vkDestroyCommandPool(Device::getHandle(), m_handle, nullptr);
}

CommandBuffer::CommandBuffer(VkCommandPool commandPool)
	: m_commandPool(commandPool) {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_commandPool;
	allocInfo.commandBufferCount = 1;

	VK_CHECK(vkAllocateCommandBuffers(Device::getHandle(), &allocInfo, &m_handle));

	m_imageAvailableSemaphores = std::make_unique<Semaphore>();
	m_renderFinishedSemaphores = std::make_unique<Semaphore>();
	m_fence = std::make_unique<Fence>();
}

CommandBuffer::~CommandBuffer() {
	vkFreeCommandBuffers(Device::getHandle(), m_commandPool, 1, &m_handle);
}

void CommandBuffer::beginRecording() {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK(vkBeginCommandBuffer(m_handle, &beginInfo));
}

void CommandBuffer::beginRenderpass(std::shared_ptr<RenderPass> renderPass, std::shared_ptr<Framebuffer> framebuffer, uint32_t width, uint32_t height)
{
	m_renderPass = renderPass;
	
	auto& clearValues = renderPass->getClearValues();

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass->getHandle();
	renderPassInfo.framebuffer = framebuffer->getHandle();
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = { width, height };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(m_handle, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

}

void CommandBuffer::beginRenderpass(VkRenderPass renderPass, std::shared_ptr<Framebuffer> framebuffer, uint32_t width, uint32_t height)
{
	VkClearValue clearValues = {0.1,0.1,0.1,1.};
	
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = framebuffer->getHandle();
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = { width, height };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearValues;
	
	vkCmdBeginRenderPass(m_handle, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void CommandBuffer::endRecording()
{
	VK_CHECK(vkEndCommandBuffer(m_handle));
}

void CommandBuffer::endRenderPass()
{
	vkCmdEndRenderPass(m_handle);
}

void CommandBuffer::reset()
{
	vkResetCommandBuffer(m_handle, 0);
}

void CommandBuffer::submit(bool semaphores)
{
	VkSemaphore waitSemaphore = m_imageAvailableSemaphores->getHandle();
	VkSemaphore signalSemaphore = m_renderFinishedSemaphores->getHandle();

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_handle;
	if (semaphores) {
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &waitSemaphore;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &signalSemaphore;
	}
	
	

	m_fence->reset();

	VK_CHECK(vkQueueSubmit(Device::get()->getGraphicsQueue(), 1, &submitInfo, m_fence->getHandle()));
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
