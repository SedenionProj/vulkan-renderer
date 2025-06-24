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

		m_model = std::make_shared<Model>(MODEL_PATH_S.c_str());
		
		m_pipeline = std::make_shared<Pipeline>(m_model->m_meshes[0]->m_material->m_shader, m_window->getSwapchain(), m_renderPass);


		m_uniformBuffers.reserve(MAX_FRAMES_IN_FLIGHT);

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			m_uniformBuffers.emplace_back((uint32_t)sizeof(UniformBufferObject));
		}
		

		for (auto mesh : m_model->m_meshes) {
			if (mesh->m_material->m_albedo != nullptr) {
				mesh->m_material->m_descriptorSet->setUniform(m_uniformBuffers, 0);
			}
		}

		//m_descriptorSet = std::make_shared<DescriptorSet>(m_model->m_meshes[0]->m_material->m_shader);
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

		m_uniformBuffers[currentImage].setData(&ubo, sizeof(ubo));

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

		// per pass
		commandBuffer->beginRenderpass(m_renderPass, m_swapChainFramebuffers[swapchain->getCurrentImageIndex()], 1280, 720);
		commandBuffer->bindPipeline(m_pipeline);

		commandBuffer->updateViewport(1280, 720);

		for (auto mesh : m_model->m_meshes) {
			if (mesh->m_material->m_albedo != nullptr && mesh->m_material->m_specular != nullptr && mesh->m_material->m_normal) {
				//mesh->m_material->m_descriptorSet->setUniform(m_uniformBuffers, 0, m_window->getSwapchain()->getCurrentFrameIndex());
			}
			else
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

	void cleanup() {

	}



	bool hasStencilComponent(VkFormat format) {
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
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
	std::shared_ptr<Model> m_model;

	std::shared_ptr<RenderPass> m_renderPass;
	std::shared_ptr<DescriptorSet> m_descriptorSet;
	std::shared_ptr<Pipeline> m_pipeline;
	std::vector<UniformBuffer> m_uniformBuffers;
	std::shared_ptr<Texture2D> m_depthTexture;
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