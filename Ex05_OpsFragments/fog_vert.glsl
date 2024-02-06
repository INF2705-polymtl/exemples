#version 410


layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoords;

uniform mat4 model = mat4(1);
uniform mat4 view = mat4(1);
uniform mat4 projection = mat4(1);

out vec2 texCoords;
// Ajouter une variable de sortie passé au nuanceur de fragments pour avoir la distance entre la caméra et le sommet. Cette distance est interpolées pour les fragments comme toute variable de sortie.
out float distanceToCamera;


void main() {
	// Les coordonnées d'objets, de scène (ou univers), de visualisation et normalisées.
	vec4 worldPosition = model * vec4(a_position, 1.0);
	vec4 viewPosition = view * worldPosition;
	vec4 normPosition = projection * viewPosition;

	// Faire les trucs habituels.
	gl_Position = normPosition;
	texCoords = a_texCoords;

	// Calculer la distance du sommet à la caméra en utilisant les coodonnées de visualisation.
	distanceToCamera = -viewPosition.z;
	// Utiliser le z des coordonnées de visualisation nous donne un plan face à la caméra. Pour un effet de brouillard un peu "old school", c'est probablement la bonne chose à faire. On peut aussi calculer la distance en obtenant la norme du vecteur, ce qui nous donne plus un dôme.
	//distanceToCamera = length(viewPosition.xyz);
}
