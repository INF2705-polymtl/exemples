#version 410


uniform mat4 model = mat4(1);
uniform mat4 view = mat4(1);
uniform mat4 projection = mat4(1);


layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoords;


out VertexOut {
	vec3 origPosition;
	vec2 texCoords;
} outputs;


void main() {
	// On passe les coordonnées d'objets en attributs de sortie.
	outputs.texCoords = a_texCoords;
	outputs.origPosition = a_position;
}
