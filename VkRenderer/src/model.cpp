#include "src/vulkan/texture.hpp"
#include "src/vulkan/buffer.hpp"
#include "src/vulkan/descriptorSet.hpp"
#include "src/vulkan/shader.hpp"
#include "src/vulkan/device.hpp"
#include "src/model.hpp"
#include "src/material.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

Mesh::Mesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, std::shared_ptr<Material> material)
	: m_material(material), m_count(indices.size()) {
	m_vertexBuffer = std::make_shared<VertexBuffer>(sizeof(vertices[0]) * vertices.size(), vertices.data());
	m_indexBuffer = std::make_shared<IndexBuffer>(sizeof(indices[0]) * indices.size(), indices.data());
}

void computeTangentBitangent(
	const glm::vec3& pos1, const glm::vec3& pos2, const glm::vec3& pos3,
	const glm::vec2& uv1, const glm::vec2& uv2, const glm::vec2& uv3,
	glm::vec3& tangent, glm::vec3& bitangent)
{
	glm::vec3 edge1 = pos2 - pos1;
	glm::vec3 edge2 = pos3 - pos1;
	glm::vec2 deltaUV1 = uv2 - uv1;
	glm::vec2 deltaUV2 = uv3 - uv1;

	float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent = f * (deltaUV2.y * edge1 - deltaUV1.y * edge2);
	bitangent = f * (-deltaUV2.x * edge1 + deltaUV1.x * edge2);
}

Model::Model(std::filesystem::path filePath) {

	tinyobj::ObjReaderConfig reader_config;

	tinyobj::ObjReader reader;

	if (!reader.ParseFromFile(ASSETS_PATH+filePath.string(), reader_config)) {
		if (!reader.Error().empty()) {
			std::cerr << "TinyObjReader: " << reader.Error();
		}
		exit(1);
	}

	if (!reader.Warning().empty()) {
		std::cout << "TinyObjReader: " << reader.Warning();
	}

	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	auto& materials = reader.GetMaterials();

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	std::unordered_map<std::string, std::shared_ptr<Texture2D>> textureCache;
	m_meshes.reserve(shapes.size());

	auto shader = std::make_shared<Shader>("spv/basicVert.spv", "spv/pbrFrag.spv");

	char pixels[4] = { 255, 255, 255, 255 };
	std::shared_ptr<Texture2D> defaultTexture = std::make_shared<Texture2D>(pixels,1,1);

	for (const auto& shape : shapes) {

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		indices.reserve(shape.mesh.indices.size());

		for (size_t i = 0; i < shape.mesh.indices.size(); i += 3) {
			std::array<Vertex, 3> triangleVertices;

			for (size_t j = 0; j < 3; ++j) {
				const auto& index = shape.mesh.indices[i + j];
				Vertex vertex{};

				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};

				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};

				triangleVertices[j] = vertex;
			}

			glm::vec3 tangent, bitangent;
			computeTangentBitangent(
				triangleVertices[0].pos, triangleVertices[1].pos, triangleVertices[2].pos,
				triangleVertices[0].texCoord, triangleVertices[1].texCoord, triangleVertices[2].texCoord,
				tangent, bitangent
			);

			for (size_t j = 0; j < 3; ++j) {
				auto& vertex = triangleVertices[j];
				vertex.tangent += tangent;
				vertex.bitangent += bitangent;

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}

				indices.emplace_back(uniqueVertices[vertex]);
			}
		}


		std::shared_ptr<Material> material = nullptr;

		if (shape.mesh.material_ids[0] > 0) {
			material = std::make_shared<Material>();
			material->m_shader = shader;
			material->m_descriptorSet = std::make_shared<DescriptorSet>(shader, 1);

			material->m_albedo = defaultTexture;
			material->m_normal = defaultTexture;
			material->m_specular = defaultTexture;


			const tinyobj::material_t* mp = &materials[shape.mesh.material_ids[0]];

			if (!mp->diffuse_texname.empty()) {
				// todo : texture loader helper function
				material->m_properties.brightness = 1;
				auto texPath = ASSETS_PATH / filePath.parent_path() / mp->diffuse_texname;

				std::string texPathStr = texPath.string();
				auto it = textureCache.find(texPathStr);
				if (it != textureCache.end()) {
					material->m_albedo = it->second;
				}
				else {
					auto tex = std::make_shared<Texture2D>(texPath);
					material->m_albedo = tex;
					textureCache[texPathStr] = tex;

					
					printf("loaded %s\n", mp->diffuse_texname.c_str());
				}
			}

			if (!mp->specular_texname.empty()) {
				material->m_properties.reflectance = 1.f;
				auto texPath = ASSETS_PATH / filePath.parent_path() / mp->specular_texname;

				std::string texPathStr = texPath.string();
				auto it = textureCache.find(texPathStr);
				if (it != textureCache.end()) {
					material->m_specular = it->second;
				}
				else {
					auto tex = std::make_shared<Texture2D>(texPath);
					material->m_specular = tex;
					textureCache[texPathStr] = tex;

					printf("loaded %s\n", mp->diffuse_texname.c_str());
				}

			}

			if (!mp->normal_texname.empty()) {
				auto texPath = ASSETS_PATH / filePath.parent_path() / mp->normal_texname;

				std::string texPathStr = texPath.string();
				auto it = textureCache.find(texPathStr);
				if (it != textureCache.end()) {
					material->m_normal = it->second;
				}
				else {
					auto tex = std::make_shared<Texture2D>(texPath);
					material->m_normal = tex;
					textureCache[texPathStr] = tex;

					printf("loaded %s\n", mp->diffuse_texname.c_str());
				}
			}

			if (!mp->bump_texname.empty()) {
				material->m_properties.roughness = 1.f;
				auto texPath = ASSETS_PATH / filePath.parent_path() / mp->bump_texname;

				std::string texPathStr = texPath.string();
				auto it = textureCache.find(texPathStr);
				if (it != textureCache.end()) {
					material->m_normal = it->second;
				}
				else {
					auto tex = std::make_shared<Texture2D>(texPath);
					material->m_normal = tex;
					textureCache[texPathStr] = tex;

					printf("loaded %s\n", mp->diffuse_texname.c_str());
				}
			}

			material->createUniformBuffers();
			material->m_descriptorSet->setTexture(material->m_albedo, 1);
			material->m_descriptorSet->setTexture(material->m_specular, 2);
			material->m_descriptorSet->setTexture(material->m_normal, 3);
		}


		std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(vertices, indices, material);
		
		m_meshes.emplace_back(mesh);
	}
}

