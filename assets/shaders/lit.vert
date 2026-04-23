#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 tex_coord;
layout(location = 3) in vec3 normal;
layout(location = 4) in vec4 tangent;
layout(location = 5) in ivec4 bone_ids;
layout(location = 6) in vec4 weights;

out Varyings {
    vec4 color;
    vec2 tex_coord;
    vec3 worldPos;
    vec3 worldNormal;
    mat3 TBN;
} vs_out;

#define MAX_BONES 128
uniform mat4 transform;
uniform mat4 model;
layout(std140) uniform Bones {
    mat4 bones[MAX_BONES];
};
uniform bool hasBones;

void main() {
    vec4 skinnedPosition = vec4(0.0);
    vec3 skinnedNormal = vec3(0.0);
    vec3 skinnedTangent = vec3(0.0);
    float totalWeight = 0.0;

    if(hasBones) {
        for(int i = 0; i < 4; i++) {
            if (bone_ids[i] < 0 || bone_ids[i] >= MAX_BONES)
                continue;
            mat4 boneMatrix = bones[bone_ids[i]];
            float weight = weights[i];
            totalWeight += weight;

            skinnedPosition += weight * (boneMatrix * vec4(position, 1.0));
            skinnedNormal += weight * (mat3(boneMatrix) * normal);
            skinnedTangent += weight * (mat3(boneMatrix) * tangent.xyz);
        }
        if (totalWeight <= 0.0) {
            skinnedPosition = vec4(position, 1.0);
            skinnedNormal = normal;
            skinnedTangent = tangent.xyz;
        }
    } else {
        skinnedPosition = vec4(position, 1.0);
        skinnedNormal = normal;
        skinnedTangent = tangent.xyz;
    }

    gl_Position = transform * skinnedPosition;
    vs_out.worldPos = vec3(model * skinnedPosition);
    vs_out.color = color;
    vs_out.tex_coord = tex_coord;

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 N = normalize(normalMatrix * skinnedNormal);
    vs_out.worldNormal = N;

    vec3 T = normalize(normalMatrix * skinnedTangent);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T) * tangent.w;
    vs_out.TBN = mat3(T, B, N);
}
