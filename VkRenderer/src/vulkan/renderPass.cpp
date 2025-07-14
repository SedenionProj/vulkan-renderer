#include "src/vulkan/renderPass.hpp"
#include "src/vulkan/device.hpp"
#include "src/vulkan/texture.hpp"

RenderPass::RenderPass(std::initializer_list<Attachment> attachmentInfos, bool clear) {
	std::vector<VkAttachmentDescription> attachmentDescriptions;
	attachmentDescriptions.reserve(attachmentInfos.size());

	std::vector<VkAttachmentReference> colorAttachmentRef;
	std::vector<VkAttachmentReference> depthAttachmentRef;
	VkAttachmentReference colorAttachmentResolveRef;

	bool resolve = false;
	uint32_t binding = 0;

	for (auto& attachmentInfo : attachmentInfos) {
		VkAttachmentReference ref{};
		ref.attachment = binding;
		binding++;

		std::shared_ptr<Texture> tex = attachmentInfo.texture;
		VkAttachmentDescription desc{};
		desc.format = tex->getFormat();
		desc.samples = tex->getSampleCount();
		desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		switch (tex->getType()) {
		case TextureType::COLOR:
			desc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // todo
			break;
		case TextureType::DEPTH:
			desc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depthAttachmentRef.push_back(ref);
			break;
		case TextureType::SWAPCHAIN:
			desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			break;
		}

		if (clear) {
			if (tex->getType() == TextureType::DEPTH)
				m_clearValues.push_back({ 1.,0.});
			else
				m_clearValues.push_back({ 0.1,0.1,0.1,0.1 });
			desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		} else {
			desc.initialLayout = desc.finalLayout;
			desc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		}

		if (tex->getType() == TextureType::COLOR || tex->getType() == TextureType::SWAPCHAIN) {
			ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			if (attachmentInfo.resolve) {
				colorAttachmentResolveRef = ref;
				resolve = true;
			} else {
				colorAttachmentRef.push_back(ref);
			}
				
		}
		attachmentDescriptions.emplace_back(desc);
	}

	
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRef.size());
	subpass.pColorAttachments = colorAttachmentRef.data();
	subpass.pDepthStencilAttachment = depthAttachmentRef.data();
	subpass.pResolveAttachments = resolve ? &colorAttachmentResolveRef : nullptr;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcAccessMask = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
	renderPassInfo.pAttachments = attachmentDescriptions.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VK_CHECK(vkCreateRenderPass(Device::getHandle(), &renderPassInfo, nullptr, &m_handle));
}

RenderPass::~RenderPass() {
	vkDestroyRenderPass(Device::getHandle(), m_handle, nullptr);
}