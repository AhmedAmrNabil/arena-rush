#version 330 core

in Varyings {
    vec4 color;
    vec2 tex_coord;
} fs_in;

out vec4 frag_color;

uniform sampler2D texture_diffuse;
uniform struct Material {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 emissive;
    float shininess;
} material;

void main() {
    frag_color = material.diffuse;
}
