#pragma once

#include "src/vulkan/buffer.hpp"

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
};

class VertexBuffer;
class IndexBuffer;
class Material;

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
	Model(std::filesystem::path filePath);

	std::vector<std::shared_ptr<Mesh>> m_meshes;
};