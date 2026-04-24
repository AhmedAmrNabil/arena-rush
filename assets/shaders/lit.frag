#version 330 core

in Varyings {
    vec4 color;
    vec2 tex_coord;
    vec3 worldPos;
    vec3 worldNormal;
    mat3 TBN;
} fs_in;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 frag_bright;

// ── Material ─────────────────────────────────────────────────────────
struct Material {
    // factors (used when no texture)
    vec3 albedo;
    float metallic;
    float roughness;
    float ambientOcclusion;
    vec3 emission;

    // textures + flags
    sampler2D textureAlbedo;
    bool hasTextureAlbedo;

    sampler2D textureMetallic;
    bool hasTextureMetallic;

    sampler2D textureRoughness;
    bool hasTextureRoughness;

    sampler2D textureNormal;
    bool hasTextureNormal;

    sampler2D textureAmbientOcclusion;
    bool hasTextureAmbientOcclusion;

    sampler2D textureEmissive;
    bool hasTextureEmissive;

    sampler2D textureMetalnessRoughness;
    bool hasTextureMetalnessRoughness;
};
uniform Material material;
uniform float alphaThreshold;
uniform vec4 tint;
uniform float bloomThreshold;
uniform float bloomSoftKnee;

// ── Lights ───────────────────────────────────────────────────────────
#define MAX_LIGHTS 24

#define LIGHT_DIRECTIONAL 0
#define LIGHT_POINT       1
#define LIGHT_SPOT        2

struct Light {
    int type;
    vec3 color;
    vec3 position;
    vec3 direction;
    vec3 attenuation;   // (constant, linear, quadratic)
    vec2 spotAngles;    // (inner, outer) in radians
};
uniform Light lights[MAX_LIGHTS];
uniform int numLights;

uniform vec3 cameraPos;

// ── Helpers ───────────────────────────────────────────────────────────
vec3 sampleAlbedo(vec2 uv) {
    vec3 base = material.hasTextureAlbedo ? pow(texture(material.textureAlbedo, uv).rgb, vec3(2.2)) : vec3(1.0);
    return base * material.albedo;
}

float sampleMetallic(vec2 uv) {
    if(material.hasTextureMetalnessRoughness)
        return texture(material.textureMetalnessRoughness, uv).b * material.metallic; // B channel
    if(material.hasTextureMetallic)
        return texture(material.textureMetallic, uv).r * material.metallic;
    return material.metallic;
}

float sampleRoughness(vec2 uv) {
    if(material.hasTextureMetalnessRoughness)
        return texture(material.textureMetalnessRoughness, uv).g * material.roughness; // G channel
    if(material.hasTextureRoughness)
        return texture(material.textureRoughness, uv).r * material.roughness;
    return material.roughness;
}

float sampleAO(vec2 uv) {
    return material.hasTextureAmbientOcclusion ? texture(material.textureAmbientOcclusion, uv).r * material.ambientOcclusion : material.ambientOcclusion;
}

vec3 sampleEmission(vec2 uv) {
    if(material.hasTextureEmissive)
        return pow(texture(material.textureEmissive, uv).rgb, vec3(2.2)) * material.emission;
    return material.emission;
}

vec3 sampleNormal(vec2 uv) {
    if(!material.hasTextureNormal)
        return normalize(fs_in.worldNormal);
    vec3 n = texture(material.textureNormal, uv).rgb * 2.0 - 1.0;
    return normalize(fs_in.TBN * n);
}

float bloomWeight(vec3 color) {
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float knee = bloomThreshold * bloomSoftKnee;
    if(knee > 0.0) {
        return smoothstep(bloomThreshold - knee, bloomThreshold + knee, brightness);
    }
    return step(bloomThreshold, brightness);
}

// ── Blinn-Phong per light ─────────────────────────────────────────────
vec3 calcLight(
    Light light,
    vec3 N,
    vec3 V,
    vec3 albedo,
    float roughness,
    float metallic
) {

    vec3 L;
    float attenuation = 1.0;

    if(light.type == LIGHT_DIRECTIONAL) {
        L = normalize(-light.direction);

    } else {
        vec3 toLight = light.position - fs_in.worldPos;
        float dist = length(toLight);
        L = normalize(toLight);
        if(dot(toLight, N) <= 0.0) {
            return vec3(0.0);
        }

        // quadratic attenuation: 1 / (c + l*d + q*d^2)
        attenuation = 1.0 / (light.attenuation.x + light.attenuation.y * dist + light.attenuation.z * dist * dist);

        if(light.type == LIGHT_SPOT) {
            float theta = dot(L, normalize(-light.direction));
            float innerCos = cos(light.spotAngles.x);
            float outerCos = cos(light.spotAngles.y);
            float epsilon = innerCos - outerCos;
            float spot = clamp((theta - outerCos) / epsilon, 0.0, 1.0);
            attenuation *= spot;
        }
    }

    // diffuse
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * albedo * light.color;

    // specular (Blinn-Phong)
    // roughness → shininess: rough=1 → shininess=2, rough=0 → shininess=256
    float shininess = mix(256.0, 2.0, roughness);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), shininess);
    // metallic surfaces use albedo color for specular
    vec3 specColor = mix(vec3(0.04), albedo, metallic);
    vec3 specular = spec * specColor * light.color;

    return (diffuse + specular) * attenuation;
}

// ── Main ──────────────────────────────────────────────────────────────
void main() {
    vec2 uv = fs_in.tex_coord;
    vec3 albedo = sampleAlbedo(uv) * fs_in.color.rgb * tint.rgb;
    float metallic = sampleMetallic(uv);
    float roughness = sampleRoughness(uv);
    float ao = sampleAO(uv);
    vec3 emission = sampleEmission(uv);
    vec3 N = sampleNormal(uv);
    vec3 V = normalize(cameraPos - fs_in.worldPos);

    // ambient
    vec3 ambient = vec3(0.03) * albedo * ao;

    // accumulate all lights
    vec3 lighting = vec3(0.0);
    int lightCount = min(numLights, MAX_LIGHTS);
    for(int i = 0; i < lightCount; i++) {
        lighting += calcLight(lights[i], N, V, albedo, roughness, metallic);
    }

    vec3 result = ambient + lighting + emission;

    // alpha from albedo texture or tint
    float alpha = material.hasTextureAlbedo ? texture(material.textureAlbedo, uv).a * tint.a * fs_in.color.a : tint.a * fs_in.color.a;

    if(alpha < alphaThreshold)
        discard;

    frag_color = vec4(result, alpha);
    float weight = bloomWeight(result);
    frag_bright = vec4(result * weight, alpha);
}
