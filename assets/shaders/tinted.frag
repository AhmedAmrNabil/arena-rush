#version 330 core

in Varyings {
    vec4 color;
} fs_in;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 frag_bright;

uniform vec4 tint;
uniform float bloomThreshold;
uniform float bloomSoftKnee;

float bloomWeight(vec3 color) {
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float knee = bloomThreshold * bloomSoftKnee;
    if (knee > 0.0) {
        return smoothstep(bloomThreshold - knee, bloomThreshold + knee, brightness);
    }
    return step(bloomThreshold, brightness);
}

void main() {
    vec4 color = tint * fs_in.color;
    frag_color = color;
    float weight = bloomWeight(color.rgb);
    frag_bright = vec4(color.rgb * weight, color.a);
}
