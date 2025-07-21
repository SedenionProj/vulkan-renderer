#define MAX_LIGHTS 16

struct Light{
    vec4 color;
	vec4 position;
};
layout (set = 0, binding = 0) uniform SceneData {
    Light lights[MAX_LIGHTS];
    mat4 lightSpace;
    mat4 view;
    mat4 proj;
    vec4 camPos;
    int lightCount;
    vec3 padding;
} u_scene;