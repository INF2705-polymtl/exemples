#version 410


uniform mat4 model = mat4(1);
uniform mat4 view = mat4(1);
uniform mat4 projection = mat4(1);

uniform vec4 clipPlane = vec4(0);


layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoords;


out vec2 texCoords;


void main() {
	// Appliquer les transformations habituelles.
	vec4 worldPosition = model * vec4(a_position, 1.0);
	vec4 viewPosition = view * worldPosition;
	vec4 clipPosition = projection * viewPosition;

	// Calculer la distance du sommet au plan de coupe en utilisant les coordonnées de scène.
	// Les fragments avec des distances négatives seront éliminés.
	gl_ClipDistance[0] = dot(clipPlane, worldPosition);

	gl_Position = clipPosition;
	texCoords = a_texCoords;
}
