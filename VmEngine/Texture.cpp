#include "stdafx.h"
#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Texture::Texture(const VkDevice& vkDevice, const VkCommandBuffer& vkCommandBuffer, const VkPhysicalDeviceMemoryProperties& vkGPUMemoryProperties, const char* sFileName)
	: m_vkDevice(vkDevice), m_vkCommandBuffer(vkCommandBuffer), m_vkGPUMemoryProperties(vkGPUMemoryProperties)
{
	int nTextureWidth, nTextureHeight, nTextureChannels;

	stbi_uc* pixels = stbi_load(sFileName, &nTextureWidth, &nTextureHeight, &nTextureChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = nTextureWidth * nTextureHeight * 4;
	
	assert(pixels);

	CreateTextureImage(nTextureWidth, nTextureHeight, pixels);
	SetImageLayout();

	//m_Texture[0].image = 0;

	CreateSampler();
} 

Texture::~Texture()
{
}

void Texture::CreateTextureImage(int nTextureWidth, int nTextureHeight, void* pPixels)
{
	VkResult result;
	const VkFormat tex_format = TEXFORMAT;
	stbi_uc* pixels = static_cast<stbi_uc*>(pPixels);

	VkImageCreateInfo image_create_info;
	image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.pNext = nullptr;
	image_create_info.imageType = VK_IMAGE_TYPE_2D;
	image_create_info.format = tex_format;
	image_create_info.extent.width = nTextureWidth;
	image_create_info.extent.height = nTextureHeight;
	image_create_info.extent.depth = 1;
	image_create_info.mipLevels = 1;
	image_create_info.arrayLayers = 1;
	image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
	image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
	image_create_info.flags = 0;
	image_create_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

	// 이미지 객체 생성
	result = vkCreateImage(m_vkDevice, &image_create_info, nullptr, &m_Texture[0].image);
	assert(result == VK_SUCCESS);

	// 버퍼 메모리 요구사항을 가져온다
	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(m_vkDevice, m_Texture[0].image, &mem_reqs);

	// 메타 데이터 정보 생성
	m_Texture[0].mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	m_Texture[0].mem_alloc.pNext = nullptr;
	m_Texture[0].mem_alloc.allocationSize = mem_reqs.size;
	m_Texture[0].mem_alloc.memoryTypeIndex = 0;

	bool pass = memory_type_from_properties(m_vkGPUMemoryProperties,
		mem_reqs.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&m_Texture[0].mem_alloc.memoryTypeIndex);
	assert(pass);
	
	// 버퍼 개체를 위한 메모리 할당
	result = vkAllocateMemory(m_vkDevice, &m_Texture[0].mem_alloc, nullptr, &m_Texture[0].mem);
	assert(result == VK_SUCCESS);

	// 이미지 장치 메모리를 바인딩
	result = vkBindImageMemory(m_vkDevice, m_Texture[0].image, m_Texture[0].mem, 0);
	assert(result == VK_SUCCESS);


	// 여기서부터 할당된 장치에 메모리를 채운다.
	VkImageSubresource subres;
	subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subres.mipLevel = 0;
	subres.arrayLayer = 0;

	VkSubresourceLayout layout;
	void *pData;

	vkGetImageSubresourceLayout(m_vkDevice, m_Texture[0].image, &subres, &layout);
	
	// GPU 메모리를 로컬 호스트로 매핑
	result = vkMapMemory(m_vkDevice, m_Texture[0].mem, 0, m_Texture[0].mem_alloc.allocationSize, 0, &pData);
	assert(result == VK_SUCCESS);

	memcpy(pData, pixels, static_cast<size_t>(m_Texture[0].mem_alloc.allocationSize));
	stbi_image_free(pixels);

	vkUnmapMemory(m_vkDevice, m_Texture[0].mem);
}

void Texture::SetImageLayout()
{
	VkImageSubresourceRange subresourceRange;
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1; // image_create_info.mipLevels;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.layerCount = 1;

	m_Texture[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkImageMemoryBarrier image_memory_barrier;
	image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_memory_barrier.pNext = nullptr;
	image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
	image_memory_barrier.dstAccessMask = 0;
	image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	image_memory_barrier.newLayout = m_Texture[0].imageLayout;
	image_memory_barrier.image = m_Texture[0].image;
	image_memory_barrier.subresourceRange = subresourceRange;
	image_memory_barrier.dstAccessMask =
		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;

	vkCmdPipelineBarrier(m_vkCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);
}

void Texture::CreateSampler()
{
	const VkFormat tex_format = TEXFORMAT;
	VkResult result;

	VkSamplerCreateInfo sampler;
	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.pNext = nullptr;
	sampler.magFilter = VK_FILTER_NEAREST;
	sampler.minFilter = VK_FILTER_NEAREST;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.mipLodBias = 0.0f;
	sampler.anisotropyEnable = VK_FALSE;
	sampler.maxAnisotropy = 1;
	sampler.compareOp = VK_COMPARE_OP_NEVER;
	sampler.minLod = 0.0f;
	sampler.maxLod = 0.0f;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	sampler.unnormalizedCoordinates = VK_FALSE;

	// 사실 SubResourceRange는 ImageLayout 만들어줄 때도 쓰인다.
	VkImageSubresourceRange subresourceRange;
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1; // image_create_info.mipLevels;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.layerCount = 1;

	VkImageViewCreateInfo view;
	view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view.pNext = nullptr;
	view.image = VK_NULL_HANDLE;
	view.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view.format = tex_format;
	view.components.r = VK_COMPONENT_SWIZZLE_R;
	view.components.g = VK_COMPONENT_SWIZZLE_G;
	view.components.b = VK_COMPONENT_SWIZZLE_B;
	view.components.a = VK_COMPONENT_SWIZZLE_A;
	view.subresourceRange = subresourceRange;
	view.flags = 0;
	
	result = vkCreateSampler(m_vkDevice, &sampler, nullptr, &m_Texture[0].sampler);
	assert(result == VK_SUCCESS);

	view.image = m_Texture[0].image;
	result = vkCreateImageView(m_vkDevice, &view, nullptr, &m_Texture[0].view);
	assert(result == VK_SUCCESS);
}

texture_object Texture::GetTextureObject()
{
	return m_Texture[0];
}

bool Texture::memory_type_from_properties(VkPhysicalDeviceMemoryProperties vkGPUMemoryProperties, uint32_t typeBits,
	VkFlags requirements_mask,
	uint32_t *typeIndex) {
	// Search memtypes to find first index with those properties
	for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
		if ((typeBits & 1) == 1) {
			// Type is available, does it match user properties?
			if ((vkGPUMemoryProperties.memoryTypes[i].propertyFlags &
				requirements_mask) == requirements_mask) {
				*typeIndex = i;
				return true;
			}
		}
		typeBits >>= 1;
	}
	// No memory types matched, return failure
	return false;
}