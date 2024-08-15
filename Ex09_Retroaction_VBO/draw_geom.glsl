#version 410

layout(points) in;
layout(triangle_strip, max_vertices = 8) out;


uniform mat4 model = mat4(1);
uniform mat4 view = mat4(1);
uniform mat4 projection = mat4(1);

uniform float trailLengthMax = 10;
uniform float speedMin = 1e-10;
uniform float speedMax = 10;


in VertexOut {
	vec3 position;
	vec3 velocity;
	float mass;
	float miscValue;
} inputs[];


out vec2 texCoords;
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
	float trailLength = mix(0.1, trailLengthMax, speedValue);
	// Couleur de la particule dépend de la vitesse (lent = rouge, vite = jaune).
	vec4 particleColor = mix(vec4(1, 0, 0, 1), vec4(1, 1, 0, 1), speedValue);

	// Le vecteur de rayon du sprite.
	vec3 radiusVec = direction * radius;

	// Les sommets du carré affichant le sprite de la particule.
	gl_Position = transformMat * vec4(center + rotate(radiusVec, 45), 1);
	texCoords = vec2(1, 0);
	color = particleColor;
	EmitVertex();
	gl_Position = transformMat * vec4(center + rotate(radiusVec, -45), 1);
	texCoords = vec2(0, 0);
	color = particleColor;
	EmitVertex();
	gl_Position = transformMat * vec4(center + rotate(radiusVec, 135), 1);
	texCoords = vec2(1, 0.5);
	color = particleColor;
	EmitVertex();
	gl_Position = transformMat * vec4(center + rotate(radiusVec, -135), 1);
	texCoords = vec2(0, 0.5);
	color = particleColor;
	EmitVertex();

	EndPrimitive();

	// Le triangle du sprite de la trainée derrière la particule (donc qui pointe à l'opposée de sa direction).
	// 1er sommet : le coin "tribord" (à droite de sa direction).
	gl_Position = transformMat * vec4(center + rotate(radiusVec, -135), 1);
	texCoords = vec2(0, 0.5);
	color = particleColor;
	EmitVertex();
	// 2e sommet : le coin "babord"
	gl_Position = transformMat * vec4(center + rotate(radiusVec, 135), 1);
	texCoords = vec2(1, 0.5);
	color = particleColor;
	EmitVertex();
	// 3e sommet : le bout de la queue. trailLength est en fait un facteur appliqué au rayon du polygone.
	gl_Position = transformMat * vec4(center + rotate(radiusVec * (0.7 + trailLength), 180), 1);
	texCoords = vec2(0.5, 1);
	color = particleColor;
	EmitVertex();
}


vec3 rotate(vec3 v, float angle) {
	float rads = radians(angle);
	// En GLSL, on n'a pas les rotate/scale/translate comme avec GLM, donc on bâtit notre propre matrice de rotation pour tourner le vecteur v autour de l'axe des z (on est dans le plan XY dans notre exemple pour garder ça simple).
	// Attention, les matrices GLSL sont en colonne-major, comme avec GLM.
	mat3 rotation = mat3(
		 cos(rads), sin(rads), 0.0,
		-sin(rads), cos(rads), 0.0,
		       0.0,       0.0, 1.0
	);
	// Résultat de la rotation.
	return rotation * v;
}
