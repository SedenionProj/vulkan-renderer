#pragma once
#include "src/vulkan/renderPass.hpp"
#include "src/vulkan/shader.hpp"

class Pipeline {
public:
	Pipeline(std::shared_ptr<Shader> shader, std::shared_ptr<RenderPass> renderPass, VkSampleCountFlagBits samples);
	~Pipeline();
	
	VkPipeline getHandle() { return m_handle; }

private:
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_handle;

	std::shared_ptr<Shader> m_shader;
	std::shared_ptr<RenderPass> m_renderPass;
};