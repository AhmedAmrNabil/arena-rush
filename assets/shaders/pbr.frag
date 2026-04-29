#version 330 core

#define PI 3.14159265358979323846

in Varyings {
    vec4 color;
    vec2 tex_coord;
    vec3 worldPos;
    vec3 worldNormal;
    mat3 TBN;
} fs_in;

out vec4 frag_color;

// Material
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
    bool hasMetalnessRoughness;
};
uniform Material material;
uniform float alphaThreshold;
uniform vec4 tint;

// Lights
#define MAX_LIGHTS 8

#define LIGHT_DIRECTIONAL 0
#define LIGHT_POINT       1
#define LIGHT_SPOT        2

struct Light {
    int type;
    vec3 color;
    vec3 position;
    vec3 direction;
    vec3 attenuation;  // (constant, linear, quadratic)
    vec2 spotAngles;   // (inner, outer) in radians
};
uniform Light lights[MAX_LIGHTS];
uniform int numLights;

uniform vec3 cameraPos;

// Texture helpers

vec3 sampleAlbedo(vec2 uv) {
    vec3 base = material.hasTextureAlbedo ? pow(texture(material.textureAlbedo, uv).rgb, vec3(2.2)) : vec3(1.0);
    return base * material.albedo;
}

float sampleMetallic(vec2 uv) {
    if(material.hasMetalnessRoughness)
        return texture(material.textureMetalnessRoughness, uv).b * material.metallic; // B channel
    if(material.hasTextureMetallic)
        return texture(material.textureMetallic, uv).r * material.metallic;
    return material.metallic;
}

float sampleRoughness(vec2 uv) {
    float r;
    if(material.hasMetalnessRoughness)
        r = texture(material.textureMetalnessRoughness, uv).g * material.roughness; // G channel
    else if(material.hasTextureRoughness)
        r = texture(material.textureRoughness, uv).r * material.roughness;
    else
        r = material.roughness;
    return max(r, 0.05); // avoid perfectly smooth surfaces which cause singularities in the BRDF
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

// PBR - Cook-Torrance BRDF

// Normal Distribution Function — Trowbridge-Reitz GGX
//   D(h) = alpha^2 / (PI * ((N·H)^2 * (alpha^2 - 1) + 1)^2)
float distributionGGX(float NdotH, float roughness) {
    float a = roughness * roughness;   // remap roughness
    float a2 = a * a;
    float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

// Geometry sub-term — Schlick-GGX (direct lighting k = (r+1)^2/8)
float geometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

// Smith's method: accounts for both view and light shadowing/masking
float geometrySmith(float NdotV, float NdotL, float roughness) {
    return geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);
}

// Fresnel — Schlick approximation
//   F(v,h) = F0 + (1 - F0)(1 - V·H)^5
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Per-light PBR radiance contribution
vec3 calcLight(
    Light light,
    vec3 N,
    vec3 V,
    vec3 albedo,
    float metallic,
    float roughness,
    vec3 F0
) {
    vec3 L;
    float attenuation = 1.0;

    if(light.type == LIGHT_DIRECTIONAL) {
        L = normalize(-light.direction);

    } else {
        vec3 toLight = light.position - fs_in.worldPos;
        float dist = length(toLight);
        L = normalize(toLight);

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

    float NdotL = max(dot(N, L), 0.0);
    if(NdotL <= 0.0)
        return vec3(0.0);   // light below surface — skip

    float NdotV = max(dot(N, V), 0.0);
    vec3 H = normalize(V + L);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    // Cook-Torrance specular BRDF
    float D = distributionGGX(NdotH, roughness);
    float G = geometrySmith(NdotV, NdotL, roughness);
    vec3 F = fresnelSchlick(HdotV, F0);

    // Denominator guard avoids division by zero at grazing angles
    vec3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 0.0001);

    // Energy conservation:
    //   kS = Fresnel reflectance (already in F)
    //   kD = remaining diffuse fraction, zero for pure metals
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

    vec3 radiance = light.color * attenuation;

    // Lambert diffuse + specular, scaled by NdotL and radiance
    return (kD * albedo / PI + specular) * radiance * NdotL;
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

    // Base reflectance at normal incidence:
    //   dielectrics -> 0.04 (common non-metal approximation)
    //   metals      -> tinted by albedo
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Accumulate direct lighting from all lights
    vec3 Lo = vec3(0.0);
    int lightCount = min(numLights, MAX_LIGHTS);
    for(int i = 0; i < lightCount; i++) {
        Lo += calcLight(lights[i], N, V, albedo, metallic, roughness, F0);
    }

    // Ambient simple image-based approximation via a flat ambient term.
    // should be with IBL but not implemented yet
    vec3 ambient = vec3(0.03) * albedo * ao;

    vec3 result = ambient + Lo + emission;

    result = result / (result + vec3(1.0));  // simple Reinhard tone-mapping
    result = pow(result, vec3(1.0 / 2.2));

    // Alpha: sample from texture if present, otherwise use tint/vertex alpha
    float alpha = material.hasTextureAlbedo ? texture(material.textureAlbedo, uv).a * tint.a * fs_in.color.a : tint.a * fs_in.color.a;

    if(alpha < alphaThreshold)
        discard;

    frag_color = vec4(result, alpha);
}
