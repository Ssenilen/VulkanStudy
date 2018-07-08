#version 330
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 2) uniform sampler2D tex;
layout (location = 0) in vec2 inUV;
//layout (location = 0) in vec4 inColor;
layout (location = 0) out vec4 outColor;

void main() {
		//outColor = inColor;//color;
		outColor = texture(tex, inUV);
}