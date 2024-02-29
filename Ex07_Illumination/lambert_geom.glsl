// Dans le modèle de Lambert, les calculs sont effectués une fois par primitive, donc dans le nuanceur de géométrie. Les résultats sont passés également à tous les fragments de la primitive.

#version 410


// On reçoit en entrée les triangles de la primitives et on les sort dans le même format.
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;


in Attribs {
	vec3 origPosition;
	vec2 texCoords;
	vec3 normal;
} inAttribs[];


uniform mat4 model = mat4(1);
uniform mat4 view = mat4(1);
uniform mat4 projection = mat4(1);
uniform mat3 normalTransformMat = mat3(1);
uniform bool usingBlinnFormula = true;
uniform bool showingAmbientReflection = true;
uniform bool showingDiffuseReflection = true;
uniform bool showingSpecularReflection = true;

layout(std140) uniform Material
{
	vec4 emissionColor;
	vec4 ambientColor;
	vec4 diffuseColor;
	vec4 specularColor;
	float shininess;
} material;

layout(std140) uniform LightSource
{
	vec4 position;
	vec4 ambientColor;
	vec4 diffuseColor;
	vec4 specularColor;
	float fadeCst;
	float fadeLin;
	float fadeQuad;
	vec4 direction;
	float beamAngle;
	float exponent;
} light;

layout(std140) uniform LightModel
{
	vec4 ambientColor;
	bool localViewer;
} lightModel;


out vec2 texCoords;
out vec4 color;


struct Reflections
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};

Reflections computeReflection(vec3 l, vec3 n, vec3 o, float attenuation) {
	// Initialiser à noir opaque.
	Reflections result = Reflections(
		vec4(0, 0, 0, 1),
		vec4(0, 0, 0, 1),
		vec4(0, 0, 0, 1)
	);
	
	// Calculer la réflexion ambiante. Elle ne dépend pas de la géométrie de la scène.
	result.ambient = material.ambientColor * (lightModel.ambientColor + light.ambientColor);

	float dotProd = dot(n, l);
	// Si la face est éclairée. En effet, N · L < 0 implique que la face n'est pas éclairée de l'avant, donc on ne fait pas les calculs de réflexion diffuse et spéculaire.
	if (dotProd > 0.0) {
		// Calculer la réflexion diffuse. Elle dépend de la normale et de la position de la source lumineuse.
		result.diffuse = material.diffuseColor * light.diffuseColor * dotProd;
		// Appliquer le facteur d'atténuation selon la distance (c'est le même facteur pour la réflection diffuse et spéculaire).
		result.diffuse *= attenuation;

		// Calculer l'intensité de la réflection spéculaire selon la formule de Blinn ou Phong. Elle dépend de la position de la lumière, de la normale et de la position de l'observateur.
		float specIntensity = (usingBlinnFormula) ?
			// Blinn utilise la bissectrice.
			dot(normalize(l + o), n) :
			// Phong utilise le vecteur de réflexion.
			dot(reflect(-l, n), o);

		// Si le résultat est positif (il y a de la réflexion spéculaire).
		if (specIntensity > 0) {
			// Calculer le facteur de brillance en utilisant l'exposant de "shininess".
			float shine = pow(specIntensity, material.shininess);
			// Calculer la couleur spéculaire.
			result.specular = material.specularColor * light.specularColor * shine;
			// Appliquer le facteur d'atténuation selon la distance (c'est le même facteur pour la réflection diffuse et spéculaire).
			result.specular *= attenuation;
		}
	}

	return result;
}


void main() {
	texCoords = inAttribs[0].texCoords;

	// Calculer la normale de la primitive avec un sommet quelconque (le premier disons) vu qu'ils sont tous supposés être égaux dans le modèle de Lambert.
	vec3 normal = normalTransformMat * inAttribs[0].normal;

	// Calculer le centroïde géométrique de la face en faisant la moyenne des trois sommets.
	vec3 faceCentroid = (inAttribs[0].origPosition + inAttribs[1].origPosition + inAttribs[2].origPosition) / 3.0;

	// La position du sommet dans le référentiel de la caméra (donc coords de visualisation).
	vec3 pos = vec3(view * model * vec4(faceCentroid, 1));

	// Calculer la position de la lumière en coords de visualisation (light.position est en coordonnées de scène).
	vec3 lightDir = (view * light.position).xyz - pos;

	// Calculer le vecteur de direction de l'observateur. On peut optimiser en prenant z = 1 plutôt que de faire l'opposée de la coords de visualisation. C'est n'est pas l'optimisation la plus importante considérant tous les calculs qui sont faits autour. On se laisse cette option pour des raisons historiques (c'est dans le vieux modèle d'OpenGL) même si se sont des économies de bouts de chandelle.
	vec3 observerDir = (lightModel.localViewer) ? -pos : vec3(0, 0, 1);
	
	// Appliquer la couleur d'émission du matériau. C'est indépendant des sources lumineuses.
	vec4 faceColor = material.emissionColor;
	
	// Calculer le facteur d'atténuation selon la distance.
	float dist = length(lightDir);
	float distFactor = 1.0 / (light.fadeCst + light.fadeLin * dist + light.fadeQuad * dist*dist);
	distFactor = min(1, distFactor);
	
	// Calculer les réflexions.
	Reflections reflections = computeReflection(
		normalize(lightDir),
		normalize(normal),
		normalize(observerDir),
		distFactor
	);
	
	// Appliquer les réflexions selon celles qui sont activées ou pas (pour des fins pédagogiques).
	if (showingAmbientReflection)
		faceColor += reflections.ambient;
	if (showingDiffuseReflection)
		faceColor += reflections.diffuse;
	if (showingSpecularReflection)
		faceColor += reflections.specular;

	faceColor = clamp(faceColor, 0.0, 1.0);

	// Pour chacun des sommets de la primitives (3 sommets).
	for (int i = 0 ; i < gl_in.length() ; i++) {
		// Affecter la position du sommets sans faire de modifications par rapport à la sortie du nuanceur de sommets.
		gl_Position = gl_in[i].gl_Position;
		// Affecter la couleur de la face (la même pour tous les sommets).
		color = faceColor;
		// Émettre le sommet.
		EmitVertex();
	}

	// Le nuanceur de fragments pour Lambert fait juste prendre en entrée la couleur et l'affecter telle-quelle en sortie. On réutilise donc le nuanceur de fragments de Gouraud.
}
