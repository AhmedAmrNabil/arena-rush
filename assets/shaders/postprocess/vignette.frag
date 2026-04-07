#version 330

// The texture holding the scene pixels
uniform sampler2D tex;

// Read "assets/shaders/fullscreen.vert" to know what "tex_coord" holds;
in vec2 tex_coord;

out vec4 frag_color;

// Vignette is a postprocessing effect that darkens the corners of the screen
// to grab the attention of the viewer towards the center of the screen

void main(){
    vec2 ndc = tex_coord * 2.0 - 1.0;
    float sq = dot(ndc, ndc);

    vec4 color = texture(tex, tex_coord);
    color.rgb /= (1.0 + sq);

    frag_color = color;
}
