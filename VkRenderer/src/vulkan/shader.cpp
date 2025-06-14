#include <spirv_cross.hpp>
#include "src/vulkan/shader.hpp"
#include "src/vulkan/device.hpp"

static std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

VkFormat spirvTypeToVkFormat(const spirv_cross::SPIRType& type) {
	using namespace spirv_cross;

	if (type.basetype == SPIRType::Float) {
		switch (type.columns) {
		case 1:
			switch (type.vecsize) {
			case 1: return VK_FORMAT_R32_SFLOAT;
			case 2: return VK_FORMAT_R32G32_SFLOAT;
			case 3: return VK_FORMAT_R32G32B32_SFLOAT;
			case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
			}
			break;
		}
	}
	else if (type.basetype == SPIRType::Int) {
		switch (type.vecsize) {
		case 1: return VK_FORMAT_R32_SINT;
		case 2: return VK_FORMAT_R32G32_SINT;
		case 3: return VK_FORMAT_R32G32B32_SINT;
		case 4: return VK_FORMAT_R32G32B32A32_SINT;
		}
	}
	else if (type.basetype == SPIRType::UInt) {
		switch (type.vecsize) {
		case 1: return VK_FORMAT_R32_UINT;
		case 2: return VK_FORMAT_R32G32_UINT;
		case 3: return VK_FORMAT_R32G32B32_UINT;
		case 4: return VK_FORMAT_R32G32B32A32_UINT;
		}
	}

	throw std::runtime_error("Unsupported vertex attribute format");
}

uint32_t formatSize(VkFormat format) {
	switch (format) {
	case VK_FORMAT_R32_SFLOAT: return 4;
	case VK_FORMAT_R32G32_SFLOAT: return 8;
	case VK_FORMAT_R32G32B32_SFLOAT: return 12;
	case VK_FORMAT_R32G32B32A32_SFLOAT: return 16;
	case VK_FORMAT_R32_SINT: return 4;
	case VK_FORMAT_R32G32_SINT: return 8;
	case VK_FORMAT_R32G32B32_SINT: return 12;
	case VK_FORMAT_R32G32B32A32_SINT: return 16;
	case VK_FORMAT_R32_UINT: return 4;
	case VK_FORMAT_R32G32_UINT: return 8;
	case VK_FORMAT_R32G32B32_UINT: return 12;
	case VK_FORMAT_R32G32B32A32_UINT: return 16;
	default: throw std::runtime_error("Unhandled VkFormat in format_size");
	}
}


Shader::Shader() {
	auto vertShaderCode = readFile("assets/vert.spv");
	auto fragShaderCode = readFile("assets/frag.spv");

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	m_shaderStages[0] = vertShaderStageInfo;
	m_shaderStages[1] = fragShaderStageInfo;

	spirv_cross::Compiler comp(reinterpret_cast<uint32_t*>(vertShaderCode.data()), vertShaderCode.size()/4);
	spirv_cross::ShaderResources resources = comp.get_shader_resources();

	uint32_t currOffset = 0;
	m_attributeDescriptions.reserve(resources.stage_inputs.size());
	for (auto& resource : resources.stage_inputs) {
		const spirv_cross::SPIRType& type = comp.get_type(resource.type_id);
	
		VkVertexInputAttributeDescription desc{};
		desc.binding = comp.get_decoration(resource.id, spv::DecorationBinding);
		desc.location = comp.get_decoration(resource.id, spv::DecorationLocation);
		desc.format = spirvTypeToVkFormat(type);
		desc.offset = currOffset;
		currOffset += formatSize(desc.format);
		m_attributeDescriptions.emplace_back(desc);
	}

	m_vertexInputStride = currOffset;
	
	createPipelineLayout();
}

Shader::~Shader()
{
	vkDestroyDescriptorSetLayout(Device::getHandle(), m_descriptorSetLayout, nullptr);

}

VkShaderModule Shader::createShaderModule(const std::vector<char>& code) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(Device::getHandle(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}

	return shaderModule;
}

void Shader::createPipelineLayout() {
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(Device::getHandle(), &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(Device::getHandle(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}
}