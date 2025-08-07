// Dans le modèle de Gouraud, les calculs sont effectués une fois par sommet, donc dans le nuanceur de sommets. Les résultats (couleurs) sont passés en sortie des sommets et donc interpolés aux fragments.

#version 410


uniform mat4 model = mat4(1);
uniform mat4 view = mat4(1);
uniform mat4 projection = mat4(1);
uniform mat3 normalTransformMat = mat3(1);
uniform bool usingBlinnFormula = true;
uniform bool showingAmbientReflection = true;
uniform bool showingDiffuseReflection = true;
uniform bool showingSpecularReflection = true;

// Les matériaux, sources lumineuses et modèle d'éclairage sont des struct dans le C++ et chargées comme des blocs uniformes. C'est plus commode et efficace que plein de variables uniformes.
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


layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoords;


out vec2 texCoords;
out vec4 color;


// Les trois couleurs réfléchies par un objet.
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
	result.ambient = clamp(result.ambient, 0, 1);

	float dotProd = dot(n, l);
	// Si la face est éclairée. En effet, N · L < 0 implique que la face n'est pas éclairée de l'avant, donc on ne fait pas les calculs de réflexion diffuse et spéculaire.
	if (dotProd > 0.0) {
		// Calculer la réflexion diffuse. Elle dépend de la normale et de la position de la source lumineuse.
		result.diffuse = material.diffuseColor * light.diffuseColor * dotProd;
		// Appliquer le facteur d'atténuation selon la distance (c'est le même facteur pour la réflexion diffuse et spéculaire).
		result.diffuse *= attenuation;
		result.diffuse = clamp(result.diffuse, 0, 1);

		// Calculer l'intensité de la réflexion spéculaire selon la formule de Blinn ou Phong. Elle dépend de la position de la lumière, de la normale et de la position de l'observateur.
		float specIntensity = (usingBlinnFormula) ?
			// Blinn utilise la bissectrice.
			dot(normalize(l + o), n) :
			// Phong utilise le vecteur de réflexion.
			dot(reflect(-l, n), o);

		// Si le résultat est positif (il y a de la réflexion spéculaire).
		if (specIntensity > 0) {
			// Appliquer l'exposant de brillance ("shininess").
			specIntensity = pow(specIntensity, material.shininess);
			// Calculer la couleur spéculaire.
			result.specular = material.specularColor * light.specularColor * specIntensity;
			// Appliquer le facteur d'atténuation selon la distance (c'est le même facteur pour la réflexion diffuse et spéculaire).
			result.specular *= attenuation;
			result.specular = clamp(result.specular, 0, 1);
		}
	}

	return result;
}


void main() {
	// Appliquer les transformations habituelles.
	vec4 worldPosition = model * vec4(a_position, 1.0);
	vec4 viewPosition = view * worldPosition;
	vec4 clipPosition = projection * viewPosition;

	gl_Position = clipPosition;
	texCoords = a_texCoords;

	// Calculer la normale du sommet en appliquant l'inverse transposée de la matrice modèle-vue.
	vec3 normal = normalTransformMat * a_normal;

	// La position du sommet dans le référentiel de la caméra (donc coords de visualisation).
	vec3 pos = vec3(viewPosition);

	// Calculer la position de la lumière en coords de visualisation (light.position est en coordonnées de scène).
	vec3 lightDir = (view * light.position).xyz - pos;

	// Calculer le vecteur de direction de l'observateur. On peut optimiser en prenant z = 1 plutôt que de faire l'opposée de la coords de visualisation. Ce n'est pas l'optimisation la plus importante considérant tous les calculs qui sont faits autour. On se laisse cette option pour des raisons historiques (c'est dans le vieux modèle d'OpenGL) même si ce sont des économies de bouts de chandelle.
	vec3 observerDir = (lightModel.localViewer) ? -pos : vec3(0, 0, 1);

	// Appliquer la couleur d'émission du matériau. C'est indépendant des sources lumineuses.
	color = material.emissionColor;
	
	// Calculer le facteur d'atténuation selon la distance.
	float dist = length(lightDir);
	float distFactor = 1.0 / (light.fadeCst + light.fadeLin * dist + light.fadeQuad * dist*dist);
	distFactor = clamp(distFactor, 0, 1);

	// Calculer les réflexions.
	Reflections reflections = computeReflection(
		normalize(lightDir),
		normalize(normal),
		normalize(observerDir),
		distFactor
	);

	// Appliquer les réflexions selon celles qui sont activées ou pas (pour des fins pédagogiques).
	if (showingAmbientReflection)
		color += reflections.ambient;
	if (showingDiffuseReflection)
		color += reflections.diffuse;
	if (showingSpecularReflection)
		color += reflections.specular;

	color = clamp(color, 0.0, 1.0);
}
