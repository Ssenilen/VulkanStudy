#version 330
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (std140, binding = 0) uniform bufferVals {
	mat4 mvp;
} myBufferVals;

layout (std140, binding = 1) uniform bone {
	mat4 boneTransform[96];
} boneOffset;

layout (location = 0) in vec4 pos;
layout (location = 1) in vec4 inUV;
layout (location = 2) in vec4 boneIndices;
layout (location = 3) in vec4 boneWeights;
layout (location = 0) out vec2 outUV;
//layout (location = 0) out vec4 outColor;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	float weights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	weights[0] = 1 - boneWeights.y - boneWeights.z - boneWeights.w;
	weights[1] = boneWeights.y;
	weights[2] = boneWeights.z;
	weights[3] = boneWeights.w;

	vec4 pos_B = { 0.0f, 0.0f, 0.0f, 0.0f };

	pos_B += weights[0] * (boneOffset.boneTransform[int(boneIndices.x)] * pos);
	pos_B += weights[1] * (boneOffset.boneTransform[int(boneIndices.y)] * pos);
	pos_B += weights[2] * (boneOffset.boneTransform[int(boneIndices.z)] * pos);
	pos_B += weights[3] * (boneOffset.boneTransform[int(boneIndices.w)] * pos);

	outUV = inUV.xy;
	//outColor = vec4(weights[0], weights[1], weights[2], 1.0f);
 	gl_Position = myBufferVals.mvp * pos_B;
	//gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}