#version 450

layout(set = 0, binding = 0) uniform sampler2D inputImage;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

void main() {
	outColor = vec4(texture(inputImage, fragUV).rgb, 1);
}