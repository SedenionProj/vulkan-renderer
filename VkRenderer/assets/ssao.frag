#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 fragUV;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 lightSpace;
    vec4 camPos;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D u_depthMap;

const int KERNEL_SIZE = 64;
const float RADIUS = 0.5;
const float BIAS = 0.025;

// temp
vec3 kernel[KERNEL_SIZE] = vec3[](
    vec3(0.5381, 0.1856, -0.4319), vec3(0.1379, 0.2486, 0.4430),
    vec3(0.3371, 0.5679, -0.0057), vec3(-0.6999, -0.0451, -0.0019),
    vec3(0.0689, -0.1598, -0.8547), vec3(0.0560, 0.0069, -0.1843),
    vec3(-0.0146, 0.1402, 0.0762), vec3(0.0100, -0.1924, -0.0344),
    vec3(-0.3577, -0.5301, -0.4358), vec3(-0.3169, 0.1063, 0.0158),
    vec3(0.0103, -0.5869, 0.0046), vec3(-0.0897, -0.4940, 0.3287),
    vec3(0.7119, -0.0154, -0.0918), vec3(-0.0533, 0.0596, -0.5411),
    vec3(0.0352, -0.0631, 0.5460), vec3(-0.4776, 0.2847, -0.0271),
    vec3(-0.3084, 0.1390, -0.3859), vec3(-0.5971, -0.4863, 0.3077),
    vec3(-0.6188, -0.3285, -0.4607), vec3(0.6785, -0.6664, -0.1891),
    vec3(-0.2115, 0.0774, 0.5043), vec3(0.1831, -0.5791, -0.5351),
    vec3(-0.4462, -0.4225, 0.3674), vec3(-0.3903, 0.3707, 0.2245),
    vec3(0.4486, 0.3663, -0.2473), vec3(-0.1849, 0.2802, 0.5423),
    vec3(-0.2266, 0.3871, 0.1957), vec3(0.5293, -0.2538, 0.3534),
    vec3(0.2509, 0.3093, 0.4324), vec3(0.1974, -0.5033, 0.2032),
    vec3(-0.3169, -0.1291, 0.3676), vec3(0.2017, 0.1909, -0.2823),
    vec3(0.0372, 0.0283, 0.1605), vec3(-0.2741, 0.2483, -0.1125),
    vec3(0.3832, -0.4084, 0.3432), vec3(-0.0323, -0.3534, 0.2679),
    vec3(-0.2715, -0.1233, -0.3134), vec3(0.3131, -0.1681, -0.4085),
    vec3(0.3434, 0.1131, -0.0531), vec3(0.0873, 0.2765, -0.3231),
    vec3(-0.1545, 0.1096, 0.1946), vec3(0.0341, 0.4864, 0.0725),
    vec3(-0.2493, -0.1029, 0.4728), vec3(0.1306, 0.1473, 0.1923),
    vec3(-0.4821, 0.1804, 0.0819), vec3(0.2225, -0.2189, -0.0334),
    vec3(0.3032, -0.2711, 0.2109), vec3(-0.0862, 0.3984, 0.1343),
    vec3(0.1065, -0.1179, -0.1887), vec3(0.1013, -0.0661, 0.1433),
    vec3(-0.1429, 0.2147, -0.0213), vec3(0.0982, -0.3131, -0.3243),
    vec3(0.1176, 0.2173, 0.3654), vec3(-0.3714, -0.1123, 0.1032),
    vec3(0.3133, -0.0164, 0.2891), vec3(-0.1834, -0.3464, 0.0932),
    vec3(-0.2666, 0.1752, 0.4235), vec3(0.1942, 0.1678, 0.2685),
    vec3(0.1721, -0.1491, -0.0972), vec3(-0.1281, 0.2373, -0.3819),
    vec3(0.0123, -0.1529, -0.1323), vec3(0.0221, 0.1231, -0.2012),
    vec3(0.0933, -0.1032, 0.0274), vec3(0.2223, 0.0971, 0.1123)
);

vec3 getViewPos(vec2 uv, float depth) {
    vec4 clip = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 view = inverse(ubo.proj) * clip;
    return view.xyz / view.w;
}

vec3 estimateNormal(vec2 uv) {
    float depthC = texture(u_depthMap, uv).r;
    float depthR = texture(u_depthMap, uv + vec2(1.0 / 1280.0, 0)).r;
    float depthU = texture(u_depthMap, uv + vec2(0, 1.0 / 720.0)).r;

    vec3 posC = getViewPos(uv, depthC);
    vec3 posR = getViewPos(uv + vec2(1.0 / 1280.0, 0), depthR);
    vec3 posU = getViewPos(uv + vec2(0, 1.0 / 720.0), depthU);

    vec3 dx = posR - posC;
    vec3 dy = posU - posC;
    return normalize(cross(dx, dy));
}

void main() {
    float depth = texture(u_depthMap, fragUV).r;

    vec3 fragPos = getViewPos(fragUV, depth);
    vec3 normal = estimateNormal(fragUV);

    float occlusion = 0.0;
    for (int i = 0; i < KERNEL_SIZE; ++i) {
        vec3 sampleVec = fragPos + kernel[i] * RADIUS;

        vec4 offset = ubo.proj * vec4(sampleVec, 1.0);
        offset.xyz /= offset.w;
        vec2 sampleUV = offset.xy * 0.5 + 0.5;

        float sampleDepth = texture(u_depthMap, sampleUV).r;
        if (sampleDepth >= 1.0) continue;

        vec3 samplePos = getViewPos(sampleUV, sampleDepth);
        float rangeCheck = smoothstep(0.0, 1.0, RADIUS / abs(fragPos.z - samplePos.z));

        if (samplePos.z >= sampleVec.z + BIAS)
            occlusion += rangeCheck;
    }

    float ao = 1.0 - (occlusion / float(KERNEL_SIZE));
    outColor = vec4(vec3(ao), 1.0);
}
