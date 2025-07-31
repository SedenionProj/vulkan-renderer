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
#include "src/vulkan/imguiContext.hpp"
#include "src/model.hpp"
#include "src/window.hpp"
#include "src/material.hpp"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

// todo : framebuffer resize

class Renderer {

public:
	void run() {
		m_window = std::make_shared<Window>();
		init();
		//m_gui = std::make_shared<Gui>(m_window);
		mainLoop();
	}

private:
	void init() {
		PipelineDesc pipelineDesc{};

		// init scene data
		m_sceneUBO.reserve(MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			m_sceneUBO.emplace_back((uint32_t)sizeof(SceneDataUBO));
		}
		// depth pre-pass
		m_depthPrePass.texture = std::make_shared<DepthTexture>(1280, 720, VK_SAMPLE_COUNT_1_BIT);
		m_depthPrePass.texture->createSampler();

		pipelineDesc.shader = std::make_shared<Shader>("spv/depthPrePassVert.spv", "spv/depthPrePassFrag.spv");
		pipelineDesc.sampleCount = VK_SAMPLE_COUNT_1_BIT;
		pipelineDesc.clear = true;
		pipelineDesc.attachmentInfos = { { m_depthPrePass.texture } };
		pipelineDesc.swapchain = nullptr;

		m_depthPrePass.pipeline = std::make_shared<Pipeline>(pipelineDesc);

		m_depthPrePass.descriptorSet = std::make_shared<DescriptorSet>(pipelineDesc.shader, 0);
		m_depthPrePass.descriptorSet->setUniform(m_sceneUBO, 0);

		// ssao
		m_ssaoPass.texture = std::make_shared<Texture2D>(TextureType::COLOR, 1280, 720, VK_FORMAT_B10G11R11_UFLOAT_PACK32, VK_SAMPLE_COUNT_1_BIT);
		m_ssaoPass.texture->createImage(VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		m_ssaoPass.texture->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
		m_ssaoPass.texture->createSampler();

		pipelineDesc.shader = std::make_shared<Shader>("spv/screenVert.spv", "spv/ssaoFrag.spv");
		pipelineDesc.sampleCount = VK_SAMPLE_COUNT_1_BIT;
		pipelineDesc.clear = true;
		pipelineDesc.attachmentInfos = { { m_ssaoPass.texture } };
		pipelineDesc.swapchain = nullptr;
		m_ssaoPass.pipeline = std::make_shared<Pipeline>(pipelineDesc);

		m_ssaoPass.descriptorSet = std::make_shared<DescriptorSet>(pipelineDesc.shader, 0);
		m_ssaoPass.descriptorSet->setUniform(m_sceneUBO, 0);
		m_ssaoPass.descriptorSet->setTexture(m_depthPrePass.texture, 1);

		// shadow
		float orthoSize = 30.0f;
		float nearPlane = 1.0f;
		float farPlane = 35.0f;

		m_shadowData.lightProjection = glm::ortho(
			-orthoSize, orthoSize,
			-orthoSize, orthoSize,
			nearPlane, farPlane
		);

		glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, -1.0f, 0.2f));
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

		pipelineDesc.shader = std::make_shared<Shader>("spv/shadowMapVert.spv", "spv/shadowMapFrag.spv");
		pipelineDesc.sampleCount = VK_SAMPLE_COUNT_1_BIT;
		pipelineDesc.clear = true;
		pipelineDesc.attachmentInfos = { { m_shadowData.texture } };
		pipelineDesc.swapchain = nullptr;

		m_shadowData.pipeline = std::make_shared<Pipeline>(pipelineDesc);

		m_shadowData.descriptorSet = std::make_shared<DescriptorSet>(pipelineDesc.shader, 0);
		m_shadowData.descriptorSet->setUniform(m_sceneUBO, 0);
		// forward scene
		m_forwardData.shader = std::make_shared<Shader>("spv/basicVert.spv", "spv/pbrFrag.spv");

		m_forwardData.depthTexture = std::make_shared<DepthTexture>(1280, 720, VK_SAMPLE_COUNT_8_BIT);

