#version 410


in vec4 color;
in vec2 texCoords;

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;

out vec4 fragColor;


vec4 composite(vec4 over, vec4 under) {
	vec4 result;
	result.a = over.a + under.a * (1 - over.a);
	result.rgb = (over.a * over.rgb + (1 - over.a) * under.a * under.rgb) / result.a;
	return result;
}


void main() {
	vec4 texel0 = texture(tex0, texCoords);
	vec4 texel1 = texture(tex1, texCoords);
	vec4 texel2 = texture(tex1, texCoords);
	fragColor = composite(texel1, texel0);
	fragColor = composite(texel2, fragColor);
	fragColor *= color;
	if (fragColor.a < 0.01)
		discard;
}
