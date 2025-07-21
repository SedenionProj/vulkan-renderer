#version 450

layout(set = 0, binding = 0) uniform sampler2D u_inputTexture;
layout(set = 0, binding = 1) uniform sampler2D u_bloomTexture;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

vec3 ACESTonemap(vec3 color) {
	mat3 m1 = mat3(
		0.59719, 0.07600, 0.02840,
		0.35458, 0.90834, 0.13383,
		0.04823, 0.01566, 0.83777);
	mat3 m2 = mat3(
		 1.60475, -0.10208, -0.00327,
		 -0.53108, 1.10813, -0.07276,
		 -0.07367, -0.00605, 1.07602);

	vec3 v = m1 * color;
	vec3 a = v * (v + 0.0245786) - 0.000090537;
	vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
	return clamp(m2 * (a / b), 0.0, 1.0);
}

void main() {
	vec3 scene = texture(u_inputTexture, fragUV).rgb;
	vec3 bloom = texture(u_bloomTexture, fragUV).rgb;
	vec3 result = scene + bloom;
	result = ACESTonemap(result);
	result = pow(result, vec3(1.0 / 2.2));
	outColor = vec4(result, 1.0);
}