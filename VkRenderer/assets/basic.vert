#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out VertexData{
    vec3 fragPos;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    vec2 texCoord;
    vec4 fragPosLightSpace;
} data;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 lightSpace;
    vec4 camPos;
} ubo;

void main() {
    mat3 normalMatrix = transpose(inverse(mat3(ubo.model)));

    data.fragPos = vec3(ubo.model * vec4(inPosition, 1.0));
    data.normal = normalize(normalMatrix * inNormal);
    data.tangent = normalize(normalMatrix * inTangent);
    data.bitangent = normalize(normalMatrix * inBitangent);
    data.fragPosLightSpace = ubo.lightSpace * vec4(data.fragPos, 1.0);
    data.texCoord = inTexCoord;

    gl_Position = ubo.proj * ubo.view * vec4(data.fragPos, 1.0);
}