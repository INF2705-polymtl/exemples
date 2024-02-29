#version 410


uniform vec4 globalColor;

out vec4 fragColor;


void main() {
	// On n'utilise pas de textures ou de couleurs d'entrée, juste une couleur uniforme.
	fragColor = globalColor;
}
