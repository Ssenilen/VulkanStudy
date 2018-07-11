#pragma once
class VmBuffer
{
	VkDevice m_vkDevice;
	VkBuffer m_vkBuffer;
	VkDeviceMemory m_vkDeviceMemory;
	VkDeviceSize m_vkSize;
	VkDeviceSize m_vkAlignment;
	VkDescriptorBufferInfo m_vkDescriptorBufferInfo;
	void* m_pMapped;

	VkBufferUsageFlags m_vkUsageFlags;
	VkMemoryPropertyFlags m_vkMemoryPropertyFlags;

	VkPhysicalDeviceMemoryProperties m_vkPhysicalDeviceMemoryProperites;

public:
	explicit VmBuffer(VkDevice vkDevice, VkPhysicalDeviceMemoryProperties vkPhysicalDeviceMemoryProperties);
	~VmBuffer();
	VkResult CreateBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, void *data = nullptr);
	VkResult Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
	void Unmap();
	VkResult VmBuffer::Bind(VkDeviceSize offset = 0);
	VkDescriptorBufferInfo SetupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
	void* GetMappedMemory() { return m_pMapped; }


	bool memory_type_from_properties(uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex);
};

