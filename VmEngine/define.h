#pragma once

#define Vector3 glm::vec3
#define Vector4 glm::vec4
#define Matrix4 glm::mat4

/// TODO: 완전 편법. 나중에 구조체를 GameObject에서 관리하게 만들자.
struct uniform_data
{
	VkBuffer buf;
	VkDeviceMemory mem;
	VkDescriptorBufferInfo buffer_info;
};
struct texture_object {
	VkSampler sampler;

	VkImage image;
	VkImageLayout imageLayout;

	VkMemoryAllocateInfo mem_alloc;
	VkDeviceMemory mem;
	VkImageView view;
	int32_t tex_width, tex_height;
};

typedef struct {
	VkDevice vkDevice;
	VkDescriptorPool vkDescriptorPool;
	VkDescriptorSetLayout vkDescriptorSetLayout;
	VkDescriptorSet vkDescriptorSet;
} VkDeviceManager;

typedef struct {
	Matrix4		mtxView;
	Matrix4		mtxProjection;
	Matrix4		mtxClip;
	float		fov;
	float		fCameraPosX;
	float		fCameraPosY;
	float		fCameraPosZ;
} MatrixManager;