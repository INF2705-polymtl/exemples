#version 410


in vec2 texCoords;
// La distance à la caméra calculé par le sommet et interpolée par le trammage.
in float distanceToCamera;

uniform sampler2D texMain;
// La couleur du brouillard (probablement égale à la couleur du glClearColor).
uniform vec3 fogColor;
// Le plan proche du brouillard (où le brouillard commence).
uniform float fogNear;
// Le plan loin du brouillard (où le brouillard est maximal).
uniform float fogFar;

out vec4 fragColor;


void main() {
	// Échantiollonnage de texture habituel.
	fragColor = texture(texMain, texCoords);

	// Calculer la valeur d'intensité du brouillard (0 = pas de brouillard, 1 = juste du brouillard).
	float d = clamp(distanceToCamera, fogNear, fogFar);
	d = (d - fogNear) / (fogFar - fogNear);

	// Interpolation linéaire (mix est comme lerp) de la couleur de texture et de la couleur de brouillard selon la valeur calculée ci-dessus.
	fragColor.rgb = mix(fragColor.rgb, fogColor, d);
}
