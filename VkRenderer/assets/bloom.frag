#version 450

layout(set = 0, binding = 0) uniform sampler2D inputImage;

layout(push_constant) uniform push {
	int mode;
	int mipLevel;
	vec2 mipResolution;
} u_data;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

const float filterRadius = 0.005;

// from https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom

vec3 PowVec3(vec3 v, float p) {
	return vec3(pow(v.x, p), pow(v.y, p), pow(v.z, p));
}

const float invGamma = 1.0 / 2.2;
vec3 ToSRGB(vec3 v) { return PowVec3(v, invGamma); }

float sRGBToLuma(vec3 col) {
	return dot(col, vec3(0.299f, 0.587f, 0.114f));
}

float KarisAverage(vec3 col) {
	float luma = sRGBToLuma(ToSRGB(col)) * 0.25f;
	return 1.0f / (1.0f + luma);
}

void main() {
	switch (u_data.mode) {
	case 0:
		// bloom pre-filter
		vec3 col = texture(inputImage, fragUV).rgb;
		float brightness = dot(col, vec3(0.2126, 0.7152, 0.0722));
		if (brightness > 1.f) {
			outColor = vec4(col, 1.0);
		} else {
			outColor = vec4(0.0, 0.0, 0.0, 1.0);
		}
		break;
	case 1:
		// downsample
		vec3 downsample;

		vec2 srcTexelSize = 1.0 / u_data.mipResolution;
		float x = srcTexelSize.x;
		float y = srcTexelSize.y;

		vec3 a = texture(inputImage, vec2(fragUV.x - 2*x, fragUV.y + 2*y)).rgb;
		vec3 b = texture(inputImage, vec2(fragUV.x,       fragUV.y + 2*y)).rgb;
		vec3 c = texture(inputImage, vec2(fragUV.x + 2*x, fragUV.y + 2*y)).rgb;

		vec3 d = texture(inputImage, vec2(fragUV.x - 2*x, fragUV.y)).rgb;
		vec3 e = texture(inputImage, vec2(fragUV.x,       fragUV.y)).rgb;
		vec3 f = texture(inputImage, vec2(fragUV.x + 2*x, fragUV.y)).rgb;

		vec3 g = texture(inputImage, vec2(fragUV.x - 2*x, fragUV.y - 2*y)).rgb;
		vec3 h = texture(inputImage, vec2(fragUV.x,       fragUV.y - 2*y)).rgb;
		vec3 i = texture(inputImage, vec2(fragUV.x + 2*x, fragUV.y - 2*y)).rgb;

		vec3 j = texture(inputImage, vec2(fragUV.x - x, fragUV.y + y)).rgb;
		vec3 k = texture(inputImage, vec2(fragUV.x + x, fragUV.y + y)).rgb;
		vec3 l = texture(inputImage, vec2(fragUV.x - x, fragUV.y - y)).rgb;
		vec3 m = texture(inputImage, vec2(fragUV.x + x, fragUV.y - y)).rgb;

		vec3 groups[5];

		if (u_data.mipLevel == 0) {
			groups[0] = (a + b + d + e) * (0.125f / 4.0f);
			groups[1] = (b + c + e + f) * (0.125f / 4.0f);
			groups[2] = (d + e + g + h) * (0.125f / 4.0f);
			groups[3] = (e + f + h + i) * (0.125f / 4.0f);
			groups[4] = (j + k + l + m) * (0.5f   / 4.0f);

			for (int i = 0; i < 5; ++i)
				groups[i] *= KarisAverage(groups[i]);

			downsample = groups[0] + groups[1] + groups[2] + groups[3] + groups[4];
			downsample = max(downsample, vec3(0.0001));
		} else {
			downsample = e * 0.125f;
			downsample += (a + c + g + i) * 0.03125f;
			downsample += (b + d + f + h) * 0.0625f;
			downsample += (j + k + l + m) * 0.125f;
		}

		outColor = vec4(downsample, 1.0);
		break;
	case 2:
		// upsample
		vec3 upsample;
		float x1 = filterRadius;
		float y1 = filterRadius;

		vec3 a1 = texture(inputImage, vec2(fragUV.x - x1, fragUV.y + y1)).rgb;
		vec3 b1 = texture(inputImage, vec2(fragUV.x,      fragUV.y + y1)).rgb;
		vec3 c1 = texture(inputImage, vec2(fragUV.x + x1, fragUV.y + y1)).rgb;

		vec3 d1 = texture(inputImage, vec2(fragUV.x - x1, fragUV.y)).rgb;
		vec3 e1 = texture(inputImage, vec2(fragUV.x,      fragUV.y)).rgb;
		vec3 f1 = texture(inputImage, vec2(fragUV.x + x1, fragUV.y)).rgb;

		vec3 g1 = texture(inputImage, vec2(fragUV.x - x1, fragUV.y - y1)).rgb;
		vec3 h1 = texture(inputImage, vec2(fragUV.x,      fragUV.y - y1)).rgb;
		vec3 i1 = texture(inputImage, vec2(fragUV.x + x1, fragUV.y - y1)).rgb;

		upsample  = e1 * 4.0;
		upsample += (b1 + d1 + f1 + h1) * 2.0;
		upsample += (a1 + c1 + g1 + i1);
		upsample *= 1.0 / 16.0;

		outColor = vec4(upsample, 1.0);
		break;
	}
}
