#version 450

layout(set = 0, binding = 1) uniform samplerCube cubeMapTexture;

layout(location = 0) in vec3 direction;
layout(location = 0) out vec4 outColor;

void main() {
	vec4 textureSample = texture(cubeMapTexture, direction);

	outColor = vec4(textureSample.rgb, 1);
}