// Dans le modèle de Phong, les calculs géométriques (positions, directions, normales) sont faits dans le nuanceur de sommet et interpolés aux fragments. C'est ensuite dans chaque fragment que sont effectués les calculs d'éclairage avec les vecteurs interpolés en entrée.

#version 410


layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoords;


uniform mat4 model = mat4(1);
uniform mat4 view = mat4(1);
uniform mat4 projection = mat4(1);
uniform mat3 normalTransformMat = mat3(1);

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


// Les valeurs de sortie seront interpolés pour les fragments et le nuanceur de fragment fera les calculs d'éclairage pour chaque fragments.
out vec2 texCoords;
out vec3 normal;
out vec3 observerDir;
out vec3 lightDir;


void main() {
	// Appliquer les transformations habituelles.
	vec4 worldPosition = model * vec4(a_position, 1.0);
	vec4 viewPosition = view * worldPosition;
	vec4 normPosition = projection * viewPosition;

	gl_Position = normPosition;
	texCoords = a_texCoords;

	// Calculer la normale du sommet en appliquant l'inverse transposée de la matrice modèle-vue.
	normal = normalTransformMat * a_normal;

	// La position du sommet dans le référentiel de la caméra (donc coords de visualisation).
	vec3 pos = vec3(viewPosition);

	// Calculer la position de la lumière en coords de visualisation (light.position est en coordonnées de scène).
	lightDir = (view * light.position).xyz - pos;

	// Calculer le vecteur de direction de l'observateur. On peut optimiser en prenant z = 1 plutôt que de faire l'opposée de la coords de visualisation. C'est n'est pas l'optimisation la plus importante considérant tous les calculs qui sont faits autour. On se laisse cette option pour des raisons historiques (c'est dans le vieux modèle d'OpenGL) même si se sont des économies de bouts de chandelle.
	observerDir = (lightModel.localViewer) ? -pos : vec3(0, 0, 1);
}
