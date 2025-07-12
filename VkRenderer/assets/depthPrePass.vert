#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 lightSpace;
    vec4 camPos;
} ubo;

void main(){
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0); 
}