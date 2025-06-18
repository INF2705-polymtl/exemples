#version 410


// On prend en entrée la couleur provenant du nuanceur de sommets. Elle est interpolée pour chaque fragment entre les sommets, comme pour toutes les variables d'entrée d'un nuanceur de fragments.
in vec4 color;

out vec4 fragColor;


void main() {
	fragColor = color;
}
