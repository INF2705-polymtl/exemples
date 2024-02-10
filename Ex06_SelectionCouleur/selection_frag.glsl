#version 410


uniform uint objectID = 0;
uniform vec4 objectColor = vec4(0, 0, 0, 1);

out vec4 fragColor;


void main() {
	fragColor = objectColor;
}
