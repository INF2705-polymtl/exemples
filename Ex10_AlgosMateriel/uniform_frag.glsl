#version 410


uniform vec4 globalColor = vec4(1, 1, 1, 1);

out vec4 fragColor;


void main() {
	// Appliquer la couleur globale.
	fragColor = globalColor;
}
