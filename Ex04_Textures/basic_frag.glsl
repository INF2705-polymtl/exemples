#version 410


in vec2 texCoords;

uniform sampler2D tex0;

out vec4 fragColor;


void main() {
	fragColor = texture(tex0, texCoords);
}
