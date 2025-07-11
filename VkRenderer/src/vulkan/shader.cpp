#include <spirv_cross.hpp>
#include "src/vulkan/shader.hpp"
#include "src/vulkan/device.hpp"

static std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	DEBUG_ASSERT(file.is_open(), "failed to open file %s", filename.c_str());

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

	DEBUG_ERROR("Unsupported vertex attribute format");
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
	default: DEBUG_ERROR("Unhandled VkFormat");
	}
}

Shader::Shader(const char* vertPath, const char* fragPath) {
	std::string vertFullPath = std::string(ASSETS_PATH) + vertPath;
	std::string fragFullPath = std::string(ASSETS_PATH) + fragPath;

	auto vertShaderCode = readFile(vertFullPath.c_str());
	auto fragShaderCode = readFile(fragFullPath.c_str());

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

	loadData(vertShaderCode, VK_SHADER_STAGE_VERTEX_BIT);
	loadData(fragShaderCode, VK_SHADER_STAGE_FRAGMENT_BIT);

	createPipelineLayout();
}

Shader::~Shader()
{
	destroy();
	for(auto& layout : m_descriptorSetLayouts)
		vkDestroyDescriptorSetLayout(Device::getHandle(), layout, nullptr);
}

void Shader::destroy()
{
	for (auto& stage : m_shaderStages) {
		vkDestroyShaderModule(Device::getHandle(), stage.module, nullptr);
	}
}

VkShaderModule Shader::createShaderModule(const std::vector<char>& code) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	VkShaderModule shaderModule;
	VK_CKECK(vkCreateShaderModule(Device::getHandle(), &createInfo, nullptr, &shaderModule));

	return shaderModule;
}

void Shader::loadData(std::vector<char>& code, VkShaderStageFlags stage)
{
	spirv_cross::Compiler comp(reinterpret_cast<uint32_t*>(code.data()), code.size() / 4);
	spirv_cross::ShaderResources resources = comp.get_shader_resources();

	// vertex data

	if (stage == VK_SHADER_STAGE_VERTEX_BIT) {
		uint32_t currOffset = 0;
		m_attributeDescriptions.reserve(resources.stage_inputs.size());

		// we want stage_inputs sorted by location
		std::sort(
			resources.stage_inputs.begin(),
			resources.stage_inputs.end(),
			[&comp](const spirv_cross::Resource& a, const spirv_cross::Resource& b) {
				return comp.get_decoration(a.id, spv::DecorationLocation) <
					comp.get_decoration(b.id, spv::DecorationLocation);
			}
		);

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
	}

	// uniform data
	for (auto& uniform : resources.uniform_buffers) {
		uint32_t binding = comp.get_decoration(uniform.id, spv::DecorationBinding);

		uint32_t targetBinding = binding;

		auto it = std::find_if(
			m_descriptorInfos.begin(),
			m_descriptorInfos.end(),
			[targetBinding](const DescriptorInfo& info) {
				return info.binding == targetBinding;
			}
		);

		if (it != m_descriptorInfos.end()) {
			DescriptorInfo& match = *it;
			match.shaderStage |= stage;
		} else {
			auto& bufferType = comp.get_type(uniform.base_type_id);
			auto bufferSize = comp.get_declared_struct_size(bufferType);

			m_descriptorInfos.push_back({
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			stage,
			(uint32_t)bufferSize,
			binding,
			comp.get_decoration(uniform.id, spv::DecorationDescriptorSet),
			});
		}
	}

	// image sampler data
	for (auto& image : resources.sampled_images) {

		m_descriptorInfos.push_back({
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			stage,
			0,
			comp.get_decoration(image.id, spv::DecorationBinding),
			comp.get_decoration(image.id, spv::DecorationDescriptorSet),
			});
	}
}

void Shader::createPipelineLayout() {
	bool done = false;

	for (int i = 0; i < 4; i++) {
		std::vector<VkDescriptorSetLayoutBinding> bindings;

		for (auto& info : m_descriptorInfos) {
			if (info.set == i) {
				VkDescriptorSetLayoutBinding layoutBinding{};
				layoutBinding.binding = info.binding;
				layoutBinding.descriptorType = info.type;
				layoutBinding.stageFlags = info.shaderStage;
				layoutBinding.descriptorCount = 1;

				bindings.push_back(layoutBinding);
			}
		}

		if (bindings.empty())
			continue;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		VkDescriptorSetLayout layout;
		VK_CKECK(vkCreateDescriptorSetLayout(Device::getHandle(), &layoutInfo, nullptr, &layout));

		m_descriptorSetLayouts.push_back(layout);
	}

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(m_descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = m_descriptorSetLayouts.data();

	VK_CKECK(vkCreatePipelineLayout(Device::getHandle(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout));
}