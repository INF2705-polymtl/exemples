#version 410


in vec2 texCoords;

uniform sampler2D texMain;

out vec4 fragColor;


void main() {
	// Échantillonner la texture.
	fragColor = texture(texMain, texCoords);
}
