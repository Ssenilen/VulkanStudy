#include "stdafx.h"
#include "VmBuffer.h"

VmBuffer::VmBuffer(VkDevice vkDevice, VkPhysicalDeviceMemoryProperties vkPhysicalDeviceMemoryProperties) :
	m_vkDevice(vkDevice), m_vkBuffer(VK_NULL_HANDLE), m_vkDeviceMemory(VK_NULL_HANDLE),
	m_vkSize(0), m_vkAlignment(0), m_pMapped(nullptr), m_vkDescriptorBufferInfo({}),
	m_vkPhysicalDeviceMemoryProperites(vkPhysicalDeviceMemoryProperties)
{
}

VmBuffer::~VmBuffer()
{
}

VkResult VmBuffer::CreateBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, void * data)
{
	VkResult result;

	VkBufferCreateInfo buf_info = {};
	buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_info.pNext = nullptr;
	buf_info.usage = usageFlags;
	buf_info.size = size;
	buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	result = vkCreateBuffer(m_vkDevice, &buf_info, nullptr, &m_vkBuffer);
	assert(result == VK_SUCCESS);

	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(m_vkDevice, m_vkBuffer, &mem_reqs);
	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = nullptr;
	alloc_info.memoryTypeIndex = 0;
	alloc_info.allocationSize = mem_reqs.size;
	bool bPass = memory_type_from_properties(mem_reqs.memoryTypeBits, memoryPropertyFlags, &alloc_info.memoryTypeIndex);
	assert(bPass && "No mappable, coherent memory");
	vkAllocateMemory(m_vkDevice, &alloc_info, nullptr, &m_vkDeviceMemory);

	m_vkAlignment = mem_reqs.alignment;
	m_vkSize = alloc_info.allocationSize;
	m_vkUsageFlags = usageFlags;
	m_vkMemoryPropertyFlags = memoryPropertyFlags;
	
	if (data != nullptr)
	{
		result = Map();
		assert(result == VK_SUCCESS);
		memcpy(m_pMapped, data, size);
		Unmap();
	}

	// Initialize a default descriptor that covers the whole buffer size
	SetupDescriptor();

	// Attach the memory to the buffer object
	return Bind();
}

VkResult VmBuffer::Map(VkDeviceSize size, VkDeviceSize offset)
{
	return vkMapMemory(m_vkDevice, m_vkDeviceMemory, offset, size, 0, &m_pMapped);
}

void VmBuffer::Unmap()
{
	if (m_pMapped)
	{
		vkUnmapMemory(m_vkDevice, m_vkDeviceMemory);
		m_pMapped = nullptr;
	}
}

VkResult VmBuffer::Bind(VkDeviceSize offset)
{
	return vkBindBufferMemory(m_vkDevice, m_vkBuffer, m_vkDeviceMemory, offset);
}

VkDescriptorBufferInfo VmBuffer::SetupDescriptor(VkDeviceSize size, VkDeviceSize offset)
{
	m_vkDescriptorBufferInfo.offset = offset;
	m_vkDescriptorBufferInfo.buffer = m_vkBuffer;
	m_vkDescriptorBufferInfo.range = size;

	return m_vkDescriptorBufferInfo;
}

bool VmBuffer::memory_type_from_properties(uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex)
{
	for (uint32_t i = 0; i < m_vkPhysicalDeviceMemoryProperites.memoryTypeCount; i++)
	{
		if ((typeBits & 1) == 1)
		{
			// 사용할 수 있는 타입인지, 사용자 속성과 일치하는지
			if ((m_vkPhysicalDeviceMemoryProperites.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask)
			{
				*typeIndex = i;
				return true;
			}
		}
		typeBits >>= 1;
	}
	return false;
}