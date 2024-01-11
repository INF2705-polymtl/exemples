#version 460


// Les variables d'entrée du nuanceur de fragments sont les variables de sortie du nuanceur de sommets.
// Comme bien des choses dans les nuanceurs, elles sont interpolées pour le fragment par rapport aux sommets.
in vec4 color;

// Les variables uniformes d'un programme nuanceur sont disponibles à toutes les étapes (sommets, fragments, etc.).
// Ici, brightness est utilisé par le nuanceur de fragments, mais pas de sommets.
// À l'inverse, on n'a pas besoin de déclarer les matrices de transformation, vue qu'elles ne sont pas utilisées par les fragments. Elles sont quand même là, dans la mémoire du programme, on fait juste les ignorer.
uniform float brightness;

// Si on met une seule variable de sortie dans le nuanceur de fragments, OpenGL va automatiquement lui affecter un layout(location=0).
// Ceci représente la couleur de sortie, autrement dit la couleur du pixel (un fragment est essentiellement un pixel).
out vec4 fragColor;


void main() {
	// Appliquer un facteur multiplicatif sur la couleur (mais pas sur le alpha).
	fragColor = vec4(color.rgb * brightness, color.a);
}
