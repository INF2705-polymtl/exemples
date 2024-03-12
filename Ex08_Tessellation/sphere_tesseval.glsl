#version 410

layout(triangles) in;


in TessCtrlOut {
	vec3 origPosition;
	vec2 texCoords;
} inputs[];

uniform mat4 model = mat4(1);
uniform mat4 view = mat4(1);
uniform mat4 projection = mat4(1);

out TessEvalOut {
	vec3 origPosition;
	vec2 texCoords;
} outputs;


vec4 weightedMix(vec4 v0, vec4 v1, vec4 v2) {
	// L'inteporlation qu'on fait une moyenne pondérée où les coordonnées barycentriques sont les poids des valeurs à chaque sommet.
	return gl_TessCoord[0] * v0 + gl_TessCoord[1] * v1 + gl_TessCoord[2] * v2;
}

vec3 weightedMix(vec3 v0, vec3 v1, vec3 v2) {
	return vec3(weightedMix(vec4(v0, 0), vec4(v1, 0), vec4(v2, 0)));
}

vec2 weightedMix(vec2 v0, vec2 v1, vec2 v2) {
	return vec2(weightedMix(vec4(v0, 0, 0), vec4(v1, 0, 0), vec4(v2, 0, 0)));
}

float weightedMix(float v0, float v1, float v2) {
	return weightedMix(vec4(v0), vec4(v1), vec4(v2))[0];
}


void main() {
	// Calculer la moyenne pondérée des positions (gl_Position et coords d'objets) et des coords de textures.
	gl_Position = weightedMix(
		gl_in[0].gl_Position,
		gl_in[1].gl_Position,
		gl_in[2].gl_Position
	);
	outputs.origPosition = weightedMix(
		inputs[0].origPosition,
		inputs[1].origPosition,
		inputs[2].origPosition
	);
	outputs.texCoords = weightedMix(
		inputs[0].texCoords,
		inputs[1].texCoords,
		inputs[2].texCoords
	);

	// Calculer le rayon moyen des trois sommets de la primitive. On veut créer une sphère, mais on ne connait pas son rayon. Assumer rayon=1 est une mauvaise idée. On pourrait le mettre en variable uniforme, mais c'est mieux de tout simplement de calculer le rayon local à la primitive.
	float avgRadius = weightedMix(
		length(inputs[0].origPosition),
		length(inputs[1].origPosition),
		length(inputs[2].origPosition)
	);
	// Repositionner le sommet en le projetant sur son vecteur en utilisant le rayon moyen comme longueur.
	outputs.origPosition = avgRadius * normalize(outputs.origPosition);

}
