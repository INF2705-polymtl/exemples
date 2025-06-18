#version 410


layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;


out vec4 color;


void main() {
	gl_Position = vec4(a_position, 1.0);
	color = a_color;
}
