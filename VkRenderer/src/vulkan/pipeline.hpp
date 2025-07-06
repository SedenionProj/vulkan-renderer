#pragma once
#include "src/vulkan/renderPass.hpp"
#include "src/vulkan/shader.hpp"
#include "src/vulkan/framebuffer.hpp"

struct PipelineDesc {
	std::shared_ptr<Shader> shader;
	std::initializer_list<Attachment> attachmentInfos;
	std::shared_ptr<Swapchain> swapchain = nullptr;
	bool clear = true;
	VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
};

class Pipeline {
public:
	Pipeline(PipelineDesc info);
	~Pipeline();
	
	VkPipeline getHandle() { return m_handle; }
	std::shared_ptr<RenderPass> getRenderPass() { return m_renderPass; }
	std::vector<std::shared_ptr<Framebuffer>>& getFramebuffers() { return m_framebuffers; }
private:

	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_handle;

	std::shared_ptr<Shader> m_shader;
	std::shared_ptr<RenderPass> m_renderPass;
	std::vector<std::shared_ptr<Framebuffer>> m_framebuffers;
};