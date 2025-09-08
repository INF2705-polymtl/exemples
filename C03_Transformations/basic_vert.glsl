#version 410


// Les matrices de transformations. Oui, on peut donner des valeurs par défaut aux variables uniformes. C'est la valeurs qu'elles ont si aucun glUniform* n'est fait dans le programme principal.
uniform mat4 model = mat4(1);
uniform mat4 view = mat4(1);
uniform mat4 projection = mat4(1);


// Les attributs 0, 1, 2 sont les attributs définis par la classe Mesh et sont les propriétés de base d'un sommet.
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoords;
layout(location = 3) in vec4 a_color;


out vec4 color;


void main() {
	// Combiner les matrices de transformation en les multipliant dans l'ordre inverse de leur étape dans le pipeline de transformation.
	// On fait donc Projection * Visualisation * Modélisation. On se rappelle que la multiplication matricielle est associative, mais pas commutative.
	mat4 transform = projection * view * model;
	// Appliquer la matrice de transformation à la position du sommet. Il faut ajouter une coordonnée virtuelle W=1 au vecteur XYZ.
	vec4 clipCoords = transform * vec4(a_position, 1.0);
	// gl_Position est une variable de sortie « built-in » dans laquelle on met la position transformée du sommet.
	gl_Position = clipCoords;

	// On pourrait passer la matrice de transformation (MVP) déjà calculée sur le CPU comme variable uniforme. C'est aussi une bonne idée d'avoir les trois matrices séparées dans le nuanceur de sommets pour garder plus de contrôle sur les calculs faits. Par exemple,on pourrait aussi séparer nos étapes de transformation pour obtenir les coordonnées de scène (model * a_position), les coords de visualisation (view * model * position) et coordonnées de projection (projection * view * model * position).
	//vec4 worldPosition = model * vec4(a_position, 1.0);
	//vec4 viewPosition = view * worldPosition;
	//vec4 clipPosition = projection * viewPosition;
	//gl_Position = clipPosition;
	
	// Affecter la couleur telle-quelle. Elle sera interpolée entre les sommets pour le nuanceur de fragments.
	color = a_color;
}
