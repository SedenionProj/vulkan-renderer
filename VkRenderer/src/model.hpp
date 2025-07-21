#pragma once

#include "src/vulkan/vkHeader.hpp"

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 texCoord;
	glm::vec3 tangent = glm::vec3(0.0f);
	glm::vec3 bitangent = glm::vec3(0.0f);

	bool operator==(const Vertex& other) const {
		return pos == other.pos && texCoord == other.texCoord && normal == other.normal;
	}
};

class Mesh {
public:
	Mesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, std::shared_ptr<Material> material);
	uint32_t m_count;
	std::shared_ptr<VertexBuffer> m_vertexBuffer;
	std::shared_ptr<IndexBuffer> m_indexBuffer;
	std::shared_ptr<Material> m_material;
};

class Model {
public:
	Model() = default;
	Model(std::filesystem::path filePath);
	void createCube();

	std::vector<std::shared_ptr<Mesh>> m_meshes;
	glm::mat4 m_modelMatrix = glm::mat4(1.0f);
};