#version 410


layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoords;


uniform mat4 model = mat4(1);
uniform mat4 view = mat4(1);
uniform mat4 projection = mat4(1);


out Attribs {
	vec3 origPosition;
	vec2 texCoords;
	vec3 normal;
} outAttribs;


void main() {
	// Appliquer les transformations habituelles.
	vec4 worldPosition = model * vec4(a_position, 1.0);
	vec4 viewPosition = view * worldPosition;
	vec4 normPosition = projection * viewPosition;

	// Passer tout au nuanceur de géométrie qui fera les calculs d'éclairage.
	gl_Position = normPosition;
	outAttribs.texCoords = a_texCoords;
	outAttribs.normal = a_normal;
	outAttribs.origPosition = a_position;
}
