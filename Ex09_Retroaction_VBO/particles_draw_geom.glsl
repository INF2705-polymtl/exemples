#version 410

layout(points) in;
layout(triangle_strip, max_vertices = 128) out;


uniform mat4 model = mat4(1);
uniform mat4 view = mat4(1);
uniform mat4 projection = mat4(1);

uniform int numFaces = 8; // Nombre de faces du polygone généré.
uniform float trailLengthMax = 10;
uniform float speedMin = 1e-10;
uniform float speedMax = 10;


in VertexOut {
	vec3 position;
	vec3 velocity;
	float mass;
	float miscValue;
} inputs[];


out vec4 color;


vec3 rotate(vec3 v, float angle);


void main() {
	// La matrice de tranformation habituelle.
	mat4 transformMat = projection * view * model;

	// Le rayon du polygone dépend de sa masse de la particule.
	float radius = inputs[0].mass * 0.05;
	// La position de la particule est le centre du polygone.
	vec3 center = inputs[0].position;
	// Direction de la particule (vec unitaire de sa vitesse).
	vec3 direction = normalize(inputs[0].velocity);
	// La valeur normalisée de la vitesse (entre 0 et 1 selon les vitesses min et max).
	float speedValue = smoothstep(speedMin, speedMax, length(inputs[0].velocity));
	// Longueur de la trainée dépend de la vitesse.
	float trailLength = mix(1, trailLengthMax, speedValue);
	// Couleur de la particule dépend de la vitesse (lent = rouge, vite = jaune).
	vec4 particleColor = mix(vec4(1, 0, 0, 1), vec4(1, 1, 0, 1), speedValue);

	// On veut dessiner un genre de triangle fan, mais on n'a pas cette primitive donc on dessine séparément chaque triangle du triangle fan.

	// Le vecteur de rayon de la première face.
	vec3 radiusVec = direction * radius;
	// L'angle entre chaque sommet du polygone.
	float angle = 360.0 / numFaces;
	// Pour chaque face du polygone.
	for (int i = 0; i < numFaces; i++) {
		// 1er sommet : le centre du polygone.
		gl_Position = transformMat * vec4(center, 1);
		color = particleColor;
		EmitVertex();

		// 2e sommet : le premier coin de la face courante.
		gl_Position = transformMat * vec4(center + radiusVec, 1);
		color = particleColor;
		EmitVertex();

		// 3e sommet : le deuxième coin de la face courante, donc avec une rotation anti-horaire.
		radiusVec = rotate(radiusVec, angle);
		gl_Position = transformMat * vec4(center + radiusVec, 1);
		color = particleColor;
		EmitVertex();

		// Dessiner le triangle séparément (sinon ça sera un triangle strip).
		EndPrimitive();
	}

	// Enfin, on ajoute un triangle pour la trainée derrière le polygone (donc qui pointe à l'opposée de sa direction).

	// 1er sommet : le coin "tribord" (à droite de sa direction) du polygone.
	radiusVec = direction;
	radiusVec = rotate(radiusVec, -90);
	gl_Position = transformMat * vec4(center + radiusVec * radius, 1);
	color = particleColor;
	EmitVertex();
	// 2e sommet : le coin "babord"
	radiusVec = rotate(radiusVec, 180);
	gl_Position = transformMat * vec4(center + radiusVec * radius, 1);
	color = particleColor;
	EmitVertex();
	// 3e sommet : le bout de la queue. trailLength est en fait un facteur appliqué au rayon du polygone.
	radiusVec = rotate(radiusVec, 90) * trailLength;
	gl_Position = transformMat * vec4(center + radiusVec * radius, 1);
	color = particleColor;
	EmitVertex();
}


vec3 rotate(vec3 v, float angle) {
	float rads = radians(angle);
	// En GLSL, on n'a pas les rotate/scale/translate comme avec GLM, donc on bâtit notre propre matrice de rotation pour tourner le vecteur v autour de l'axe des z (on est en 2D dans notre exemple pour garder ça simple).
	// Attention, les matrices GLSL sont en colonne-major, comme avec GLM.
	mat3 rotation = mat3(
		 cos(rads), sin(rads), 0.0,
		-sin(rads), cos(rads), 0.0,
		       0.0,       0.0, 1.0
	);
	// Résultat de la rotation.
	return rotation * v;
}
