// Dans le modèle de Gouraud, les calculs sont effectués une fois par sommet, donc dans le nuanceur de sommets. Les résultats (couleurs) sont passés en sortie des sommets et donc interpolés aux fragments.

#version 410


in vec4 color;

out vec4 fragColor;


void main() {
	fragColor = color;
}
