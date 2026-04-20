#version 330 core

in Varyings {
    vec4 color;
    vec2 tex_coord;
} fs_in;

out vec4 frag_color;

uniform vec4 tint;
uniform sampler2D tex;
uniform float alphaThreshold;

void main() {
    frag_color = tint * fs_in.color * pow(texture(tex, fs_in.tex_coord), vec4(2.2));
    if(frag_color.a <= alphaThreshold)
        discard;
}
