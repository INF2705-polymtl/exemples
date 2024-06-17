#version 410


uniform float vertexZ = 0.69;
uniform vec4 vertexColor = vec4(0.69, 0.69, 0.69, 0.69);


out vec4 fragColor;


void main() {
	// Affecter la couleur voulu en sortie.
	fragColor = vertexColor;
	// Affecter directement la profondeur du fragment (entre 0 et 1).
	gl_FragDepth = vertexZ;
}
