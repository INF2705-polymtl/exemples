#version 410


in vec2 texCoords;

uniform sampler2D texDiffuse;

out vec4 fragColor;


void main() {
	// Échantillonnage habituel de la texture.
	fragColor = texture(texDiffuse, texCoords);
}
