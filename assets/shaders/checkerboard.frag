#version 330 core

out vec4 frag_color;

// In this shader, we draw a checkboard where the size of each tile is (size x size).
// The color of the top-left most tile should be "colors[0]" and the 2 tiles adjacent to it
// should have the color "colors[1]".

uniform int size = 32;
uniform vec3 colors[2];

void main(){

    int xTile = int(gl_FragCoord.x) / size;
    int yTile = int(gl_FragCoord.y) / size;

    int tileIndex = (xTile + yTile) % 2;
    frag_color = vec4(colors[tileIndex], 1.0);
}
