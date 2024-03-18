#version 410

layout(points) in;
layout(triangle_strip, max_vertices = 128) out;


in VertOut {
	vec3 position;
	vec3 velocity;
	float mass;
	float miscValue;
} inputs[];

uniform mat4 model = mat4(1);
uniform mat4 view = mat4(1);
uniform mat4 projection = mat4(1);
uniform int numFaces = 8;
uniform float trailLengthMax = 10;
uniform float speedMin = 1e-10;
uniform float speedMax = 10;

out vec4 color;


vec3 rotate(vec3 v, float angle);


void main() {
	mat4 transformMat = projection * view * model;

	float radius = inputs[0].mass * 0.05;
	vec3 center = inputs[0].position;
	vec3 direction = normalize(inputs[0].velocity);
	float speedValue = smoothstep(speedMin, speedMax, length(inputs[0].velocity));
	float massValue = smoothstep(0.5, 1.0, inputs[0].mass);
	float trailLength = mix(1, trailLengthMax, speedValue);
	vec4 particleColor = mix(vec4(1, 0, 0, 1), vec4(1, 1, 0, 1), speedValue);
	
	vec3 radiusVec = direction * radius;
	for (int i = 0; i < numFaces; i++) {
		gl_Position = transformMat * vec4(center, 1);
		color = particleColor;
		EmitVertex();

		gl_Position = transformMat * vec4(center + radiusVec, 1);
		color = particleColor;
		EmitVertex();

		radiusVec = rotate(radiusVec, 360.0 / numFaces);
		gl_Position = transformMat * vec4(center + radiusVec, 1);
		color = particleColor;
		EmitVertex();

		EndPrimitive();
	}

	radiusVec = direction;
	radiusVec = rotate(radiusVec, -90);
	gl_Position = transformMat * vec4(center + radiusVec * radius, 1);
	color = particleColor;
	EmitVertex();
	radiusVec = rotate(radiusVec, 180);
	gl_Position = transformMat * vec4(center + radiusVec * radius, 1);
	color = particleColor;
	EmitVertex();
	radiusVec = rotate(radiusVec, 90) * trailLength;
	gl_Position = transformMat * vec4(center + radiusVec * radius, 1);
	color = particleColor;
	EmitVertex();


}


vec3 rotate(vec3 v, float angle) {
	float rads = radians(angle);
	mat4 rotation = mat4(
		cos(rads), sin(rads), 0.0, 0.0,
		-sin(rads), cos(rads), 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0
	);
	return vec3(rotation * vec4(v, 1));
}
