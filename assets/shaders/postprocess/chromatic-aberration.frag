#version 330

// The texture holding the scene pixels
uniform sampler2D tex;

// Read "assets/shaders/fullscreen.vert" to know what "tex_coord" holds;
in vec2 tex_coord;
out vec4 frag_color;

// How far (in the texture space) is the distance (on the x-axis) between
// the pixels from which the red/green (or green/blue) channels are sampled
#define STRENGTH 0.005

// Chromatic aberration mimics some old cameras where the lens disperses light
// differently based on its wavelength. In this shader, we will implement a
// cheap version of that effect

void main() {
    // red = move by STRENGTH to the left, then sample the red channel
    // green = sample the green channel from the correct pixel
    // blue = move by STRENGTH to the right, then sample the blue channel
    frag_color.r = texture(tex, tex_coord - vec2(STRENGTH, 0.0)).r;
    frag_color.ga = texture(tex, tex_coord).ga;
    frag_color.b = texture(tex, tex_coord + vec2(STRENGTH, 0.0)).b;
}
