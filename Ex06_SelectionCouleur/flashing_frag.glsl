#version 410


in vec2 texCoords;

uniform sampler2D texMain;
uniform vec4 flashingColor = vec4(1, 1, 1, 1);
// Valeur entre 0 et 1 qui change dans le temps.
uniform float flashingValue = 0;

out vec4 fragColor;


const float pi = 3.14159265;


// sRGB à HSV (tous entre 0 et 1)
vec3 rgb2hsv(vec3 c) {
	// Pris de https://stackoverflow.com/a/17897228
	vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
	vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
	vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

	float d = q.x - min(q.w, q.y);
	float e = 1.0e-10;
	return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

// HSV à sRGB (tous entre 0 et 1)
vec3 hsv2rgb(vec3 c) {
	// Pris de https://stackoverflow.com/a/17897228
	vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
	return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}


void main() {
	// Échantillonner la texture normalement.
	vec4 texColor = texture(texMain, texCoords);
	// Calculer l'inverse des composantes RGB du texel. On garde le alpha original.
	vec4 invColor = vec4(vec3(1, 1, 1) - texColor.rgb, texColor.a);
	// Convertir les deux couleurs en HSV pour faire une interpolation différente.
	vec3 texColorHsv = rgb2hsv(vec3(texColor));
	vec3 invColorHsv = rgb2hsv(vec3(invColor));

	// Pour expérimenter, vous pouvez commenter/décommenter les lignes suivantes.

	float val = 0;
	
	// "Ramp-up" linéaire
	val = flashingValue;
	// "Ramp down" parabolique
	//val = 1 - pow(flashingValue, 2);
	// Clignotement sinusoïdale.
	//val = 0.5 * sin(flashingValue * 2 * pi) + 1;
	// Clignotement sinusoïdale moins linéaire (plus applati aux pics).
	float wave = sin(flashingValue * 2 * pi);
	val = 0.5 * (pow(abs(wave), 1.0 / 3) * sign(wave) + 1);

	// Interpoler entre la texture et son inverse.
	fragColor = mix(texColor, invColor, val);
	//fragColor = vec4(hsv2rgb(mix(texColorHsv, invColorHsv, val)), texColor.a);
	// Interpoler entre la texture et la couleur donnée.
	//fragColor = mix(texColor, flashingColor, val);
	//fragColor = vec4(hsv2rgb(mix(texColorHsv, rgb2hsv(vec3(flashingColor)), val)), texColor.a);
}
