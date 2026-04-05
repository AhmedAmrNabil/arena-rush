#version 330

// The texture holding the scene pixels
uniform sampler2D tex;

// Read "assets/shaders/fullscreen.vert" to know what "tex_coord" holds;
in vec2 tex_coord;

out vec4 frag_color;

// Vignette is a postprocessing effect that darkens the corners of the screen
// to grab the attention of the viewer towards the center of the screen

void main() {
    // To apply vignette, divide the scene color
    // by 1 + the squared length of the 2D pixel location the NDC space
    vec2 ndc = tex_coord * 2.0 - 1.0; // Convert texture coordinates to NDC space
    float vignette = 1.0 / (1.0 + dot(ndc, ndc)); // Calculate vignette factor based on distance from center
    frag_color = texture(tex, tex_coord) * vignette;
}
