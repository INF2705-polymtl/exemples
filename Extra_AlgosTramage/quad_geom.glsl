#version 410

// On reçoit en entrée les centres des «fragments» et on sort des triangles.
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;


uniform mat4 model = mat4(1);
uniform mat4 view = mat4(1);
uniform mat4 projection = mat4(1);

uniform vec4 activeFragColor = vec4(1, 0.5, 0.5, 1);
uniform vec4 inactiveFragColor = vec4(1, 1, 1, 1);


in VertOut {
	vec3 origPosition;
	bool activeFrag;
} inputs[];


out vec4 color;


void main() {
	mat4 transform = projection * view * model;
	vec3 pos = inputs[0].origPosition;
	vec4 currentColor = inputs[0].activeFrag ? activeFragColor : inactiveFragColor;

	gl_Position = transform * vec4(pos + vec3(-0.5, -0.5, 0), 1);
	color = currentColor;
	EmitVertex();

	gl_Position = transform * vec4(pos + vec3(0.5, -0.5, 0), 1);
	color = currentColor;
	EmitVertex();

	gl_Position = transform * vec4(pos + vec3(-0.5, 0.5, 0), 1);
	color = currentColor;
	EmitVertex();

	gl_Position = transform * vec4(pos + vec3(0.5, 0.5, 0), 1);
	color = currentColor;
	EmitVertex();
	
}

