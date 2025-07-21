#version 450

#include "sceneData.glsl"

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

layout(push_constant) uniform push {
	mat4 model;
} transform;

void main() {
    mat3 normalMatrix = transpose(inverse(mat3(transform.model)));

    data.fragPos = vec3(transform.model * vec4(inPosition, 1.0));
    data.normal = normalize(normalMatrix * inNormal);
    data.tangent = normalize(normalMatrix * inTangent);
    data.bitangent = normalize(normalMatrix * inBitangent);
    data.fragPosLightSpace = u_scene.lightSpace * vec4(data.fragPos, 1.0);
    data.texCoord = inTexCoord;

    gl_Position = u_scene.proj * u_scene.view * vec4(data.fragPos, 1.0);
}