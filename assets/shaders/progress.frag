#version 330 core

in Varyings {
    vec4 color;
    vec2 tex_coord;
} fs_in;

out vec4 frag_color;

uniform vec4 tint;
uniform sampler2D tex;
uniform float progress;
uniform vec2 uvScale; // defaults to (1.0, 1.0) if not scaling

void main() {
    if (fs_in.tex_coord.x * uvScale.x > progress) {
        discard;
    }
    
    vec4 texColor = texture(tex, fs_in.tex_coord);
    frag_color = tint * fs_in.color * texColor;
}
