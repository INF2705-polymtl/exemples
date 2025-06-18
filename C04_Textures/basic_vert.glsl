#version 410


// On peut donner des valeurs par défaut aux variables uniformes. C'est la valeur qu'elles auront si elles ne sont jamais assignées (avec des glUniform*) dans le programme principal.
uniform mat4 model = mat4(1);
uniform mat4 view = mat4(1);
uniform mat4 projection = mat4(1);


// Les attributs de sommets configurés par la classe Mesh.
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoords;


// Les coordonnées de texture (s,t). Comme toute variable de sortie du nuanceur de sommets, elles seront interpolées avant d'être passées au nuanceur de fragments.
out vec2 texCoords;


void main() {
	// Calculer la matrice de transformation.
	mat4 transform = projection * view * model;
	// Calculer et affecter les coordonnées normalisées du sommet.
	vec4 clipPosition = transform * vec4(a_position, 1.0);
	gl_Position = clipPosition;
	// Passer les coordonnées de texture en sortie.
	texCoords = a_texCoords;
}
