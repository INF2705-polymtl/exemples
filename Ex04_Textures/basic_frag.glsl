#version 410


in vec4 color;
in vec2 texCoords;

uniform sampler2D tex0;

out vec4 fragColor;


void main() {
	fragColor = color * texture(tex0, texCoords);
}
