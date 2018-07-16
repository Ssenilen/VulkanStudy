#version 330
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (std140, binding = 0) uniform bufferVals {
	mat4 vp;
} myBufferVals;

layout (std140, binding = 1) uniform bone {
	mat4 boneTransform[96];
} boneOffset;

layout (location = 0) in vec4 pos;
layout (location = 1) in vec4 inUV;
layout (location = 2) in vec4 boneIndices;
layout (location = 3) in vec4 boneWeights;
// Instancing
layout (location = 4) in vec4 instanceMatrix0;
layout (location = 5) in vec4 instanceMatrix1;
layout (location = 6) in vec4 instanceMatrix2;
layout (location = 7) in vec4 instanceMatrix3;

layout (location = 0) out vec2 outUV;
//layout (location = 0) out vec4 outColor;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	outUV = inUV.xy;

	mat4 mtxInstanceModel;
	mtxInstanceModel[0] = instanceMatrix0;
	mtxInstanceModel[1] = instanceMatrix1;
	mtxInstanceModel[2] = instanceMatrix2;
	mtxInstanceModel[3] = instanceMatrix3;

	gl_Position = myBufferVals.vp * mtxInstanceModel * pos;
}