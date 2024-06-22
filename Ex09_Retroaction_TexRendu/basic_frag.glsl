#version 410


uniform sampler2D texMain;


in vec2 texCoords;


out vec4 fragColor;


void main() {
	// Échantillonner la texture.
	fragColor = texture(texMain, texCoords);
}
