#version 330

uniform sampler2D tex;
uniform float intensity;
uniform float damageFlash;
uniform float exposure;

in vec2 tex_coord;
out vec4 frag_color;

#define BLUR_SAMPLES 8

void main() {
    vec2 center = vec2(0.5);
    vec2 dir = tex_coord - center;

    // Radial Zoom Blur
    vec3 color = vec3(0.0);
    float blurStrength = intensity * 0.2;
    for (int i = 0; i < BLUR_SAMPLES; i++) {
        float t = float(i) / float(BLUR_SAMPLES);
        vec2 offset = dir * blurStrength * t;
        color += texture(tex, tex_coord - offset).rgb;
    }
    color /= float(BLUR_SAMPLES);

    // Chromatic Aberration
    float caStrength = intensity * 0.015;
    vec2 caDir = normalize(dir + vec2(0.001)) * caStrength;
    float r = texture(tex, tex_coord - caDir).r;
    float g = texture(tex, tex_coord).g;
    float b = texture(tex, tex_coord + caDir).b;
    vec3 caColor = vec3(r, g, b);

    color = mix(color, caColor, 0.4);

    // Vignette
    vec2 ndc = tex_coord * 2.0 - 1.0;
    float vignette = 1.0 / (1.0 + dot(ndc, ndc));
    color *= vignette;

    // red vignette at screen edges when player takes a hit
    if (damageFlash > 0.0) {
        float edgeMask = smoothstep(0.2, 1.2, length(ndc));
        vec3 damageColor = vec3(0.7, 0.02, 0.02);
        color = mix(color, damageColor, damageFlash * edgeMask * 0.75);
    }

    color *= exposure;

    // filmic tonemap + gamma in one ALU expression
    vec3 x = max(vec3(0.0), color - 0.004);
    color = (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06);

    frag_color = vec4(color, 1.0);
}
