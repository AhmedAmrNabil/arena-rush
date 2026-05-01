#version 330

uniform sampler2D tex;
uniform vec2 direction;
uniform float radius;

in vec2 tex_coord;
out vec4 frag_color;
uniform float weight[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 texel = 1.0 / vec2(textureSize(tex, 0));
    vec2 offset = direction * texel * radius;

    vec3 color = texture(tex, tex_coord).rgb * weight[0];
    for (int i = 1; i < 5; ++i) {
        color += texture(tex, tex_coord + offset * float(i)).rgb * weight[i];
        color += texture(tex, tex_coord - offset * float(i)).rgb * weight[i];
    }

    frag_color = vec4(color, 1.0);
}
