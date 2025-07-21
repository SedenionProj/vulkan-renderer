#version 450

#include "sceneData.glsl"
#include "common.glsl"

layout(set = 0, binding = 1) uniform sampler2D u_shadowMap;
layout(set = 0, binding = 2) uniform sampler2D u_ssaoMap;

// material data
layout(set = 1, binding = 0) uniform MaterialData {
	float brightness;
	float roughness;
	float reflectance;
} u_material;
layout(set = 1, binding = 1) uniform sampler2D u_albedoMap;
layout(set = 1, binding = 2) uniform sampler2D u_specularMap;
layout(set = 1, binding = 3) uniform sampler2D u_normalMap;

layout(location = 0) out vec4 outColor;

layout(location = 0) in VertexData{
    vec3 fragPos;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    vec2 texCoord;
    vec4 fragPosLightSpace;
} data;

vec3 getNormal(){
    if(u_material.roughness >= 0.01){
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
    if(u_material.brightness >= 0.01){
        return u_material.brightness*invGamma(texture(u_albedoMap, data.texCoord).rgb);
    }

    return vec3(1);
}

vec3 getSpecular(){
    if(u_material.reflectance >= 0.01){
        return u_material.reflectance*texture(u_specularMap, data.texCoord).rgb;
    }

    return vec3(0);
}   

float getShadow(vec4 fragPosLightSpace, vec3 n) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    if (projCoords.z > 1.0)
        return 0.0;

    float currentDepth = projCoords.z;

    vec3 normal = normalize(n);
    vec3 lightDir = normalize(u_scene.lights[0].position.xyz - data.fragPos);
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec2 offset = vec2(x, y) * texelSize;
            float pcfDepth = texture(u_shadowMap, projCoords.xy + offset).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }

    shadow /= 9.0;

    return shadow;
}

float getSSAO(vec2 uv) {
    return texture(u_ssaoMap, uv).r;
}

float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float geometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}



void main() {
    vec3 N = getNormal();
    vec3 V = normalize(u_scene.camPos.xyz - data.fragPos);
    
    vec3 albedo = getAlbedo();
    vec3 metallic = getSpecular();
    float roughness = 0.1;
    
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic.x);
    
    vec3 Lo = vec3(0);
    for (int i = 0; i < u_scene.lightCount; i++) {
        Light light = u_scene.lights[i];

        vec3 L = normalize(light.position.xyz - data.fragPos);
        vec3 H = normalize(L + V);
        float dist = length(light.position.xyz - data.fragPos);
        float attenuation = 1.0 / (dist*dist);
        vec3 radiance = light.color.rgb * attenuation;

        // === Cook-Torrance BRDF ===
        float NDF = distributionGGX(N, H, roughness);
        float G = geometrySmith(N, V, L, roughness);      
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic.x;	  

        float NdotL = max(dot(N, L), 0.0);        

        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }
    
    float shadow = (1.0 - getShadow(data.fragPosLightSpace, N)*0.75);          
    float ssao = getSSAO(gl_FragCoord.xy / vec2(1280.0, 720.0));  
    
    vec3 ambient = vec3(0.03) * albedo * ssao;
    vec3 color = ambient + Lo;
    
    outColor = vec4(color*shadow, 1.0);
}