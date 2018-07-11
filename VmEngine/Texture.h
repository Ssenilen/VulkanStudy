#pragma once

#define TEXTURE_COUNT 1
#define TEXFORMAT VK_FORMAT_R8G8B8A8_UNORM



class Texture
{
private:
	VkBuffer m_vkStagingBuffer;
	VkDeviceMemory m_vkStagingBufferMemory;
	texture_object m_Texture[TEXTURE_COUNT];

	const VkDevice& m_vkDevice;
	const VkCommandBuffer& m_vkCommandBuffer;
	const VkPhysicalDeviceMemoryProperties& m_vkGPUMemoryProperties;


public:
	explicit Texture(const VkDevice& vkDevice, const VkCommandBuffer& vkCommandBuffer,
		const VkPhysicalDeviceMemoryProperties& vkGPUMemoryProperties, const char* sFileName);
	~Texture();

	void CreateTextureImage(int nTextureWidth, int nTextureHeight, void* pPixels);
	void SetImageLayout();
	void CreateSampler();
	texture_object GetTextureObject();

	// 커맨드 버퍼 객체도 만들어줘야 한다. (409p~410p)
	bool memory_type_from_properties(VkPhysicalDeviceMemoryProperties vkGPUMemoryProperties, uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex);
};