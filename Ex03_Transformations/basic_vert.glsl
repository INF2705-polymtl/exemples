#version 410


// Les attributs 0, 1, 2 sont les attributs définis par la classe Mesh et sont les propriétés de base d'un sommet.
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoords;
// L'attribut 3 est la couleur définie dans un autre VBO. On n'est pas obligé de s'en servir. Dans notre code, on ne définit pas de VBO pour la couleur par sommet dans le VAO du cube. a_color va donc être initialisé par défaut pour les cubes, puis non utilisé dans le nuanceur de fragment qui utilise une couleur globale.
layout(location = 3) in vec4 a_color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec4 color;


void main() {
	// On pourrait passer la matrice de transformation (MVP) déjà calculée sur le CPU comme variable uniforme. C'est aussi une bonne idée d'avoir les trois matrices séparées dans le nuanceur de sommets pour garder plus de contrôle sur les calculs faits.
	mat4 transform = projection * view * model;
	vec4 position = transform * vec4(a_position, 1.0);
	gl_Position = position;
	color = a_color;
}
