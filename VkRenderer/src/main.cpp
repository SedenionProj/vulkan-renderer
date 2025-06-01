#include "src/vulkan/window.hpp"
#include "src/vulkan/context.hpp"
#include "src/vulkan/device.hpp"
#include "src/vulkan/swapchain.hpp"
#include "src/vulkan/renderPass.hpp"
#include "src/vulkan/pipeline.hpp"
#include "src/vulkan/framebuffer.hpp"
#include "src/vulkan/commandBuffer.hpp"



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
		for (int i = 0; i < m_swapchain->getSwapchainTexturesCount(); i++) {

			std::vector<std::shared_ptr<Texture2D>> textures = {
				m_swapchain->m_swapchainTextures[i]
			};

			m_swapChainFramebuffers.emplace_back(std::make_shared<Framebuffer>(
				textures,
				m_renderPass
			));
		}
	}

	void recordCommandBuffer(std::shared_ptr<CommandBuffer> commandBuffer, uint32_t imageIndex) {
		commandBuffer->beginRecording();
		commandBuffer->beginRenderpass(m_renderPass, m_swapChainFramebuffers[imageIndex], 1280, 720);
		commandBuffer->bindPipeline(m_pipeline);
		commandBuffer->updateViewport(1280, 720);

		vkCmdDraw(commandBuffer->getHandle(), 3, 1, 0, 0);

		commandBuffer->endRenderPass();
		commandBuffer->endRecording();
	}

	void drawFrame() {
		m_swapchain->getCurrentCommandBuffer()->getFence()->wait();

		m_swapchain->acquireNexImage();

		/*
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image");
		}
		*/



		m_swapchain->getCurrentCommandBuffer()->getFence()->reset();
		m_swapchain->getCurrentCommandBuffer()->reset();
		recordCommandBuffer(m_swapchain->getCurrentCommandBuffer(), m_swapchain->getCurrentImageIndex());

		m_swapchain->getCurrentCommandBuffer()->submit(m_device); // temp

		m_swapchain->present();

		/*
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_window->m_data.resized) {
			m_window->m_data.resized = false;
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}
		*/
		


	}

	void mainLoop() {
		while (!glfwWindowShouldClose(m_window->getHandle())) {
			glfwPollEvents();
			drawFrame();
		}

		vkDeviceWaitIdle(m_device->getHandle());

	}

	void cleanup() {

	}

private:
	

	void recreateSwapChain() {

		while (m_window->m_data.width == 0 || m_window->m_data.height == 0) {
			glfwWaitEvents();
		}
		vkDeviceWaitIdle(m_device->getHandle());
	}


private:		
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