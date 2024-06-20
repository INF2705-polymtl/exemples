#version 410

// On reçoit en entrée les triangles de la primitive et on sort les trois faces du pic extrudé, donc 9 sommets au total.
layout(triangles) in;
layout(triangle_strip, max_vertices = 9) out;


uniform mat4 model = mat4(1);
uniform mat4 view = mat4(1);
uniform mat4 projection = mat4(1);
uniform float extrudeLength = 0;
uniform bool usingWorldPositions = false;


in VertexOut {
	vec3 origPosition;
	vec3 worldPosition;
	vec2 texCoords;
} inputs[];


out vec2 texCoords;


void main() {
	// Choisir quelles coordonnées utiliser (coords d'objet ou coords de scène). En utilisant les coordonnées d'objet, on applique les transformations (matrice de modélisation) comme si les pics extrudés faisaient partie de la forme d'origine. En utilisant les coords de scène, on extrude sur la normale des faces déjà transformées.
	vec3 v0, v1, v2;
	if (usingWorldPositions) {
		v0 = inputs[0].worldPosition;
		v1 = inputs[1].worldPosition;
		v2 = inputs[2].worldPosition;
	} else {
		v0 = inputs[0].origPosition;
		v1 = inputs[1].origPosition;
		v2 = inputs[2].origPosition;
	}

	// Calculer le centroïde géométrique de la face en faisant la moyenne des trois sommets.
	vec3 faceCentroidPos = (v0 + v1 + v2) / 3.0;
	// Calculer la normale de la face avec le produit croisé de deux arêtes quelconques du triangle.
	vec3 faceNormal = normalize(cross(v1 - v0, v2 - v0));
	// Calculer la position du pic en additionnant le centroïde et la normale étirée. 
	vec3 spikeTip = faceCentroidPos + extrudeLength * faceNormal;

	// Calculer les coordonnées finales du pic. Si on a extrudé en utilisant les coords de scène, on applique les matrices de proj et visu, sinon on applique les trois matrices.
	mat4 transformMat = (usingWorldPositions) ? projection * view : projection * view * model;
	vec4 spikeTipNormPos = transformMat * vec4(spikeTip, 1);

	// Pour chacune des arêtes de la primitive, on génère un triangle dont l'arête original est la base et le pic calculée est le haut du triangle.
	for (int i = 0; i < 3; i++) {
		// Le premier sommet de la base du pic.
		gl_Position = gl_in[i].gl_Position;
		texCoords = inputs[0].texCoords;
		EmitVertex();

		// Le deuxième sommet de la base du pic.
		gl_Position = gl_in[(i + 1) % 3].gl_Position;
		texCoords = inputs[1].texCoords;
		EmitVertex();

		// Le sommet du pic. On remarque l'ordre dans lequel les sommets sont générés pour garder l'ordre de la primitive originale. De cette façon, si la face original est une face avant, les faces extrudées seront aussi des faces avant.
		gl_Position = spikeTipNormPos;
		texCoords = inputs[2].texCoords;
		EmitVertex();

		// On fait EndPrimitive() car on veut dessiner nos trois triangles indépendamment. Sinon, on dessine un triangle_strip. En faisant EndPrimitive(), on commence une nouvelle strip, donc on fait un nouveau triangle.
		EndPrimitive();
	}
}

