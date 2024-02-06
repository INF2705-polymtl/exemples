#version 410


layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoords;

uniform mat4 model = mat4(1);
uniform mat4 view = mat4(1);
uniform mat4 projection = mat4(1);

out vec2 texCoords;


void main() {
	mat4 transform = projection * view * model;
	vec4 position = transform * vec4(a_position, 1.0);
	gl_Position = position;
	texCoords = a_texCoords;
}
