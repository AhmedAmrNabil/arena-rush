#version 330 core

layout(location = 0) in vec3 position;
layout(location = 2) in vec2 tex_coord;

out Varyings {
    vec2 uv;
} vs_out;

uniform mat4 transform;

void main() {
    gl_Position = transform * vec4(position, 1.0);
    vs_out.uv = tex_coord;
}
