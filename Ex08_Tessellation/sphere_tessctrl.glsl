#version 410

layout(vertices = 3) out;


uniform int tessLevelInner = 1;
uniform int tessLevelOuter = 1;


in VertexOut {
	vec3 origPosition;
	vec2 texCoords;
} inputs[];


out TessCtrlOut {
	vec3 origPosition;
	vec2 texCoords;
} outputs[];


void main() {
	// Passer la position et les attributs. Il faut utiliser l'indice d'invocation qui est le numéro du sommet courant.
	outputs[gl_InvocationID].origPosition = inputs[gl_InvocationID].origPosition;
	outputs[gl_InvocationID].texCoords = inputs[gl_InvocationID].texCoords;

	// On peut passer les niveaux de tessellation tels-quels dans toutes les invocations si on est garanti que les valeurs uniformes restent constantes (c'est notre cas). Cependant, par convention on le fait seulement dans le premier sommet du patch pour éviter les comportements indéfinis.
	if (gl_InvocationID == 0) {
		gl_TessLevelInner[0] = tessLevelInner;
		gl_TessLevelOuter[0] = tessLevelOuter;
		gl_TessLevelOuter[1] = tessLevelOuter;
		gl_TessLevelOuter[2] = tessLevelOuter;
	}
	// De la spec GLSL 4.x, ch. 2.2 :
	// Tessellation control shaders will get undefined results if one invocation reads a per-vertex or per-patch attribute written by another invocation at any point during the same phase, or if two invocations attempt to write different values to the same per-patch output in a single phase.
}
