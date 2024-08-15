#version 410


uniform sampler2D texMain;


in vec2 texCoords;
in vec4 color;


out vec4 fragColor;


void main() {
	// Appliquer la couleur en entrée multipliée par la texture.
	fragColor = color * texture(texMain, texCoords);
}