void Model::createCube() {
	std::vector<Vertex> vertices = {
		// FRONT (+Z)
		{{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}},
		{{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}},
		{{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}},
		{{-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}},

		// BACK (-Z)
		{{ 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}},
		{{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}},
		{{-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}},
		{{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}},

		// RIGHT (+X)
		{{ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
		{{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
		{{ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
		{{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},

		// LEFT (-X)
		{{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
		{{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
		{{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
		{{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},

		// TOP (+Y)
		{{-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}},
		{{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}},
		{{ 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}},
		{{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}},

		// BOTTOM (-Y)
		{{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}},
		{{ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}},
		{{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}},
		{{-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}},
	};

	std::vector<uint32_t> indices = {
		// Front face
		0, 1, 2,  2, 3, 0,

		// Back face
		4, 5, 6,  6, 7, 4,

		// Right face
		8, 9,10, 10,11, 8,

		// Left face
	   12,13,14, 14,15,12,

	   // Top face
	  16,17,18, 18,19,16,

	  // Bottom face
	 20,21,22, 22,23,20
	};

	char pixels[4] = { 255, 128, 216, 255 };
	std::shared_ptr<Texture2D> defaultTexture = std::make_shared<Texture2D>(pixels, 1, 1);

	std::shared_ptr<Material> material = std::make_shared<Material>();
	material->m_albedo = defaultTexture;
	material->m_normal = defaultTexture;
	material->m_specular = defaultTexture;
	material->m_shader = std::make_shared<Shader>("spv/basicVert.spv", "spv/pbrFrag.spv");
	material->m_properties.brightness = 10.f;

	material->m_descriptorSet = std::make_shared<DescriptorSet>(material->m_shader, 1);
	material->createUniformBuffers();
	material->m_descriptorSet->setTexture(material->m_albedo, 1);
	material->m_descriptorSet->setTexture(material->m_specular, 2);
	material->m_descriptorSet->setTexture(material->m_normal, 3);

	std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(vertices, indices, material);

	m_meshes.emplace_back(mesh);
}