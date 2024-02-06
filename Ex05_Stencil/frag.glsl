#version 410


in vec2 texCoords;

uniform sampler2D texDiffuse;
uniform bool shouldDiscard = false;

out vec4 fragColor;


void main() {
	fragColor = texture(texDiffuse, texCoords);
	if (shouldDiscard && fragColor.a < 0.01)
		discard;
}
