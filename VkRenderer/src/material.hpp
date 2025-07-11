#pragma once
#include "src/vulkan/vkHeader.hpp"
#include "src/vulkan/buffer.hpp"

struct MaterialProperties {
	float brightness = 0.f;
	float roughness = 0.f;
	float reflectance = 0.f;
};

class Material {
public:
	Material();

	void createUniformBuffers();

	MaterialProperties m_properties;

	std::shared_ptr<Texture2D> m_albedo = nullptr;
	std::shared_ptr<Texture2D> m_specular = nullptr;
	std::shared_ptr<Texture2D> m_normal = nullptr;

	std::shared_ptr<Shader> m_shader = nullptr;
	std::shared_ptr<DescriptorSet> m_descriptorSet = nullptr;

	std::vector<UniformBuffer> m_uniformBuffers;
};