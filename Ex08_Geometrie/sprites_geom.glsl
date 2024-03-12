#version 410

// On reçoit en entrée les triangles de la primitive et on sort les trois faces du pic extrudé, donc 9 sommets au total.
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;


in VertOut {
	vec3 origPosition;
	vec3 worldPosition;
	vec2 texCoords;
} inputs[];

uniform mat4 model = mat4(1);
uniform mat4 view = mat4(1);
uniform mat4 projection = mat4(1);

out vec2 texCoords;


void main() {
	mat4 transform = projection * view * model;
	vec3 pos = inputs[0].origPosition;

	gl_Position = transform * vec4(pos + vec3(-1, -1, 0), 1);
	texCoords = vec2(0, 0);
	EmitVertex();

	gl_Position = transform * vec4(pos + vec3(1, -1, 0), 1);
	texCoords = vec2(1, 0);
	EmitVertex();

	gl_Position = transform * vec4(pos + vec3(-1, 1, 0), 1);
	texCoords = vec2(0, 1);
	EmitVertex();

	gl_Position = transform * vec4(pos + vec3(1, 1, 0), 1);
	texCoords = vec2(1, 1);
	EmitVertex();
}

