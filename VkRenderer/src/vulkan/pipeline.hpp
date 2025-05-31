#pragma once
#include "src/vulkan/renderPass.hpp"
#include "src/vulkan/shader.hpp"

class Pipeline {
public:
	Pipeline(std::shared_ptr<Shader> shader, std::shared_ptr<Swapchain> swapchain, std::shared_ptr<RenderPass> renderPass);
	~Pipeline();
	
	VkPipeline getHandle() { return m_handle; }

private:
	void createGraphicsPipeline();
private:
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_handle;

	std::shared_ptr<Swapchain> m_swapchain;
	std::shared_ptr<Shader> m_shader;
	std::shared_ptr<RenderPass> m_renderPass;
};