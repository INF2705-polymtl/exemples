#version 410


layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoords;
layout(location = 3) in vec4 a_color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec4 color;


void main() {
	mat4 transform = projection * view * model;
	vec4 position = transform * vec4(a_position, 1.0);
	gl_Position = position;
	color = a_color;
}
