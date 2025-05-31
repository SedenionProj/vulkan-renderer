#include "src/vulkan/window.hpp"
#include "src/vulkan/context.hpp"
#include "src/vulkan/device.hpp"
#include "src/vulkan/swapchain.hpp"
#include "src/vulkan/renderPass.hpp"
#include "src/vulkan/pipeline.hpp"
#include "src/vulkan/framebuffer.hpp"

const int MAX_FRAMES_IN_FLIGHT = 2;

class Renderer {

public:
	void run() {
		m_window = std::make_shared<Window>();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:

	void initVulkan() {
		m_context = std::make_shared<Context>();
		m_device = std::make_shared<Device>(m_context);
		m_swapchain = std::make_shared<Swapchain>(m_context, m_window, m_device);
		m_renderPass = std::make_shared<RenderPass>();
		m_shader = std::make_shared<Shader>();
		m_pipeline = std::make_shared<Pipeline>(m_shader, m_swapchain, m_renderPass);
		for (int i = 0; i<m_swapchain->getSwapchainTexturesCount(); i++) {

			std::vector<std::shared_ptr<Texture2D>> textures = {
				m_swapchain->m_swapchainTextures[i]
			};

			m_swapChainFramebuffers.emplace_back(std::make_shared<Framebuffer>(
				textures,
				m_renderPass
			));
		}
		createCommandPool();
		createCommandBuffers();
		createSyncObjects();

	}

	void createCommandPool() {
		QueueFamilyIndices queueFamilyIndices = m_device->getPhysicalDevice().findQueueFamilies(m_device->getPhysicalDevice().getHandle());

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
		if (vkCreateCommandPool(m_device->getHandle(), &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}
	}

	void createCommandBuffers() {
		m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

		if (vkAllocateCommandBuffers(m_device->getHandle(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers");
		}
	}

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer");
		}

		VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_renderPass->getHandle();
		renderPassInfo.framebuffer = m_swapChainFramebuffers[imageIndex]->getHandle();
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_swapchain->m_swapchainExtent;
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_swapchain->m_swapchainExtent.width);
		viewport.height = static_cast<float>(m_swapchain->m_swapchainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = m_swapchain->m_swapchainExtent;

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->getHandle());

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer");
		}
	}

	void createSyncObjects() {
		m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (vkCreateSemaphore(m_device->getHandle(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(m_device->getHandle(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(m_device->getHandle(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {

				throw std::runtime_error("failed to create synchronization objects for a frame");
			}
		}

	}

	void drawFrame() {
		vkWaitForFences(m_device->getHandle(), 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(m_device->getHandle(), m_swapchain->getSwapchain(), UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image");
		}

		vkResetFences(m_device->getHandle(), 1, &m_inFlightFences[m_currentFrame]);

		vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0);
		recordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];

		VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(m_device->m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { m_swapchain->getSwapchain()};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = &imageIndex;

		result = vkQueuePresentKHR(m_device->m_presentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_window->m_data.resized) {
			m_window->m_data.resized = false;
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}

		m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	}

	void mainLoop() {
		while (!glfwWindowShouldClose(m_window->getHandle())) {
			glfwPollEvents();
			drawFrame();
		}

		vkDeviceWaitIdle(m_device->getHandle());

	}


	void cleanup() {
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(m_device->getHandle(), m_renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(m_device->getHandle(), m_imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(m_device->getHandle(), m_inFlightFences[i], nullptr);
		}

		vkDestroyCommandPool(m_device->getHandle(), m_commandPool, nullptr);
	}

private:
	

	void recreateSwapChain() {

		while (m_window->m_data.width == 0 || m_window->m_data.height == 0) {
			glfwWaitEvents();
		}
		vkDeviceWaitIdle(m_device->getHandle());
	}


private:	
	VkCommandPool m_commandPool;
	uint32_t m_currentFrame = 0;

	std::vector<VkCommandBuffer> m_commandBuffers;
	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;

	std::shared_ptr<Window> m_window;
	std::shared_ptr<Context> m_context;
	std::shared_ptr<Device> m_device;
	std::shared_ptr<Swapchain> m_swapchain;
	std::shared_ptr<RenderPass> m_renderPass;
	std::shared_ptr<Pipeline> m_pipeline;
	std::shared_ptr<Shader> m_shader;
	std::vector<std::shared_ptr<Framebuffer>> m_swapChainFramebuffers;
};

int main() {
	Renderer app;

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}