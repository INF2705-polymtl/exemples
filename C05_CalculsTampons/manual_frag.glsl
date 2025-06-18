#version 410


// Donner des valeurs par défauts reconnaissables pour aider au débogage.
uniform float vertexZ = 0.69;
uniform vec4 vertexColor = vec4(0.69, 0.69, 0.69, 0.69);


out vec4 fragColor;


void main() {
	// Affecter la couleur voulue en sortie.
	fragColor = vertexColor;
	// Affecter directement la profondeur du fragment (entre 0 et 1).
	gl_FragDepth = vertexZ;
}
