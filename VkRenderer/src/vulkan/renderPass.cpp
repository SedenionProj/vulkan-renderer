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

	for (auto& attachmentInfo : attachmentInfos) {
		VkAttachmentReference ref{};
		ref.attachment = attachmentInfo.binding;

		std::shared_ptr<Texture> tex = attachmentInfo.texture;
		VkAttachmentDescription desc{};
		desc.format = tex->getFormat();
		desc.samples = tex->getSampleCount();
		desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;



		desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		if (attachmentInfo.type == Attachment::Type::COLOR) {
			desc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // todo
		}
		if (attachmentInfo.type == Attachment::Type::DEPTH) {
			desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depthAttachmentRef.push_back(ref);
		}
		if (attachmentInfo.type == Attachment::Type::PRESENT) {
			desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		}

		if (clear) {
			desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		}
		else {
			desc.initialLayout = desc.finalLayout;
			desc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		}

		if (attachmentInfo.type == Attachment::Type::COLOR || attachmentInfo.type == Attachment::Type::PRESENT) {
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
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
	renderPassInfo.pAttachments = attachmentDescriptions.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	//renderPassInfo.dependencyCount = 1;
	//renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(Device::getHandle(), &renderPassInfo, nullptr, &m_handle) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass");
	}
}

RenderPass::~RenderPass() {
	vkDestroyRenderPass(Device::getHandle(), m_handle, nullptr);
}