		m_forwardData.colorTexture = std::make_shared<Texture2D>(TextureType::COLOR, 1280, 720, VK_FORMAT_B10G11R11_UFLOAT_PACK32, VK_SAMPLE_COUNT_8_BIT);
		m_forwardData.colorTexture->createImage(VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		m_forwardData.colorTexture->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);

		m_forwardData.resolveTexture = std::make_shared<Texture2D>(TextureType::COLOR, 1280, 720, VK_FORMAT_B10G11R11_UFLOAT_PACK32, VK_SAMPLE_COUNT_1_BIT);
		m_forwardData.resolveTexture->createImage(VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		m_forwardData.resolveTexture->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
		m_forwardData.resolveTexture->createSampler();

		m_drawables.resize(2);
		m_drawables[1] = std::make_shared<Model>("models/sponza/sponza.obj");
		m_drawables[1]->m_modelMatrix = glm::scale(glm::mat4(1.f), glm::vec3(0.01f));

		m_drawables[0] = std::make_shared<Model>();
		m_drawables[0]->createCube();
		m_drawables[0]->m_modelMatrix = glm::translate(glm::mat4(1.f), glm::vec3(0,10,0));

		pipelineDesc.shader = m_forwardData.shader; // todo : asset manager
		pipelineDesc.sampleCount = VK_SAMPLE_COUNT_8_BIT;
		pipelineDesc.clear = true;
		pipelineDesc.attachmentInfos = {
			{ m_forwardData.colorTexture},
			{ m_forwardData.depthTexture},
			{ m_forwardData.resolveTexture, true} };

		m_forwardData.pipeline = std::make_shared<Pipeline>(pipelineDesc);

		m_forwardData.descriptorSet = std::make_shared<DescriptorSet>(pipelineDesc.shader, 0);
		m_forwardData.descriptorSet->setUniform(m_sceneUBO, 0);
		m_forwardData.descriptorSet->setTexture(m_shadowData.texture, 1);
		m_forwardData.descriptorSet->setTexture(m_ssaoPass.texture, 2);

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
		pipelineDesc.shader = std::make_shared<Shader>("spv/cubeMapVert.spv", "spv/cubeMapFrag.spv");
		pipelineDesc.sampleCount = VK_SAMPLE_COUNT_8_BIT;
		pipelineDesc.clear = false;
		pipelineDesc.attachmentInfos = {
			{ m_forwardData.colorTexture},
			{ m_forwardData.depthTexture},
			{ m_forwardData.resolveTexture, true} };

		m_skyBoxData.pipeline = std::make_shared<Pipeline>(pipelineDesc);
		m_skyBoxData.descriptorSet = std::make_shared<DescriptorSet>(pipelineDesc.shader, 0);
		m_skyBoxData.descriptorSet->setUniform(m_sceneUBO, 0);
		m_skyBoxData.descriptorSet->setTexture(m_skyBoxData.cubeMap, 1);

		// bloom
		float mutl = 1;
		for (int i = 0; i < mipChainLength; i++) {
			m_bloomData.mipChain[i] = std::make_shared<Texture2D>(TextureType::COLOR, 1280 * mutl, 720 * mutl, VK_FORMAT_B10G11R11_UFLOAT_PACK32, VK_SAMPLE_COUNT_1_BIT);
			m_bloomData.mipChain[i]->createImage(VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			m_bloomData.mipChain[i]->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
			m_bloomData.mipChain[i]->createSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
			mutl *= 0.5f;
		}

		pipelineDesc.shader = std::make_shared<Shader>("spv/screenVert.spv", "spv/bloomFrag.spv");
		pipelineDesc.sampleCount = VK_SAMPLE_COUNT_1_BIT;
		pipelineDesc.clear = true; // ?
		pipelineDesc.attachmentInfos = { { m_bloomData.mipChain[0] } };
		pipelineDesc.swapchain = nullptr;
		pipelineDesc.createFramebuffers = false;
		pipelineDesc.blendMode = BlendMode::DEFAULT;
		m_bloomData.pipeline = std::make_shared<Pipeline>(pipelineDesc);

		for (int i = 0; i < mipChainLength; i++) {
			for (int j = 0; j < MAX_FRAMES_IN_FLIGHT; j++) {
				m_bloomData.framebuffers[j][i] = std::make_shared<Framebuffer>(
					std::vector<std::shared_ptr<Texture>>{ m_bloomData.mipChain[i] },
					m_bloomData.pipeline->getRenderPass()
				);
			}
		}

		for (int i = 0; i<mipChainLength+1; i++) {	// +1 counting resolve
			m_bloomData.descriptorSets[i] = std::make_shared<DescriptorSet>(pipelineDesc.shader, 0);
			if (i == 0) {
				m_bloomData.descriptorSets[i]->setTexture(m_forwardData.resolveTexture, 0);
			} else {
				m_bloomData.descriptorSets[i]->setTexture(m_bloomData.mipChain[i-1], 0);
			}

		}
		// tone mapping
		m_toneMappingData.texture = std::make_shared<Texture2D>(TextureType::COLOR, 1280, 720, VK_FORMAT_B8G8R8A8_UNORM, VK_SAMPLE_COUNT_1_BIT);
		m_toneMappingData.texture->createImage(VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		m_toneMappingData.texture->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
		m_toneMappingData.texture->createSampler();

		pipelineDesc.shader = std::make_shared<Shader>("spv/screenVert.spv", "spv/toneMappingFrag.spv");
		pipelineDesc.sampleCount = VK_SAMPLE_COUNT_1_BIT;
		pipelineDesc.clear = true;
		pipelineDesc.attachmentInfos = { { m_toneMappingData.texture } };
		pipelineDesc.swapchain = nullptr;
		pipelineDesc.createFramebuffers = true;
		pipelineDesc.blendMode = BlendMode::DEFAULT;
		m_toneMappingData.pipeline = std::make_shared<Pipeline>(pipelineDesc);
		
		m_toneMappingData.descriptorSet = std::make_shared<DescriptorSet>(pipelineDesc.shader, 0);
		m_toneMappingData.descriptorSet->setTexture(m_forwardData.resolveTexture, 0);
		m_toneMappingData.descriptorSet->setTexture(m_bloomData.mipChain[0], 1);

		// post process
		pipelineDesc.shader = std::make_shared<Shader>("spv/screenVert.spv", "spv/postProcessFrag.spv");
		pipelineDesc.sampleCount = VK_SAMPLE_COUNT_1_BIT;
		pipelineDesc.clear = true;
		pipelineDesc.attachmentInfos = { { m_window->getSwapchain()->m_swapchainTextures[0] } };
		pipelineDesc.swapchain = m_window->getSwapchain();
		pipelineDesc.createFramebuffers = true;

		m_postProcessData.pipeline = std::make_shared<Pipeline>(pipelineDesc);
		m_postProcessData.descriptorSet = std::make_shared<DescriptorSet>(pipelineDesc.shader, 0);
		m_postProcessData.descriptorSet->setTexture(m_toneMappingData.texture, 0);
	}

	void updateUniformBuffer(uint32_t currentImage) {
		static auto lastTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> delta = currentTime - lastTime;
		m_deltaTime = delta.count();

		lastTime = currentTime;

		// Static camera state
		static glm::vec3 cameraPos = glm::vec3(0.0f, 10.0f, 5.0f);
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

		float deltaTime = m_deltaTime*10.f;
		if (glfwGetKey(m_window->getHandle(), GLFW_KEY_W) == GLFW_PRESS) cameraPos += front * speed * deltaTime;
		if (glfwGetKey(m_window->getHandle(), GLFW_KEY_S) == GLFW_PRESS) cameraPos -= front * speed * deltaTime;
		if (glfwGetKey(m_window->getHandle(), GLFW_KEY_A) == GLFW_PRESS) cameraPos -= right * speed * deltaTime;
		if (glfwGetKey(m_window->getHandle(), GLFW_KEY_D) == GLFW_PRESS) cameraPos += right * speed * deltaTime;
		if (glfwGetKey(m_window->getHandle(), GLFW_KEY_SPACE) == GLFW_PRESS) cameraPos += up * speed * deltaTime;
		if (glfwGetKey(m_window->getHandle(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) cameraPos -= up * speed * deltaTime;

		m_sceneData.view = glm::lookAt(cameraPos, cameraPos + front, up);
		m_sceneData.proj = glm::perspective(glm::radians(90.0f), 1280.0f / 720.0f, 0.1f, 100.0f);
		m_sceneData.proj[1][1] *= -1;
		m_sceneData.camPos = glm::vec4(cameraPos, 1);
		m_sceneData.lightSpace = m_shadowData.lightSpace;
		m_sceneData.lights[0].position = glm::vec4(m_shadowData.lightPos, 1.0f);
		m_sceneData.lights[0].color = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f)*1000.f;
		m_sceneData.lights[1].position = glm::vec4(0,5,0, 1.0f);
		m_sceneData.lights[1].color = glm::vec4(1.f,0.5f,0.85f,0.f)*500.f;

		m_sceneUBO[currentImage].setData(&m_sceneData, sizeof(m_sceneData));
	}

	void depthPrePass() {
		commandBuffer->beginRenderpass(m_depthPrePass.pipeline->getRenderPass(), m_depthPrePass.pipeline->getFramebuffers()[swapchain->getCurrentImageIndex()], 1280, 720);
		commandBuffer->bindPipeline(m_depthPrePass.pipeline);
		commandBuffer->updateViewport(1280, 720);
		for (auto& model : m_drawables) {
			m_forwardData.shader->pushConstants(commandBuffer->getHandle(), &model->m_modelMatrix);
			for (auto mesh : model->m_meshes) {
				if (mesh->m_material == nullptr)
					continue;

				VkBuffer vertexBuffers[] = { mesh->m_vertexBuffer->getHandle() };
				VkDeviceSize offsets[] = { 0 };
				VkDescriptorSet descriptorSets[] = { m_depthPrePass.descriptorSet->getHandle(m_window->getSwapchain()->getCurrentFrameIndex()) };
				vkCmdBindDescriptorSets(commandBuffer->getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_depthPrePass.descriptorSet->getShader()->getPipelineLayout(), 0, 1, descriptorSets, 0, nullptr);
				vkCmdBindVertexBuffers(commandBuffer->getHandle(), 0, 1, vertexBuffers, offsets);
				vkCmdBindIndexBuffer(commandBuffer->getHandle(), mesh->m_indexBuffer->getHandle(), 0, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(commandBuffer->getHandle(), static_cast<uint32_t>(mesh->m_count), 1, 0, 0, 0);
			}
		}
		commandBuffer->endRenderPass();
	}

	void ssaoPass() {
		commandBuffer->beginRenderpass(m_ssaoPass.pipeline->getRenderPass(), m_ssaoPass.pipeline->getFramebuffers()[swapchain->getCurrentImageIndex()], 1280, 720);
		commandBuffer->bindPipeline(m_ssaoPass.pipeline);

		VkDescriptorSet ssaoDescriptorSet = m_ssaoPass.descriptorSet->getHandle(swapchain->getCurrentFrameIndex());
		vkCmdBindDescriptorSets(commandBuffer->getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_ssaoPass.descriptorSet->getShader()->getPipelineLayout(), 0, 1, &ssaoDescriptorSet, 0, nullptr); // todo: abstract these

		vkCmdDraw(commandBuffer->getHandle(), 3, 1, 0, 0);

		commandBuffer->endRenderPass();
	}

	void shadowPass() {
		commandBuffer->beginRenderpass(m_shadowData.pipeline->getRenderPass(), m_shadowData.pipeline->getFramebuffers()[swapchain->getCurrentImageIndex()], m_shadowData.resolution, m_shadowData.resolution);
		commandBuffer->bindPipeline(m_shadowData.pipeline);
		commandBuffer->updateViewport(m_shadowData.resolution, m_shadowData.resolution);

		for (auto& model : m_drawables) {
			m_forwardData.shader->pushConstants(commandBuffer->getHandle(), &model->m_modelMatrix);
			for (auto mesh : model->m_meshes) {
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
		}

		commandBuffer->endRenderPass();
	}

	void forwardPass() {
		commandBuffer->beginRenderpass(m_forwardData.pipeline->getRenderPass(), m_forwardData.pipeline->getFramebuffers()[swapchain->getCurrentImageIndex()], 1280, 720);
		commandBuffer->bindPipeline(m_forwardData.pipeline);
		commandBuffer->updateViewport(1280, 720);

		for (auto& model : m_drawables) {
			m_forwardData.shader->pushConstants(commandBuffer->getHandle(), &model->m_modelMatrix);
			for (auto mesh : model->m_meshes) {
				if (mesh->m_material == nullptr)
					continue;

				VkBuffer vertexBuffers[] = { mesh->m_vertexBuffer->getHandle() };
				VkDeviceSize offsets[] = { 0 };
				VkDescriptorSet descriptorSets[] = {
					m_forwardData.descriptorSet->getHandle(m_window->getSwapchain()->getCurrentFrameIndex()),
					mesh->m_material->m_descriptorSet->getHandle(m_window->getSwapchain()->getCurrentFrameIndex())
				};
				
				vkCmdBindDescriptorSets(commandBuffer->getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, mesh->m_material->m_shader->getPipelineLayout(), 0, 2, descriptorSets, 0, nullptr);
				vkCmdBindVertexBuffers(commandBuffer->getHandle(), 0, 1, vertexBuffers, offsets);
				vkCmdBindIndexBuffer(commandBuffer->getHandle(), mesh->m_indexBuffer->getHandle(), 0, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(commandBuffer->getHandle(), static_cast<uint32_t>(mesh->m_count), 1, 0, 0, 0);
			}
		}
		commandBuffer->endRenderPass();
	}

	void skyBoxPass() {
		commandBuffer->beginRenderpass(m_skyBoxData.pipeline->getRenderPass(), m_skyBoxData.pipeline->getFramebuffers()[swapchain->getCurrentImageIndex()], 1280, 720);
		commandBuffer->bindPipeline(m_skyBoxData.pipeline);
		commandBuffer->updateViewport(1280, 720);
		VkDescriptorSet skyBoxDescriptorSet = m_skyBoxData.descriptorSet->getHandle(swapchain->getCurrentFrameIndex());
		vkCmdBindDescriptorSets(commandBuffer->getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_skyBoxData.descriptorSet->getShader()->getPipelineLayout(), 0, 1, &skyBoxDescriptorSet, 0, nullptr); // todo: abstract these

		vkCmdDraw(commandBuffer->getHandle(), 36, 1, 0, 0);

		commandBuffer->endRenderPass();
	}

	void bloomPass() {
		// downsample
		for (int i = 0; i < mipChainLength; i++) {
			auto& mip = m_bloomData.mipChain[i];
			commandBuffer->beginRenderpass(m_bloomData.pipeline->getRenderPass(), m_bloomData.framebuffers[swapchain->getCurrentFrameIndex()][i], mip->getWidth(), mip->getHeight());
			commandBuffer->bindPipeline(m_bloomData.pipeline);
			commandBuffer->updateViewport(mip->getWidth(), mip->getHeight());

			VkDescriptorSet bloomDescriptorSet = m_bloomData.descriptorSets[i]->getHandle(swapchain->getCurrentFrameIndex());
			vkCmdBindDescriptorSets(commandBuffer->getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_bloomData.descriptorSets[i]->getShader()->getPipelineLayout(), 0, 1, &bloomDescriptorSet, 0, nullptr);
			if (i == 0)
				m_bloomData.pushConstant.mode = 0;
			else
				m_bloomData.pushConstant.mode = 1;
			m_bloomData.pushConstant.mipLevel = i - 1;
			m_bloomData.pushConstant.resolution = glm::vec2(mip->getWidth(), mip->getHeight());
			m_bloomData.descriptorSets[0]->getShader()->pushConstants(commandBuffer->getHandle(), &m_bloomData.pushConstant);

			vkCmdDraw(commandBuffer->getHandle(), 3, 1, 0, 0);

			commandBuffer->endRenderPass();
		}

		// upsample
		for (int i = mipChainLength; i > 1; i--) {
			auto& mip = m_bloomData.mipChain[i - 2];
			commandBuffer->beginRenderpass(m_bloomData.pipeline->getRenderPass(), m_bloomData.framebuffers[swapchain->getCurrentFrameIndex()][i - 2], mip->getWidth(), mip->getHeight());
			commandBuffer->bindPipeline(m_bloomData.pipeline);
			commandBuffer->updateViewport(mip->getWidth(), mip->getHeight());

			VkDescriptorSet bloomDescriptorSet = m_bloomData.descriptorSets[i]->getHandle(swapchain->getCurrentFrameIndex());
			vkCmdBindDescriptorSets(commandBuffer->getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_bloomData.descriptorSets[i]->getShader()->getPipelineLayout(), 0, 1, &bloomDescriptorSet, 0, nullptr);
			m_bloomData.pushConstant.mode = 2;
			m_bloomData.descriptorSets[0]->getShader()->pushConstants(commandBuffer->getHandle(), &m_bloomData.pushConstant);

			vkCmdDraw(commandBuffer->getHandle(), 3, 1, 0, 0);

			commandBuffer->endRenderPass();
		}
	}

	void toneMapping() {
		commandBuffer->beginRenderpass(m_toneMappingData.pipeline->getRenderPass(), m_toneMappingData.pipeline->getFramebuffers()[swapchain->getCurrentImageIndex()], 1280, 720);
		commandBuffer->bindPipeline(m_toneMappingData.pipeline);
		commandBuffer->updateViewport(1280, 720);
		VkDescriptorSet toneMappingDescriptorSet = m_toneMappingData.descriptorSet->getHandle(swapchain->getCurrentFrameIndex());
		vkCmdBindDescriptorSets(commandBuffer->getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_toneMappingData.descriptorSet->getShader()->getPipelineLayout(), 0, 1, &toneMappingDescriptorSet, 0, nullptr); // todo: abstract these

		vkCmdDraw(commandBuffer->getHandle(), 3, 1, 0, 0);
		commandBuffer->endRenderPass();
	}

	void finalPass() {
		commandBuffer->beginRenderpass(m_postProcessData.pipeline->getRenderPass(), m_postProcessData.pipeline->getFramebuffers()[swapchain->getCurrentImageIndex()], 1280, 720);
		commandBuffer->bindPipeline(m_postProcessData.pipeline);
		commandBuffer->updateViewport(1280, 720);

		VkDescriptorSet postProcessDescriptorSet = m_postProcessData.descriptorSet->getHandle(swapchain->getCurrentFrameIndex());
		vkCmdBindDescriptorSets(commandBuffer->getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_postProcessData.descriptorSet->getShader()->getPipelineLayout(), 0, 1, &postProcessDescriptorSet, 0, nullptr); // todo: abstract these

		vkCmdDraw(commandBuffer->getHandle(), 3, 1, 0, 0);

		//ImGui::Render();
		//ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer->getHandle());

		commandBuffer->endRenderPass();
	}

	void beginFrame() {
		glfwPollEvents();
		swapchain = m_window->getSwapchain();
		commandBuffer = swapchain->getCurrentCommandBuffer();
		updateUniformBuffer(swapchain->getCurrentFrameIndex());

		for (auto& mip : m_bloomData.mipChain) {
			//mip->clear({0,1,0,1});
			//mip->transitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		//m_gui->begin();
		commandBuffer->getFence()->wait();
		swapchain->acquireNexImage();
		commandBuffer->reset();
		commandBuffer->beginRecording();
	}

	void endFrame() {
		commandBuffer->endRecording();
		commandBuffer->submit();
		swapchain->present();
	}

	void guiUpdate() {
		ImGui::Begin("debug");
		ImGui::Text("fps: %.1f", 1.f / m_deltaTime);
		ImGui::Checkbox("SSAO", &m_settings.enableSSAO);
		ImGui::Checkbox("Bloom", &m_settings.enableBloom);
		ImGui::Checkbox("Shadow", &m_settings.enableShadow);
		ImGui::Checkbox("SkyBox", &m_settings.enableSkyBox);
		ImGui::End();
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(m_window->getHandle())) {
			beginFrame();

			//guiUpdate();
			depthPrePass();
			if (m_settings.enableSSAO)
				ssaoPass();
			if (m_settings.enableShadow)
				shadowPass();
			forwardPass();
			if (m_settings.enableSkyBox)
				skyBoxPass();
			if (m_settings.enableBloom)
				bloomPass();
			toneMapping();
			finalPass();

			endFrame();
		}

		vkDeviceWaitIdle(Device::getHandle());
	}

private:
	std::shared_ptr<Window> m_window;
	std::shared_ptr<Gui> m_gui;
	std::vector<UniformBuffer> m_sceneUBO;
	std::vector<std::shared_ptr<Model>> m_drawables;
	std::shared_ptr<CommandBuffer> commandBuffer = nullptr;
	std::shared_ptr<Swapchain> swapchain = nullptr;
	std::shared_ptr<Texture2D> m_currentTexture = nullptr;

	float m_deltaTime = 0.0f;

	struct Settings {
		bool enableSSAO = true;
		bool enableBloom = true;
		bool enableShadow = true;
		bool enableSkyBox = true;
	} m_settings;

	struct SceneDataUBO {
		struct Light {
			glm::vec4 color;
			glm::vec4 position;
		};
		Light lights[16];
		glm::mat4 lightSpace;
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec4 camPos;
		int lightCount = 2;
		glm::vec3 padding;
	} m_sceneData;

	struct DepthPrePass {
		std::shared_ptr<Texture2D> texture;
		std::shared_ptr<Pipeline> pipeline;
		std::shared_ptr<DescriptorSet> descriptorSet;
	}  m_depthPrePass;

	struct SSAOPass {
		std::shared_ptr<Texture2D> texture;
		std::shared_ptr<Pipeline> pipeline;
		std::shared_ptr<DescriptorSet> descriptorSet;
	} m_ssaoPass;

	struct ForwardData {
		std::shared_ptr<Shader> shader;
		std::shared_ptr<Pipeline> pipeline;
		std::shared_ptr<Texture2D> colorTexture;
		std::shared_ptr<Texture2D> resolveTexture;
		std::shared_ptr<Texture2D> depthTexture;
		std::shared_ptr<DescriptorSet> descriptorSet;
	} m_forwardData;

	struct PostProcessData {
		std::shared_ptr<Pipeline> pipeline;
		std::shared_ptr<DescriptorSet> descriptorSet;
	} m_postProcessData;

	struct SkyBoxData {
		std::shared_ptr<CubeMap> cubeMap;
		std::shared_ptr<Pipeline> pipeline;
		std::shared_ptr<DescriptorSet> descriptorSet;
	}  m_skyBoxData;

	struct ShadowData {
		std::shared_ptr<Texture2D> texture;
		std::shared_ptr<Pipeline> pipeline;
		std::shared_ptr<DescriptorSet> descriptorSet;
		uint32_t resolution = 4096;
		glm::vec3 lightPos = glm::vec3(50, 50.0f, .0f);
		glm::mat4 lightSpace;
		glm::mat4 lightProjection;
		glm::mat4 lightView;
	} m_shadowData;

	static const uint32_t mipChainLength = 5;
	struct BloomData {
		struct PushConstant {
			int mode;
			int mipLevel;
			glm::vec2 resolution;
		};
		PushConstant pushConstant;
		std::shared_ptr<Pipeline> pipeline;
		std::shared_ptr<Framebuffer> framebuffers[4][mipChainLength];
		std::shared_ptr<Texture2D> mipChain[mipChainLength];
		std::shared_ptr<DescriptorSet> descriptorSets[mipChainLength+1];
	} m_bloomData;

	struct ToneMappingData {
		std::shared_ptr<Texture2D> texture;
		std::shared_ptr<Pipeline> pipeline;
		std::shared_ptr<DescriptorSet> descriptorSet;
	} m_toneMappingData;

};

int main() {
#if defined(PLATFORM_WINDOWS)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	try {
		Renderer app;
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
#if defined(PLATFORM_WINDOWS)
	_CrtDumpMemoryLeaks();
#endif
	return EXIT_SUCCESS;
}