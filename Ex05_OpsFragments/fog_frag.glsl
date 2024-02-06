#version 410


in vec2 texCoords;
in float distanceToCamera;

uniform sampler2D texDiffuse;
uniform vec3 fogColor;
uniform float fogNear;
uniform float fogFar;

out vec4 fragColor;


void main() {
	vec4 texColor = texture(texDiffuse, texCoords);
	float d = clamp(distanceToCamera, fogNear, fogFar);
	d = (d - fogNear) / (fogFar - fogNear);
	texColor.rgb = mix(texColor.rgb, fogColor, d);
	fragColor = texColor;
}
