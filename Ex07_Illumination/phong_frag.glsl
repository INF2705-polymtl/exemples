// Dans le modèle de Phong, les calculs géométriques (positions, directions, normales) sont faits dans le nuanceur de sommet et interpolés aux fragments. C'est ensuite dans chaque fragment que sont effectués les calculs d'éclairage avec les vecteurs interpolés en entrée.

#version 410


in vec2 texCoords;
in vec3 normal;
in vec3 observerDir;
in vec3 lightDir;


uniform bool usingBlinnFormula = true;
uniform bool showingAmbientReflection = true;
uniform bool showingDiffuseReflection = true;
uniform bool showingSpecularReflection = true;
uniform int numCelShadingBands = 0;


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


out vec4 fragColor;


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
	fragColor = material.emissionColor;
	
	float dist = length(lightDir);
	float distFactor = 1.0 / (light.fadeCst + light.fadeLin * dist + light.fadeQuad * dist*dist);
	distFactor = min(1, distFactor);

	Reflections reflections = computeReflection(
		normalize(lightDir),
		normalize(gl_FrontFacing ? normal : -normal),
		normalize(observerDir),
		distFactor
	);

	// On peut faire un genre de "cel-shading" (Celluloid shading) pour donner un effet de cartoon, un peu comme dans Zelda Wind Waker pour ceux qui s'en rappellent. Généralement on va seulement appliquer le cel-shading sur la réflection diffuse.
	if (numCelShadingBands > 1)
		reflections.diffuse = floor(reflections.diffuse * numCelShadingBands) / numCelShadingBands;

	if (showingAmbientReflection)
		fragColor += reflections.ambient;
	if (showingDiffuseReflection)
		fragColor += reflections.diffuse;
	if (showingSpecularReflection)
		fragColor += reflections.specular;

	fragColor = clamp(fragColor, 0.0, 1.0);
}
