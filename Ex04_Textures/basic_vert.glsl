#version 410


layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoords;

uniform mat4 model = mat4(1);
uniform mat4 view = mat4(1);
uniform mat4 projection = mat4(1);
uniform vec4 globalColor = vec4(1.0, 1.0, 1.0, 1.0);

out vec2 texCoords;
out vec4 color;


void main() {
	mat4 transform = projection * view * model;
	vec4 position = transform * vec4(a_position, 1.0);
	gl_Position = position;
	color = globalColor;
	texCoords = a_texCoords;
}
