#include "src/vulkan/vkHeader.hpp"
#include "src/vulkan/device.hpp"
#include "src/vulkan/swapchain.hpp"
#include "src/vulkan/renderPass.hpp"
#include "src/vulkan/pipeline.hpp"
#include "src/vulkan/framebuffer.hpp"
#include "src/vulkan/commandBuffer.hpp"
#include "src/vulkan/descriptorSet.hpp"
#include "src/vulkan/texture.hpp"
#include "src/vulkan/syncObjects.hpp"
#include "src/vulkan/shader.hpp"
#include "src/model.hpp"
#include "src/window.hpp"
#include "src/material.hpp"
#include "GLFW/glfw3.h"
// todo : framebuffer resize
class Renderer {

public:
	void run() {
		initVulkan();
		mainLoop();
	}

private:

	void initVulkan() {
		m_window = std::make_shared<Window>();

		PipelineDesc pipelineDesc{};

		// UBO
		m_transformUBO.reserve(MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			m_transformUBO.emplace_back((uint32_t)sizeof(UniformBufferObject));
		}

		// shadow
		float orthoSize = 30.0f;
		float nearPlane = 1.0f;
		float farPlane = 35.0f;

		m_shadowData.lightProjection = glm::ortho(
			-orthoSize, orthoSize,
			-orthoSize, orthoSize,
			nearPlane, farPlane
		);

		glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, -1.0f, 0.0f));
		glm::vec3 lightPos = -lightDir * 25.0f;
		m_shadowData.lightPos = lightPos;
		
		m_shadowData.lightView = glm::lookAt(
			lightPos,
			glm::vec3(0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f)
		);
		m_shadowData.lightSpace = m_shadowData.lightProjection * m_shadowData.lightView;

		m_shadowData.texture = std::make_shared<DepthTexture>(m_shadowData.resolution, m_shadowData.resolution);
		m_shadowData.texture->createSampler();

		pipelineDesc.shader = std::make_shared<Shader>("shadowMapVert.spv", "shadowMapFrag.spv");
		pipelineDesc.sampleCount = VK_SAMPLE_COUNT_1_BIT;
		pipelineDesc.clear = true;
		pipelineDesc.attachmentInfos = { { m_shadowData.texture } };
		pipelineDesc.swapchain = nullptr;

		m_shadowData.pipeline = std::make_shared<Pipeline>(pipelineDesc);

		m_shadowData.descriptorSet = std::make_shared<DescriptorSet>(pipelineDesc.shader, 0);
		m_shadowData.descriptorSet->setUniform(m_transformUBO, 0);
		// scene
		m_sceneData.depthTexture = std::make_shared<DepthTexture>(1280, 720, VK_SAMPLE_COUNT_8_BIT);

		m_sceneData.colorTexture = std::make_shared<Texture2D>(TextureType::COLOR, 1280, 720, m_window->getSwapchain()->m_swapchainImageFormat, VK_SAMPLE_COUNT_8_BIT);
		m_sceneData.colorTexture->createImage(VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		m_sceneData.colorTexture->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

		m_sceneData.resolveTexture = std::make_shared<Texture2D>(TextureType::COLOR, 1280, 720, m_window->getSwapchain()->m_swapchainImageFormat, VK_SAMPLE_COUNT_1_BIT);
		m_sceneData.resolveTexture->createImage(VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		m_sceneData.resolveTexture->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
		m_sceneData.resolveTexture->createSampler();

		m_sceneData.model = std::make_shared<Model>("models/sponza/sponza.obj");
		pipelineDesc.shader = m_sceneData.model->m_meshes[1]->m_material->m_shader; // todo : asset manager
		pipelineDesc.sampleCount = VK_SAMPLE_COUNT_8_BIT;
		pipelineDesc.clear = true;
		pipelineDesc.attachmentInfos = {
			{ m_sceneData.colorTexture},
			{ m_sceneData.depthTexture},
			{ m_sceneData.resolveTexture, true} };


		m_sceneData.pipeline = std::make_shared<Pipeline>(pipelineDesc);

		for (auto mesh : m_sceneData.model->m_meshes) {
			if (mesh->m_material != nullptr) {
				mesh->m_material->m_descriptorSet->setUniform(m_transformUBO, 0);
				mesh->m_material->m_descriptorSet->setTexture(m_shadowData.texture, 5);
			}
		}

		// cube map
		const char* faces[6] = {
			"skybox/right.jpg",
			"skybox/left.jpg",
			"skybox/top.jpg",
			"skybox/bottom.jpg",
			"skybox/front.jpg",
			"skybox/back.jpg"
		};
		m_skyBoxData.cubeMap = std::make_shared<CubeMap>(faces);
		pipelineDesc.shader = std::make_shared<Shader>("cubemapVert.spv", "cubemapFrag.spv");
		pipelineDesc.sampleCount = VK_SAMPLE_COUNT_8_BIT;
		pipelineDesc.clear = false;
		pipelineDesc.attachmentInfos = {
			{ m_sceneData.colorTexture},
			{ m_sceneData.depthTexture},
			{ m_sceneData.resolveTexture, true} };

		m_skyBoxData.pipeline = std::make_shared<Pipeline>(pipelineDesc);
		m_skyBoxData.descriptorSet = std::make_shared<DescriptorSet>(pipelineDesc.shader, 0);
		m_skyBoxData.descriptorSet->setUniform(m_transformUBO, 0);
		m_skyBoxData.descriptorSet->setTexture(m_skyBoxData.cubeMap, 1);

		// post process
		pipelineDesc.shader = std::make_shared<Shader>("postProcessVert.spv", "postProcessFrag.spv");
		pipelineDesc.sampleCount = VK_SAMPLE_COUNT_1_BIT;
		pipelineDesc.clear = true;
		pipelineDesc.attachmentInfos = { { m_window->getSwapchain()->m_swapchainTextures[0] } };
		pipelineDesc.swapchain = m_window->getSwapchain();

		m_postProcessData.descriptorSet = std::make_shared<DescriptorSet>(pipelineDesc.shader, 0);
		m_postProcessData.descriptorSet->setTexture(m_sceneData.resolveTexture, 0);
		m_postProcessData.pipeline = std::make_shared<Pipeline>(pipelineDesc);
	}
	
	void updateUniformBuffer(uint32_t currentImage) {
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		// Static camera state
		static glm::vec3 cameraPos = glm::vec3(0.0f, 10.0f, 0.0f);
		static float yaw = -90.0f; 
		static float pitch = 0.0f;
		static float speed = 1.0f;
		static float sensitivity = 0.5f;

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
		float yoffset = lastY - ypos;

		lastX = xpos;
		lastY = ypos;

		xoffset *= sensitivity;
		yoffset *= sensitivity;

		yaw += xoffset;
		pitch += yoffset;

		pitch = std::clamp(pitch, -89.0f, 89.0f);

		glm::vec3 front;
		front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		front.y = sin(glm::radians(pitch));
		front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		front = glm::normalize(front);

		glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
		glm::vec3 up = glm::normalize(glm::cross(right, front));

		float deltaTime = 0.016f;
		if (glfwGetKey(m_window->getHandle(), GLFW_KEY_W) == GLFW_PRESS) cameraPos += front * speed * deltaTime;
		if (glfwGetKey(m_window->getHandle(), GLFW_KEY_S) == GLFW_PRESS) cameraPos -= front * speed * deltaTime;
		if (glfwGetKey(m_window->getHandle(), GLFW_KEY_A) == GLFW_PRESS) cameraPos -= right * speed * deltaTime;
		if (glfwGetKey(m_window->getHandle(), GLFW_KEY_D) == GLFW_PRESS) cameraPos += right * speed * deltaTime;
		if (glfwGetKey(m_window->getHandle(), GLFW_KEY_SPACE) == GLFW_PRESS) cameraPos += up * speed * deltaTime;
		if (glfwGetKey(m_window->getHandle(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) cameraPos -= up * speed * deltaTime;

		UniformBufferObject ubo{};
		ubo.model = glm::mat4(1.f);
		ubo.model = glm::scale(ubo.model, glm::vec3(0.01f));
		ubo.view = glm::lookAt(cameraPos, cameraPos + front, up);
		ubo.proj = glm::perspective(glm::radians(90.0f), 1280.0f / 720.0f, 0.1f, 100.0f);
		ubo.proj[1][1] *= -1;
		ubo.camPos = glm::vec4(cameraPos,1);
		ubo.lightSpace = m_shadowData.lightSpace;

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

		// shadow pass
		commandBuffer->beginRenderpass(m_shadowData.pipeline->getRenderPass(), m_shadowData.pipeline->getFramebuffers()[swapchain->getCurrentImageIndex()], m_shadowData.resolution, m_shadowData.resolution);
		commandBuffer->bindPipeline(m_shadowData.pipeline);
		commandBuffer->updateViewport(m_shadowData.resolution, m_shadowData.resolution);

		for (auto mesh : m_sceneData.model->m_meshes) {
			if (mesh->m_material == nullptr)
				continue;

			VkBuffer vertexBuffers[] = { mesh->m_vertexBuffer->getHandle() };
			VkDeviceSize offsets[] = { 0 };
			VkDescriptorSet descriptorSets[] = { m_shadowData.descriptorSet->getHandle(m_window->getSwapchain()->getCurrentFrameIndex()) };
			vkCmdBindDescriptorSets(commandBuffer->getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_shadowData.descriptorSet->getShader()->getPipelineLayout(), 0, 1, descriptorSets, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer->getHandle(), 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffer->getHandle(), mesh->m_indexBuffer->getHandle(), 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(commandBuffer->getHandle(), static_cast<uint32_t>(mesh->m_count), 1, 0, 0, 0);
		}

		commandBuffer->endRenderPass();

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
		
		// sky box
		commandBuffer->beginRenderpass(m_skyBoxData.pipeline->getRenderPass(), m_skyBoxData.pipeline->getFramebuffers()[swapchain->getCurrentImageIndex()], 1280, 720);
		commandBuffer->bindPipeline(m_skyBoxData.pipeline);
		commandBuffer->updateViewport(1280, 720);
		VkDescriptorSet skyBoxDescriptorSet = m_skyBoxData.descriptorSet->getHandle(swapchain->getCurrentFrameIndex());
		vkCmdBindDescriptorSets(commandBuffer->getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_skyBoxData.descriptorSet->getShader()->getPipelineLayout(), 0, 1, &skyBoxDescriptorSet, 0, nullptr); // todo: abstract these

		vkCmdDraw(commandBuffer->getHandle(), 36, 1, 0, 0);

		commandBuffer->endRenderPass();

		// post processing pass
		commandBuffer->beginRenderpass(m_postProcessData.pipeline->getRenderPass(), m_postProcessData.pipeline->getFramebuffers()[swapchain->getCurrentImageIndex()], 1280, 720);
		commandBuffer->bindPipeline(m_postProcessData.pipeline);
		
		VkDescriptorSet postProcessDescriptorSet = m_postProcessData.descriptorSet->getHandle(swapchain->getCurrentFrameIndex());
		vkCmdBindDescriptorSets(commandBuffer->getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_postProcessData.descriptorSet->getShader()->getPipelineLayout(), 0, 1, &postProcessDescriptorSet, 0, nullptr); // todo: abstract these

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
		glm::mat4 lightSpace;
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

	struct SkyBoxData {
		std::shared_ptr<CubeMap> cubeMap;
		std::shared_ptr<Pipeline> pipeline;
		std::shared_ptr<DescriptorSet> descriptorSet;
	};
	struct ShadowData {
		std::shared_ptr<Texture2D> texture;
		std::shared_ptr<Pipeline> pipeline;
		std::shared_ptr<DescriptorSet> descriptorSet;
		uint32_t resolution = 4096;
		glm::vec3 lightPos = glm::vec3(50, 50.0f, .0f);
		glm::mat4 lightSpace;
		glm::mat4 lightProjection;
		glm::mat4 lightView;
	};
	
	PostProcessData m_postProcessData;
	SceneData m_sceneData;
	SkyBoxData m_skyBoxData;
	ShadowData m_shadowData;
};

int main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	try {
		Renderer app;
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	_CrtDumpMemoryLeaks();
	return EXIT_SUCCESS;
}