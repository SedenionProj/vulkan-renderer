#include "src/material.hpp"
#include "src/vulkan/descriptorSet.hpp"

Material::Material()
{
}

void Material::createUniformBuffers()
{
	m_uniformBuffers.reserve(MAX_FRAMES_IN_FLIGHT);
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		m_uniformBuffers.emplace_back(sizeof(MaterialProperties));
		m_uniformBuffers[i].setData(&m_properties, sizeof(MaterialProperties));
		m_descriptorSet->setUniform(m_uniformBuffers, 0, i);
	}
}
