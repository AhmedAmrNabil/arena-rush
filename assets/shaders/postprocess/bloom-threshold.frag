#version 330

uniform sampler2D tex;
uniform float threshold;
uniform float softKnee;

in vec2 tex_coord;
out vec4 frag_color;

void main() {
    vec3 color = texture(tex, tex_coord).rgb;
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float knee = threshold * softKnee;
    float weight = 0.0;
    if (brightness < threshold - knee) discard;
    if (knee > 0.0) {
        weight = smoothstep(threshold - knee, threshold + knee, brightness);
    } else {
        weight = step(threshold, brightness);
    }
    frag_color = vec4(color * weight, 1.0);
}
