#version 410


void main() {
	// Affecter (0,0,0) à gl_Position pour mettre le point au milieu de la fenêtre (on va affecter la couleur et la profondeur manuellement dans le nuanceur de fragments).
	gl_Position = vec4(0, 0, 0, 1);
}
