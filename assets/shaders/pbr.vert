#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 tex_coord;
layout(location = 3) in vec3 normal;
layout(location = 4) in vec3 tangent;

out Varyings {
    vec4 color;
    vec2 tex_coord;
    vec3 worldPos;
    vec3 worldNormal;
    mat3 TBN;
} vs_out;

uniform mat4 transform;
uniform mat4 model;

void main() {
    gl_Position = transform * vec4(position, 1.0);

    vs_out.color = color;
    vs_out.tex_coord = tex_coord;
    vs_out.worldPos = vec3(model * vec4(position, 1.0));

    // normal matrix — handles non-uniform scaling correctly
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 N = normalize(normalMatrix * normal);
    vs_out.worldNormal = N;

    // TBN matrix for normal mapping
    vec3 T = normalize(normalMatrix * tangent);
    T = normalize(T - dot(T, N) * N);  // re-orthogonalize
    vec3 B = cross(N, T);
    vs_out.TBN = mat3(T, B, N);
}
