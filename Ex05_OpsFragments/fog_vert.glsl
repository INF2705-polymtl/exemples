#version 410


layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoords;

uniform mat4 model = mat4(1);
uniform mat4 view = mat4(1);
uniform mat4 projection = mat4(1);

out vec2 texCoords;
out float distanceToCamera;


void main() {
	vec4 worldPosition = model * vec4(a_position, 1.0);
	vec4 viewPosition = view * worldPosition;
	vec4 normPosition = projection * viewPosition;

	gl_Position = normPosition;
	texCoords = a_texCoords;
	distanceToCamera = -viewPosition.z;
}
