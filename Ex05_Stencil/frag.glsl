#version 410


in vec2 texCoords;

uniform sampler2D texMain;
uniform bool shouldDiscard = false;

out vec4 fragColor;


void main() {
	fragColor = texture(texMain, texCoords);

	// Abandonner explicitement les fragments qui sont transparents. On fait ça quand on dessine le masque de la lunette (le cercle blanc) pour seulement mettre des 1 dans le stencil pour les fragments dans le cercle. En effet, la valeur donné par glStencilFunc est écrite dans le stencil POUR TOUS LES FRAGMENTS DE LA PRIMITIVE, peu importe leur valeur de couleur (même si c'est RGBA tout à 0). On aurait donc un carré dans dans le stencil, vu que c'est la primitive utilisé. On va donc plutôt discard les fragments qui ne sont pas dans le cercle, c-à-d ceux qui sont transparents.
	if (shouldDiscard && fragColor.a < 0.01)
		discard;
}
