#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 position;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 camPos;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D u_albedo;
layout(set = 0, binding = 2) uniform sampler2D u_specular;
layout(set = 0, binding = 3) uniform sampler2D u_normal;


void main() {
		// Screen-space texel size
	vec2 texelSize = 1.0 / vec2(textureSize(u_normal, 0));
	
	// Sample heights
	float hL = texture(u_normal, fragTexCoord - vec2(texelSize.x, 0.0)).r;
	float hR = texture(u_normal, fragTexCoord + vec2(texelSize.x, 0.0)).r;
	float hD = texture(u_normal, fragTexCoord - vec2(0.0, texelSize.y)).r;
	float hU = texture(u_normal, fragTexCoord + vec2(0.0, texelSize.y)).r;
	
	// Compute gradients
	float dx = hR - hL;
	float dy = hU - hD;
	
	// Adjust the normal using bump map (negative because slope goes "down")
	vec3 bumpedNormal = normalize(vec3(-dx, -dy, 1.0));
	float bumpStrength = 2.; // tweak from 0.0 (no bump) to 1.0 (full bump)
	vec3 n = normalize(mix(normal, bumpedNormal, bumpStrength));


	
	vec3 lightPos = vec3(0,500,0);
	vec3 lightDir = normalize(lightPos - position);  

	float ambient = 0.01;
	float diffuse = max(0, dot(lightDir, n));

	vec3 viewDir = normalize(ubo.camPos.rgb - position);
	vec3 reflectDir = reflect(-lightDir, n);  
	float specular = pow(max(dot(viewDir, reflectDir), 0.0), 64);

	vec3 color = (ambient+diffuse )*texture(u_albedo, fragTexCoord).rgb+4.*specular*texture(u_specular, fragTexCoord).rgb;



	
	outColor = vec4(color, 1.0);
}