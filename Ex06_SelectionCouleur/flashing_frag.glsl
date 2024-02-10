#version 410


in vec2 texCoords;

uniform sampler2D texDiffuse;
uniform vec4 flashingColor = vec4(1, 1, 1, 1);
uniform float flashingValue = 0;

out vec4 fragColor;


void main() {
	vec4 texColor = texture(texDiffuse, texCoords);
	vec4 invColor = vec4(vec3(1, 1, 1) - texColor.rgb, 1);
//	fragColor = mix(texColor, flashingColor, flashingValue);
	fragColor = mix(texColor, invColor, flashingValue);
}
