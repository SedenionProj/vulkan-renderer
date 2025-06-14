#include "src/vulkan/device.hpp"
#include "src/vulkan/swapchain.hpp"
#include "src/vulkan/renderPass.hpp"
#include "src/vulkan/pipeline.hpp"
#include "src/vulkan/framebuffer.hpp"
#include "src/vulkan/commandBuffer.hpp"
#include "src/model.hpp"
#include "src/window.hpp"


const std::string MODEL_PATH = "assets/models/viking_room/viking_room.obj";
const std::string TEXTURE_PATH = "assets/models/viking_room/viking_room.png";

const std::string MODEL_PATH_S = "assets/models/sponza/sponza.obj";
const std::string TEXTURE_PATH_S = "assets/models/sponza/textures/spnza_bricks_a_bump.png";

class Renderer {

public:
	void run() {
		initVulkan();
		mainLoop();
		cleanup();
	}

private:

	void initVulkan() {
		m_window = std::make_shared<Window>();

		
		m_renderPass = std::make_shared<RenderPass>();
		m_shader = std::make_shared<Shader>();
		m_pipeline = std::make_shared<Pipeline>(m_shader, m_window->getSwapchain(), m_renderPass);
		m_depthTexture = std::make_shared<Texture2D>(1280, 720);
		m_depthTexture->createImage(Device::get()->getPhysicalDevice().findDepthFormat(), VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		m_depthTexture->createImageView(Device::get()->getPhysicalDevice().findDepthFormat(), VK_IMAGE_ASPECT_DEPTH_BIT);
		for (int i = 0; i < m_window->getSwapchain()->getSwapchainTexturesCount(); i++) {
			std::vector<std::shared_ptr<Texture2D>> textures = {
				m_window->getSwapchain()->m_swapchainTextures[i],
				m_depthTexture
			};

			m_swapChainFramebuffers.emplace_back(std::make_shared<Framebuffer>(
				textures,
				m_renderPass
			));
		}

		m_texture = std::make_shared<Texture2D>(TEXTURE_PATH);

		m_model = std::make_shared<Model>(MODEL_PATH_S.c_str());
		

		m_uniformBuffers.reserve(MAX_FRAMES_IN_FLIGHT);

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			m_uniformBuffers.emplace_back((uint32_t)sizeof(UniformBufferObject));
		}
		
		createDescriptorPool();
		createDescriptorSets();
	}
	
	
	void recordCommandBuffer(std::shared_ptr<CommandBuffer> commandBuffer, uint32_t imageIndex) {
		commandBuffer->beginRecording();
		commandBuffer->beginRenderpass(m_renderPass, m_swapChainFramebuffers[imageIndex], 1280, 720);
		commandBuffer->bindPipeline(m_pipeline);

		commandBuffer->updateViewport(1280, 720);

		for(auto mesh : m_model->m_meshes){
			VkBuffer vertexBuffers[] = {mesh->m_vertexBuffer->getHandle() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer->getHandle(), 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffer->getHandle(), mesh->m_indexBuffer->getHandle(), 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(commandBuffer->getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_shader->getPipelineLayout(), 0, 1, &descriptorSets[m_window->getSwapchain()->getCurrentFrameIndex()], 0, nullptr);

			vkCmdDrawIndexed(commandBuffer->getHandle(), static_cast<uint32_t>(mesh->m_count), 1, 0, 0, 0);
		}
		

		commandBuffer->endRenderPass();
		commandBuffer->endRecording();
	}
	
	void updateUniformBuffer(uint32_t currentImage) {
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		
		static float yaw = 0.0f; 
		static float pitch = 0.0f; 
		static float zoom = -5.0f;  

		double xpos, ypos;
		glfwGetCursorPos(m_window->getHandle(), &xpos, &ypos);

		int width, height;
		glfwGetWindowSize(m_window->getHandle(), &width, &height);

		float normX = ((float)xpos / width - 0.5f) * 2.0f;
		float normY = ((float)ypos / height - 0.5f) * 2.0f;

		yaw = -normX * glm::radians(180.0f); 
		pitch = normY * glm::radians(90.0f);  

		pitch = std::clamp(pitch, -glm::half_pi<float>() + 0.01f, glm::half_pi<float>() - 0.01f);

		if (glfwGetKey(m_window->getHandle(), GLFW_KEY_Z) == GLFW_PRESS) zoom -= 1.f;
		if (glfwGetKey(m_window->getHandle(), GLFW_KEY_S) == GLFW_PRESS) zoom += 1.f;

		glm::vec3 cameraPos;
		cameraPos.x = zoom * cos(pitch) * sin(yaw);
		cameraPos.y = zoom * sin(pitch);
		cameraPos.z = zoom * cos(pitch) * cos(yaw);

		UniformBufferObject ubo{};
		ubo.model = glm::mat4(1.0f);
		ubo.view = glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 5000.0f);
		ubo.proj[1][1] *= -1;
		m_uniformBuffers[currentImage].setData(&ubo, sizeof(ubo));

	}

	void drawFrame() {
		auto swapchain = m_window->getSwapchain();
		swapchain->getCurrentCommandBuffer()->getFence()->wait();

		swapchain->acquireNexImage();

		/*
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image");
		}
		*/


		swapchain->getCurrentCommandBuffer()->reset();
		recordCommandBuffer(swapchain->getCurrentCommandBuffer(), swapchain->getCurrentImageIndex());

		updateUniformBuffer(swapchain->getCurrentFrameIndex());

		swapchain->getCurrentCommandBuffer()->submit(); // temp

		swapchain->present();

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

		vkDeviceWaitIdle(Device::getHandle());

	}

	void cleanup() {

	}

	void createDescriptorPool() {
		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
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

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = m_texture->getImageView();
			imageInfo.sampler = m_texture->getSampler();

			std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = descriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(Device::getHandle(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

		}
	}

	bool hasStencilComponent(VkFormat format) {
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}
private:
	
	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};	

private:		
	std::shared_ptr<Window> m_window;
	std::shared_ptr<Model> m_model;
	std::shared_ptr<RenderPass> m_renderPass;
	std::shared_ptr<Pipeline> m_pipeline;
	std::shared_ptr<Shader> m_shader;
	std::vector<std::shared_ptr<Framebuffer>> m_swapChainFramebuffers;

	std::vector<UniformBuffer> m_uniformBuffers;
	std::shared_ptr<Texture2D> m_texture;
	std::shared_ptr<Texture2D> m_depthTexture;

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