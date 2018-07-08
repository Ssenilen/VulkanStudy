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
	outUV = inUV.xy;
	//outColor = vec4(weights[0], weights[1], weights[2], 1.0f);
 	gl_Position = myBufferVals.mvp * pos;
	//gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}