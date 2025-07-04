#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in VertexData{
    vec3 position;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    vec2 texCoord;
} data;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 camPos;
} ubo;


layout(set = 0, binding = 4) uniform UniformMaterialData {
	float brightness;
	float roughness;
	float reflectance;
} materialData;

layout(set = 0, binding = 1) uniform sampler2D u_albedoMap;
layout(set = 0, binding = 2) uniform sampler2D u_specularMap;
layout(set = 0, binding = 3) uniform sampler2D u_normalMap;

vec3 getNormal(){
    if(materialData.roughness >= 0.01){
        vec2 texelSize = 1.0 / vec2(textureSize(u_normalMap, 0));
	
        float strengh = 2.;

	    float hL = texture(u_normalMap, data.texCoord - vec2(texelSize.x, 0.0)).r*strengh;
	    float hR = texture(u_normalMap, data.texCoord + vec2(texelSize.x, 0.0)).r*strengh;
	    float hD = texture(u_normalMap, data.texCoord - vec2(0.0, texelSize.y)).r*strengh;
	    float hU = texture(u_normalMap, data.texCoord + vec2(0.0, texelSize.y)).r*strengh;
	    
	    float dx = hR - hL;
	    float dy = hU - hD;
	    
	    vec3 bumpedNormal = normalize(vec3(-dx, -dy, 1.0));

        mat3 TBN = mat3(normalize(data.tangent),
                        normalize(data.bitangent),
                        normalize(data.normal));

        return normalize(TBN * bumpedNormal);
   }
   
   return data.normal;
}

vec3 getAlbedo(){
    if(materialData.brightness >= 0.01){
        return materialData.brightness*texture(u_albedoMap, data.texCoord).rgb;
    }

    return vec3(1);
}

vec3 getSpecular(){
    if(materialData.reflectance >= 0.01){
        return materialData.reflectance*texture(u_specularMap, data.texCoord).rgb;
    }

    return vec3(0);
}   

void main() {
    vec3 n = getNormal();

    vec3 lightPos = vec3(0.0, 500.0, 0.0);
    vec3 lightDir = normalize(lightPos - data.position);

    float ambient = 0.01;
    float diffuse = max(dot(lightDir, n), 0.0);

    vec3 viewDir = normalize(ubo.camPos.rgb - data.position);
    vec3 reflectDir = reflect(-lightDir, n);
    float specular = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);

    vec3 albedo = getAlbedo();
    vec3 spec = getSpecular();

    vec3 color = (ambient + diffuse) * albedo + 4.0 * specular * spec;

    outColor = vec4(color, 1.0);
}
