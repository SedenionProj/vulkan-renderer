#pragma once

class Texture2D;

class Material {
public:

	std::shared_ptr<Texture2D> m_albedo = nullptr;
};