#version 410


const float PI = 3.14159265;

in vec2 texCoords;

uniform sampler2D texMain;
uniform vec4 flashingColor = vec4(1, 1, 1, 1);
// Valeur entre 0 et 1 qui change dans le temps.
uniform float flashingValue = 0;

out vec4 fragColor;


void main() {
	// Échantillonner la texture normalement.
	vec4 texColor = texture(texMain, texCoords);
	// Calculer l'inverse des composantes RGB du texel. On garde le alpha original.
	vec4 invColor = vec4(vec3(1, 1, 1) - texColor.rgb, 1);

	// Pour expérimenter, vous pouvez commenter/décommenter les lignes suivantes.

	float val = flashingValue;

	// "Ramp down" parabolique
	//val = 1 - pow(val, 2);
	// Clignotement sinusoïdale.
	val = 0.5 * (sin(flashingValue * 2 * PI) + 1);

	// Interpoler entre la texture et son inverse.
	fragColor = mix(texColor, invColor, val);
	// Interpoler entre la texture et la couleur donnée.
	//fragColor = mix(texColor, flashingColor, val);

	// Vous voulez aller plus loin? Interpolez entre les valeurs HSV plutôt que RGB.
}
