#version 410


uniform float vertexZ = 0.69;
uniform vec4 vertexColor = vec4(0.69, 0.69, 0.69, 0.69);

out vec4 color;


void main() {
	// Affecter manuellement la valeur en z et la couleur selon les variables uniformes. Il faut projeter le z voulu en [0, 1] sur [-1, 1].
	gl_Position = vec4(0, 0, mix(-1, 1, vertexZ), 1);
	color = vertexColor;
}
