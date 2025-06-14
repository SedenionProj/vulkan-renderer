#include "src/vulkan/texture.hpp"
#include "src/vulkan/buffer.hpp"
#include "src/model.hpp"
#include "src/material.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

Mesh::Mesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, std::shared_ptr<Material> material)
	: m_material(material), m_count(indices.size()) {
	m_vertexBuffer = std::make_shared<VertexBuffer>(sizeof(vertices[0]) * vertices.size(), vertices.data());
	m_indexBuffer = std::make_shared<IndexBuffer>(sizeof(indices[0]) * indices.size(), indices.data());
}

Model::Model(std::filesystem::path filePath) {

	tinyobj::ObjReaderConfig reader_config;
	//reader_config.mtl_search_path = "assets/sponza"; // Path to material files

	tinyobj::ObjReader reader;

	if (!reader.ParseFromFile(filePath.string(), reader_config)) {
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

	std::unordered_map<std::string, std::shared_ptr<Texture2D>> textureCache;
	m_meshes.reserve(shapes.size());

	for (const auto& shape : shapes) {

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		vertices.reserve(shape.mesh.indices.size());
		indices.reserve(shape.mesh.indices.size());

		for (const auto& index : shape.mesh.indices) {
			Vertex vertex{};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = { 1.0f, 1.0f, 1.0f };



			vertices.emplace_back(vertex);
			indices.emplace_back(indices.size());
		}
		std::shared_ptr<Material> material = std::make_shared<Material>();

		if (shape.mesh.material_ids[0] > 0) {
			const tinyobj::material_t* mp = &materials[shape.mesh.material_ids[0]];

			if (!mp->diffuse_texname.empty()) {
				auto texPath = filePath.parent_path() / mp->diffuse_texname;

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
		}


		std::shared_ptr<Mesh> mesh = std::make_unique<Mesh>(vertices, indices, material);
		
		m_meshes.emplace_back(mesh);
	}
}