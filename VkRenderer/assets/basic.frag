#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in VertexData{
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

layout(set = 0, binding = 4) uniform UniformMaterialData {
	float brightness;
	float roughness;
	float reflectance;
} materialData;

layout(set = 0, binding = 1) uniform sampler2D u_albedoMap;
layout(set = 0, binding = 2) uniform sampler2D u_specularMap;
layout(set = 0, binding = 3) uniform sampler2D u_normalMap;
layout(set = 0, binding = 5) uniform sampler2D u_shadowMap;

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

float getShadow(vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    float currentDepth = projCoords.z;

    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0)
        return 0.0;


    // Add bias to avoid shadow acne
    float bias = 0.005;

    // PCF (3x3 kernel)
    float shadow = 0.0;
    float texelSize = 1.0 / textureSize(u_shadowMap, 0).x;

    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec2 offset = vec2(x, y) * texelSize;
            float closestDepth = texture(u_shadowMap, projCoords.xy + offset).r;
            shadow += currentDepth - bias > closestDepth ? 1.0 : 0.0;
        }
    }

    shadow /= 9.0;
    return shadow;
}


void main() {
    vec3 n = getNormal();

    vec3 lightPos = vec3(0.0, 10.0, 0.0);
    vec3 lightDir = normalize(lightPos - data.fragPos);

    float ambient = 0.1;
    float diffuse = max(dot(lightDir, n), 0.0);

    vec3 viewDir = normalize(ubo.camPos.rgb - data.fragPos);
    vec3 reflectDir = reflect(-lightDir, n);
    float specular = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);

    vec3 albedo = getAlbedo();
    vec3 spec = getSpecular();

    float shadow = getShadow(data.fragPosLightSpace);

    vec3 color = (ambient + diffuse ) * albedo + 4.0 * specular * spec;

    outColor = vec4(vec3(color)*(1.0 - shadow*0.75), 1.0);
}