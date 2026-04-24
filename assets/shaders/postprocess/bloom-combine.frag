#version 330

uniform sampler2D sceneTex;
uniform sampler2D bloomTex;
uniform float intensity;

in vec2 tex_coord;
out vec4 frag_color;

void main() {
    vec3 sceneColor = texture(sceneTex, tex_coord).rgb;
    vec3 bloomColor = texture(bloomTex, tex_coord).rgb;
    frag_color = vec4(sceneColor + bloomColor * intensity, 1.0);
}
