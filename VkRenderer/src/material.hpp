#pragma once

class Texture2D;
class Shader;
class DescriptorSet;

class Material {
public:
	Material();

	std::shared_ptr<Texture2D> m_albedo = nullptr;
	std::shared_ptr<Texture2D> m_specular = nullptr;
	std::shared_ptr<Texture2D> m_normal = nullptr;

	std::shared_ptr<Shader> m_shader = nullptr;
	std::shared_ptr<DescriptorSet> m_descriptorSet = nullptr;
};