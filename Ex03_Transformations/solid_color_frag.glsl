#version 410


// On ignore la variable d'entrée qui provient du nuanceur de sommets et on utilise plutôt la variable uniforme qui définit une couleur solide pour la primitive.
uniform vec4 globalColor;

out vec4 fragColor;


void main() {
	fragColor = globalColor;
}
