#version 410


// Les variables d'entrée du nuanceur de fragments sont les variables de sortie du nuanceur de sommets.
// Comme bien des choses dans les nuanceurs, elles sont interpolées pour le fragment par rapport aux sommets.
in vec4 color;


// Si on met une seule variable de sortie dans le nuanceur de fragments, OpenGL va automatiquement lui affecter un layout(location=0).
// Ceci représente la couleur de sortie, autrement dit la couleur du pixel (un fragment est essentiellement un pixel).
out vec4 fragColor;


void main() {
	// Affecter la couleur interpolée venant des sommets à la couleur du fragment.
	fragColor = color;
}
