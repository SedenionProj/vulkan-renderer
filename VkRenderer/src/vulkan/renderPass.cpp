#include "src/vulkan/renderPass.hpp"
#include "src/vulkan/device.hpp"
#include "src/vulkan/texture.hpp"

RenderPass::RenderPass(std::initializer_list<Attachment> attachmentInfos) {
	std::vector<VkAttachmentDescription> attachmentDescriptions;
	attachmentDescriptions.reserve(attachmentInfos.size());

	std::vector<VkAttachmentReference> attachmentReferences;
	attachmentReferences.reserve(attachmentInfos.size());

	VkAttachmentReference colorAttachmentRef;
	VkAttachmentReference depthAttachmentRef;
	VkAttachmentReference colorAttachmentResolveRef;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	for (auto& attachmentInfo : attachmentInfos) {
		// ref
		VkAttachmentReference ref{};
		ref.attachment = attachmentInfo.binding;
		if (attachmentInfo.type == Attachment::Type::COLOR || attachmentInfo.type == Attachment::Type::PRESENT) {
			ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}
		if (attachmentInfo.type == Attachment::Type::DEPTH) {
			ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}

		// desc
		std::shared_ptr<Texture2D> tex = attachmentInfo.texture;
		VkAttachmentDescription desc{};
		desc.format = tex->getFormat();
		desc.samples = tex->getSampleCount();
		desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		if (attachmentInfo.type == Attachment::Type::COLOR) {
			colorAttachmentRef = ref;
			desc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // todo
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentRef;
		}
		if (attachmentInfo.type == Attachment::Type::DEPTH) {
			depthAttachmentRef = ref;
			desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			subpass.pDepthStencilAttachment = &depthAttachmentRef;
		}
		if (attachmentInfo.type == Attachment::Type::PRESENT) {
			colorAttachmentResolveRef = ref;
			desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentResolveRef;
		}

		attachmentDescriptions.emplace_back(desc);
	}

	
	

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcAccessMask = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
	renderPassInfo.pAttachments = attachmentDescriptions.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 0;
	//renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(Device::getHandle(), &renderPassInfo, nullptr, &m_handle) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass");
	}
}

RenderPass::~RenderPass() {
	vkDestroyRenderPass(Device::getHandle(), m_handle, nullptr);
}