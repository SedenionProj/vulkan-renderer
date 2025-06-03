#include "src/vulkan/window.hpp"
#include "src/vulkan/context.hpp"
#include "src/vulkan/device.hpp"
#include "src/vulkan/swapchain.hpp"
#include "src/vulkan/renderPass.hpp"
#include "src/vulkan/pipeline.hpp"
#include "src/vulkan/framebuffer.hpp"
#include "src/vulkan/commandBuffer.hpp"
#include "src/vulkan/buffer.hpp"


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
		m_vertexBuffer = std::make_shared<VertexBuffer>(m_device, sizeof(vertices[0]) * vertices.size(), vertices.data());
		m_indexBuffer = std::make_shared<IndexBuffer>(m_device, sizeof(indices[0]) * indices.size(), indices.data());

		m_uniformBuffers.reserve(MAX_FRAMES_IN_FLIGHT);

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			m_uniformBuffers.emplace_back(m_device, (uint32_t)sizeof(UniformBufferObject));
		}
		
		createDescriptorPool();
		createDescriptorSets();
	}

	void recordCommandBuffer(std::shared_ptr<CommandBuffer> commandBuffer, uint32_t imageIndex) {
		commandBuffer->beginRecording();
		commandBuffer->beginRenderpass(m_renderPass, m_swapChainFramebuffers[imageIndex], 1280, 720);
		commandBuffer->bindPipeline(m_pipeline);

		commandBuffer->updateViewport(1280, 720);

		VkBuffer vertexBuffers[] = { m_vertexBuffer->getHandle() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer->getHandle(), 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer->getHandle(), m_indexBuffer->getHandle(), 0, VK_INDEX_TYPE_UINT16);

		vkCmdBindDescriptorSets(commandBuffer->getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_shader->getPipelineLayout(), 0, 1, &descriptorSets[m_swapchain->getCurrentFrameIndex()], 0, nullptr);

		vkCmdDrawIndexed(commandBuffer->getHandle(), static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

		commandBuffer->endRenderPass();
		commandBuffer->endRecording();
	}

	void updateUniformBuffer(uint32_t currentImage) {
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo{};
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), 1280 / (float)720, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;
		m_uniformBuffers[currentImage].setData(&ubo, sizeof(ubo));

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


		m_swapchain->getCurrentCommandBuffer()->reset();
		recordCommandBuffer(m_swapchain->getCurrentCommandBuffer(), m_swapchain->getCurrentImageIndex());

		updateUniformBuffer(m_swapchain->getCurrentFrameIndex());

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

	void createDescriptorPool() {
		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		if (vkCreateDescriptorPool(Device::getHandle(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	void createDescriptorSets() {
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_shader->getDescriptorSetLayout());
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		if (vkAllocateDescriptorSets(Device::getHandle(), &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = m_uniformBuffers[i].getHandle();
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = descriptorSets[i];
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;
			descriptorWrite.pImageInfo = nullptr; // Optional
			descriptorWrite.pTexelBufferView = nullptr; // Optional
			vkUpdateDescriptorSets(Device::getHandle(), 1, &descriptorWrite, 0, nullptr);

		}
	}

private:
	
	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};


	const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
	};

	const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
	};

private:		
	std::shared_ptr<Window> m_window;
	std::shared_ptr<Context> m_context;
	std::shared_ptr<Device> m_device;
	std::shared_ptr<Swapchain> m_swapchain;
	std::shared_ptr<RenderPass> m_renderPass;
	std::shared_ptr<Pipeline> m_pipeline;
	std::shared_ptr<Shader> m_shader;
	std::vector<std::shared_ptr<Framebuffer>> m_swapChainFramebuffers;
	std::shared_ptr<VertexBuffer> m_vertexBuffer;
	std::shared_ptr<IndexBuffer> m_indexBuffer;
	std::vector<UniformBuffer> m_uniformBuffers;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
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