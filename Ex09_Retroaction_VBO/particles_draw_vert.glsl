#version 410


layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_velocity;
layout(location = 2) in float a_mass;
layout(location = 3) in float a_miscValue;

out VertOut {
	vec3 position;
	vec3 velocity;
	float mass;
	float miscValue;
} outputs;


void main() {
	// Passer les données au nuanceur de géométrie. Tous les calculs seront faits dans celui-ci pour ne pas s'éparpiller.
	outputs.position = a_position;
	outputs.velocity = a_velocity;
	outputs.mass = a_mass;
	outputs.miscValue = a_miscValue;
}
