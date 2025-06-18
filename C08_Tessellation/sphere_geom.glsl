#version 410

// On reçoit en entrée les triangles de la primitive et on sort des triangles.
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;


uniform mat4 model = mat4(1);
uniform mat4 view = mat4(1);
uniform mat4 projection = mat4(1);


in TessEvalOut {
	vec3 origPosition;
	vec2 texCoords;
} inputs[];


out vec2 texCoords;


void main() {
	// Calculer la matrice de transformation habituelle.
	mat4 transformMat = projection * view * model;
	// Pour chacun des sommets en entrée, l'émettre en appliquant les transformations aux positions qui sont en coordonnées d'objet. Les coords de textures sont passées telles-quelles à chaque sommet.
	for (int i = 0; i < gl_in.length(); i++) {
		gl_Position = transformMat * vec4(inputs[i].origPosition, 1);
		texCoords = inputs[i].texCoords;
		EmitVertex();
	}
}

