#version 410


// Les unités de texture à échantillonner. tex2 est la texture du dessus, tex1 le milieu et tex0 en-dessous.
uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;


// Les coordonnées de texture interpolées pour le fragment.
in vec2 texCoords;


out vec4 fragColor;


// Algorithme assez traditionnel pour superposer deux pixels semi-transparents (ayant chacun un alpha pas juste égal à 0 ou 1).
vec4 composite(vec4 over, vec4 under) {
	vec4 result;
	result.a = over.a + under.a * (1 - over.a);
	result.rgb = (over.a * over.rgb + (1 - over.a) * under.a * under.rgb) / result.a;
	return result;
}


void main() {
	// Échantillonner les trois texels (TEXture ELement).
	vec4 texel0 = texture(tex0, texCoords);
	vec4 texel1 = texture(tex1, texCoords);
	vec4 texel2 = texture(tex1, texCoords);

	// Appliquer la composition (texel2 par-dessus texel1 par-dessus texel0).
	fragColor = texel0;
	fragColor = composite(texel1, fragColor);
	fragColor = composite(texel2, fragColor);

	// Si le pixel est très transparent, on s'en débarasse. Ça nous évite d'avoir à penser à l'ordre d'affichage des primitives transparentes. Si vous voulez voir de quel problème je parle, chargez "blank.png" dans texBoxBG (dans la méthode loadTextures). Essayer ensuite le programme en commentant/décommentant le discard suivant.
	if (fragColor.a < 0.01)
		discard;
}
