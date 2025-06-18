#version 410


in vec2 texCoords;

uniform sampler2D texMain;

out vec4 fragColor;


void main() {
	// Échantillonnage habituel de la texture.
	fragColor = texture(texMain, texCoords);
}
