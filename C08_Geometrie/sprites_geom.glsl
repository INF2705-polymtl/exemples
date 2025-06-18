#version 410

// On reçoit en entrée les centres de chaque lutin et sort le quad en triangle_strip.
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;


uniform mat4 model = mat4(1);
uniform mat4 view = mat4(1);
uniform mat4 projection = mat4(1);


in VertexOut {
	vec3 origPosition;
	vec3 worldPosition;
	vec2 texCoords;
} inputs[];


out vec2 texCoords;


void main() {
	mat4 transform = projection * view * model;
	vec3 pos = inputs[0].origPosition;

	// On a en entrée un seul point qui est le centre du lutin. On génère ensuite les triangles qui forment un quad sur lequel sera appliqué la texture du sprite.

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

