#version 410


// Les variables uniformes sont partagées entre tous les appels de nuanceurs (d'où le nom « uniforme »).
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


// Dans le code principal, on a manuellement configuré les attributs 0 et 1 pour être la position et la couleur des sommets.
// glVertexAttribPointer(monIndex, ...) -> layout(location = monIndex)
// Les variables d'entrée ont des valeurs spécifiques par sommet, elle viennent d'un glBufferData.
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;


// Les variables de sortie sont passées à la passe suivante de nuanceur (dans ce cas celui de fragments).
out vec4 color;


void main() {
	// Combiner les matrices de transformation en les multipliant dans l'ordre inverse de leur étape dans le pipeline de transformation.
	// On fait donc Projection * Visualisation * Modélisation. On se rappelle que la multiplication matricielle est associative, mais pas commutative.
	mat4 transform = projection * view * model;
	// Appliquer la matrice de transformation à la position du sommet. Il faut ajouter une coordonnée virtuelle W=1 au vecteur XYZ.
	vec4 clipCoords = transform * vec4(a_position, 1.0);
	
	// gl_Position est une variable de sortie « built-in » dans laquelle on met la position transformée du sommet.
	gl_Position = clipCoords;
	
	// Affecter la couleur telle-quelle. Elle sera interpolée entre les sommets pour le nuanceur de fragments.
	color = a_color;
}
