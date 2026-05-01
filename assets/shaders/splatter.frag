#version 330 core

in Varyings {
    vec2 uv;
} fs_in;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 frag_bright;

uniform vec4 tint;

float hash21(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453123);
}

float valueNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash21(i + vec2(0.0,0.0)), hash21(i + vec2(1.0,0.0)), u.x),
               mix(hash21(i + vec2(0.0,1.0)), hash21(i + vec2(1.0,1.0)), u.x), u.y);
}

// Fractal Brownian Motion for highly organic, warping liquid-like shapes
float fbm(vec2 p) {
    float v = 0.0;
    float a = 0.5;
    mat2 rot = mat2(0.866, -0.5, 0.5, 0.866);
    for(int i = 0; i < 4; i++) {
        v += a * valueNoise(p);
        p = rot * p * 2.0;
        a *= 0.5;
    }
    return v;
}

float splatterMask(vec2 uv) {
    vec2 p = uv * 2.0 - 1.0;
    float r = length(p);
    float a = atan(p.y, p.x);
    
    // Distort space using FBM so the blood isn't a perfect circle
    vec2 warpedP = p + fbm(p * 5.0) * 0.5;
    float warpedR = length(warpedP);
    
    // 1. Central chaotic blob (warped by angle and radius)
    float splat = smoothstep(0.5, 0.1, warpedR + fbm(vec2(a * 4.0, r)) * 0.4);
    
    // 2. Secondary outer droplets (scattered using high-frequency noise)
    float dropsNoise = valueNoise(p * 30.0);
    float drops = smoothstep(0.7, 0.9, dropsNoise) * smoothstep(1.0, 0.2, r);
    
    // 3. Directional explosive streaks
    float streaks = smoothstep(0.6, 1.0, valueNoise(vec2(a * 15.0, r * 2.0))) * smoothstep(0.9, 0.1, r);
    
    return clamp(splat + drops + streaks, 0.0, 1.0);
}

void main() {
    float maskBase = splatterMask(fs_in.uv);
    
    // As it fades out, the erosion threshold goes up
    float erosion = 1.0 - tint.a;
    
    // Multiply by a noise map so it dries and cracks unevenly over time
    float dryNoise = fbm(fs_in.uv * 10.0) + 0.3;
    float alpha = smoothstep(erosion, erosion + 0.1, maskBase * dryNoise);

    if (alpha <= 0.02) discard;

    // Shadertoy trick: darken the blood slightly as it dries out
    vec3 finalColor = mix(tint.rgb * 0.4, tint.rgb, tint.a);
    vec4 color = vec4(finalColor, alpha);

    frag_color = color;
    frag_bright = vec4(0.0, 0.0, 0.0, color.a);
}