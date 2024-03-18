#version 410


layout(location = 0) in vec3  a_position;
layout(location = 1) in vec3  a_velocity;
layout(location = 2) in float a_mass;
layout(location = 3) in float a_miscValue;

uniform float deltaTime = 0;
uniform float drag = 0.2;
uniform vec3 forceFieldPosition = vec3(0, 0, 0);
uniform float forceFieldStrength = 0;
uniform float speedMin = 1e-10;
uniform float speedMax = 10;
uniform float speedFactor = 1;

out vec3  position;
out vec3  velocity;
out float mass;
out float miscValue;


void main() {
	// Calculer l'effet du champs de force.
	float fieldDist = length(forceFieldPosition - a_position);
	vec3 forceFieldDir = normalize(forceFieldPosition - a_position);
	if (any(isnan(forceFieldDir)))
		forceFieldDir = vec3(0, 1, 0);
	vec3 forceFieldVector = forceFieldStrength * forceFieldDir;

	// Calculer l'effet de la trainée.
	vec3 dragVector = -drag * a_velocity;

	// Appliquer les forces sur la vitesse de la particule (F=ma baby!).
	vec3 forceVector = forceFieldVector + dragVector;
	velocity = a_velocity + deltaTime * forceVector / a_mass;
	velocity *= speedFactor;

	// Borner la vitesse.
	vec3 velocityDir = normalize(velocity);
	if (any(isnan(velocityDir)))
		velocityDir = forceFieldDir;
	velocity = velocityDir * clamp(length(velocity), speedMin, speedMax);

	// Appliquer la vitesse sur la position.
	position = a_position + velocity * deltaTime;

	// Sortir la masse et la valeur unique telles-quelles (on pourrait faire de quoi avec).
	mass = a_mass;
	miscValue = a_miscValue;
}
