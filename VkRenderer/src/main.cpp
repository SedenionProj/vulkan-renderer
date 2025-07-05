#include "src/vulkan/device.hpp"
#include "src/vulkan/swapchain.hpp"
#include "src/vulkan/renderPass.hpp"
#include "src/vulkan/pipeline.hpp"
#include "src/vulkan/framebuffer.hpp"
#include "src/vulkan/commandBuffer.hpp"
#include "src/vulkan/descriptorSet.hpp"
#include "src/model.hpp"
#include "src/window.hpp"
#include "src/material.hpp"

class Renderer {

public:
	void run() {
		initVulkan();
		mainLoop();
	}

private:

	void initVulkan() {
		m_window = std::make_shared<Window>();
		// scene
		m_sceneData.depthTexture = std::make_shared<Texture2D>(1280, 720, Device::get()->getPhysicalDevice().findDepthFormat(), VK_SAMPLE_COUNT_8_BIT);
		m_sceneData.depthTexture->createImage(VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		m_sceneData.depthTexture->createImageView(VK_IMAGE_ASPECT_DEPTH_BIT);

		m_sceneData.colorTexture = std::make_shared<Texture2D>(1280, 720, m_window->getSwapchain()->m_swapchainImageFormat, VK_SAMPLE_COUNT_8_BIT);
		m_sceneData.colorTexture->createImage(VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		m_sceneData.colorTexture->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

		m_sceneData.resolveTexture = std::make_shared<Texture2D>(1280, 720, m_window->getSwapchain()->m_swapchainImageFormat, VK_SAMPLE_COUNT_1_BIT);
		m_sceneData.resolveTexture->createImage(VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		m_sceneData.resolveTexture->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
		m_sceneData.resolveTexture->createSampler();

		m_sceneData.model = std::make_shared<Model>("assets/models/sponza/sponza.obj");

		PipelineDesc pipelineDesc{};
		pipelineDesc.shader = m_sceneData.model->m_meshes[1]->m_material->m_shader;
		pipelineDesc.sampleCount = VK_SAMPLE_COUNT_8_BIT;
		pipelineDesc.attachmentInfos = {
			{ m_sceneData.colorTexture, Attachment::Type::COLOR, 0 },
			{ m_sceneData.depthTexture, Attachment::Type::DEPTH, 1 },
			{ m_sceneData.resolveTexture, Attachment::Type::COLOR, 2, true} };


		m_sceneData.pipeline = std::make_shared<Pipeline>(pipelineDesc);

		// post process
		pipelineDesc.shader = std::make_shared<Shader>("postProcessVert.spv", "postProcessFrag.spv");
		pipelineDesc.sampleCount = VK_SAMPLE_COUNT_1_BIT;
		pipelineDesc.attachmentInfos = { { m_window->getSwapchain()->m_swapchainTextures[0], Attachment::Type::PRESENT, 0 } };
		pipelineDesc.swapchain = m_window->getSwapchain();

		m_postProcessData.descriptorSet = std::make_shared<DescriptorSet>(pipelineDesc.shader, 0);
		m_postProcessData.descriptorSet->setTexture(m_sceneData.resolveTexture, 0);
		m_postProcessData.pipeline = std::make_shared<Pipeline>(pipelineDesc);


		// UBO
		m_transformUBO.reserve(MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			m_transformUBO.emplace_back((uint32_t)sizeof(UniformBufferObject));
		}
		

		for (auto mesh : m_sceneData.model->m_meshes) {
			if (mesh->m_material != nullptr) {
				mesh->m_material->m_descriptorSet->setUniform(m_transformUBO, 0);
			}
		}
	}
	
	void updateUniformBuffer(uint32_t currentImage) {
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		// Static camera state
		static glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 5.0f);
		static float yaw = -90.0f;  // Facing forward
		static float pitch = 0.0f;
		static float speed = 100.0f;
		static float sensitivity = 0.5f;

		// Handle mouse movement
		static double lastX = 640.0, lastY = 360.0;
		static bool firstMouse = true;
		double xpos, ypos;
		glfwGetCursorPos(m_window->getHandle(), &xpos, &ypos);

		if (firstMouse) {
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}

		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos; // reversed since y-coordinates go from top to bottom

		lastX = xpos;
		lastY = ypos;

		xoffset *= sensitivity;
		yoffset *= sensitivity;

		yaw += xoffset;
		pitch += yoffset;

		// Clamp pitch to avoid flipping
		pitch = std::clamp(pitch, -89.0f, 89.0f);

		// Calculate front vector
		glm::vec3 front;
		front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		front.y = sin(glm::radians(pitch));
		front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		front = glm::normalize(front);

		// Calculate right and up vectors
		glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
		glm::vec3 up = glm::normalize(glm::cross(right, front));

		// Handle keyboard input
		float deltaTime = 0.016f; // You should actually compute frame time
		if (glfwGetKey(m_window->getHandle(), GLFW_KEY_W) == GLFW_PRESS) cameraPos += front * speed * deltaTime;
		if (glfwGetKey(m_window->getHandle(), GLFW_KEY_S) == GLFW_PRESS) cameraPos -= front * speed * deltaTime;
		if (glfwGetKey(m_window->getHandle(), GLFW_KEY_A) == GLFW_PRESS) cameraPos -= right * speed * deltaTime;
		if (glfwGetKey(m_window->getHandle(), GLFW_KEY_D) == GLFW_PRESS) cameraPos += right * speed * deltaTime;
		if (glfwGetKey(m_window->getHandle(), GLFW_KEY_SPACE) == GLFW_PRESS) cameraPos += up * speed * deltaTime;
		if (glfwGetKey(m_window->getHandle(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) cameraPos -= up * speed * deltaTime;

		// Send data to shader
		UniformBufferObject ubo{};
		ubo.model = glm::mat4(1.0f);
		ubo.view = glm::lookAt(cameraPos, cameraPos + front, up);
		ubo.proj = glm::perspective(glm::radians(90.0f), 1280.0f / 720.0f, 0.1f, 5000.0f);
		ubo.proj[1][1] *= -1;
		ubo.camPos = glm::vec4(cameraPos,1);

		m_transformUBO[currentImage].setData(&ubo, sizeof(ubo));

	}

	void drawFrame() {
		
		auto swapchain = m_window->getSwapchain();
		auto commandBuffer = swapchain->getCurrentCommandBuffer();
		updateUniformBuffer(swapchain->getCurrentFrameIndex());

		// begin
		commandBuffer->getFence()->wait();
		swapchain->acquireNexImage();
		commandBuffer->reset();
		commandBuffer->beginRecording();

		// scene pass
		commandBuffer->beginRenderpass(m_sceneData.pipeline->getRenderPass(), m_sceneData.pipeline->getFramebuffers()[swapchain->getCurrentImageIndex()], 1280, 720);
		commandBuffer->bindPipeline(m_sceneData.pipeline);

		commandBuffer->updateViewport(1280, 720);

		for (auto mesh : m_sceneData.model->m_meshes) {
			if (mesh->m_material == nullptr)
				continue;

			VkBuffer vertexBuffers[] = { mesh->m_vertexBuffer->getHandle() };
			VkDeviceSize offsets[] = { 0 };
			VkDescriptorSet descriptorSets[] = { mesh->m_material->m_descriptorSet->getHandle(m_window->getSwapchain()->getCurrentFrameIndex()) };
			vkCmdBindDescriptorSets(commandBuffer->getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, mesh->m_material->m_shader->getPipelineLayout(), 0, 1, descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer->getHandle(), 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffer->getHandle(), mesh->m_indexBuffer->getHandle(), 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(commandBuffer->getHandle(), static_cast<uint32_t>(mesh->m_count), 1, 0, 0, 0);
		}
		commandBuffer->endRenderPass();
		
		// post processing pass
		commandBuffer->beginRenderpass(m_postProcessData.pipeline->getRenderPass(), m_postProcessData.pipeline->getFramebuffers()[swapchain->getCurrentImageIndex()], 1280, 720);
		commandBuffer->bindPipeline(m_postProcessData.pipeline);
		
		VkDescriptorSet postProcessDescriptorSet = m_postProcessData.descriptorSet->getHandle(swapchain->getCurrentFrameIndex());
		vkCmdBindDescriptorSets(commandBuffer->getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_postProcessData.descriptorSet->getShader()->getPipelineLayout(), 0, 1, &postProcessDescriptorSet, 0, nullptr);

		vkCmdDraw(commandBuffer->getHandle(), 3, 1, 0, 0);

		commandBuffer->endRenderPass();

		// end
		commandBuffer->endRecording();
		commandBuffer->submit();
		swapchain->present();
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(m_window->getHandle())) {
			glfwPollEvents();

			
			drawFrame();
		}

		vkDeviceWaitIdle(Device::getHandle());

	}

private:
	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec4 camPos;
	};

private:
	std::shared_ptr<Window> m_window;

	std::vector<UniformBuffer> m_transformUBO;

	struct SceneData {
		std::shared_ptr<Model> model;
		std::shared_ptr<Pipeline> pipeline;
		std::shared_ptr<Texture2D> colorTexture;
		std::shared_ptr<Texture2D> resolveTexture;
		std::shared_ptr<Texture2D> depthTexture;
		
	};

	struct PostProcessData {
		std::shared_ptr<Pipeline> pipeline;
		std::shared_ptr<DescriptorSet> descriptorSet;
	};
	
	PostProcessData m_postProcessData;
	SceneData m_sceneData;

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