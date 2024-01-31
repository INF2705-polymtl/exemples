#version 410


// Les coordonnées de texture interpolées pour le fragment.
in vec2 texCoords;

// L'unité de texture à échantillonner. Dans le programme principal, c'est un glUniformi qu'il faut faire pour l'affecter.
uniform sampler2D tex0;

out vec4 fragColor;


void main() {
	// Échantilloner la texture en utilisant les coordonnées données.
	vec4 texel = texture(tex0, texCoords);
	// Affecter le texel (TEXture ELement) à la couleur de sortie.
	fragColor = texel;
}
