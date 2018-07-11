#include "stdafx.h"
#include "Texture.h"
#include "FBXModel.h"
#include "VmShader.h"
#include "VmBuffer.h"
#include "VmGameObject.h"

VmGameObject::VmGameObject(VkDeviceManager* pvkDeviceManager, FBXModel* pMesh, texture_object* pTexture) :
	m_pMesh(pMesh),
	m_pTexture(pTexture),
	m_pDeviceManager(pvkDeviceManager)
{
	m_pUniformBuffer = new VmBuffer(m_pDeviceManager->vkDevice, m_pDeviceManager->vkPhysicalDeviceMemoryProperties);
	CreateUniformBuffer();
	m_mtxModel = Matrix4(1.0f);
}


VmGameObject::~VmGameObject()
{
}

void VmGameObject::Initialize()
{
}

void VmGameObject::Tick()
{
}

void VmGameObject::Render()
{
}

void VmGameObject::SetPosition(float x, float y, float z)
{
	m_mtxModel[3][0] = x;
	m_mtxModel[3][1] = y;
	m_mtxModel[3][2] = z;
}

void VmGameObject::CreateUniformBuffer()
{
	m_pUniformBuffer->CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(Matrix4));
	m_vkDescriptorBufferInfo = m_pUniformBuffer->SetupDescriptor(sizeof(Matrix4));
	m_pUniformBuffer->Bind();
}

void VmGameObject::SetDescriptorSet()
{
	VkResult result;

	// 앞서 호출한 InitDescriptorPool을 이용해 DiscriptorPool을 생성하고 나면,
	// Pool에서 Descriptor를 할당할 수 있게 된다.
	VkDescriptorSetAllocateInfo alloc_info[1];
	alloc_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info[0].pNext = nullptr;
	alloc_info[0].descriptorPool = m_pDeviceManager->vkDescriptorPool;
	alloc_info[0].descriptorSetCount = 1;
	alloc_info[0].pSetLayouts = &m_pDeviceManager->vkDescriptorSetLayout;

	result = vkAllocateDescriptorSets(m_pDeviceManager->vkDevice, alloc_info, &m_vkDescriptorSet);
	assert(result == VK_SUCCESS);

	// 디스크립터 셋 업데이트
	VkWriteDescriptorSet writes[3];

	writes[0] = {};
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].pNext = nullptr;
	writes[0].dstSet = m_vkDescriptorSet;
	writes[0].descriptorCount = 1;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writes[0].pBufferInfo = &m_vkDescriptorBufferInfo;
	writes[0].dstArrayElement = 0;
	writes[0].dstBinding = 0;

	/// 임시
	writes[1] = {};
	writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[1].pNext = nullptr;
	writes[1].dstSet = m_vkDescriptorSet;
	writes[1].descriptorCount = 1;
	writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writes[1].pBufferInfo = &m_vkDescriptorBufferInfo;
	writes[1].dstArrayElement = 0;
	writes[1].dstBinding = 1;
	/// 임시
	
	VkDescriptorImageInfo tex_descs[1];
	memset(&tex_descs, 0, sizeof(tex_descs));
	for (unsigned int i = 0; i < TEXTURE_COUNT; i++)
	{
		tex_descs[i].sampler = m_pTexture->sampler;
		tex_descs[i].imageView = m_pTexture->view;
		tex_descs[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	}

	writes[2] = {};
	writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[2].dstSet = m_vkDescriptorSet;
	writes[2].dstBinding = 2;
	writes[2].descriptorCount = 1;
	writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writes[2].pImageInfo = tex_descs;
	writes[2].dstArrayElement = 0;

	vkUpdateDescriptorSets(m_pDeviceManager->vkDevice, 3, writes, 0, nullptr);
}

void VmGameObject::UpdateUniformBuffer(MatrixManager* pMatrixManager)
{
	VkResult result;
	if (pMatrixManager)
	{	

		Matrix4 mtxVp = pMatrixManager->mtxClip * pMatrixManager->mtxProjection * pMatrixManager->mtxView;
		Matrix4 mtxMVP = mtxVp * m_mtxModel;

		result = m_pUniformBuffer->Map(sizeof(Matrix4), 0);
		assert(result == VK_SUCCESS);
		memcpy(m_pUniformBuffer->GetMappedMemory(), &mtxMVP, sizeof(mtxMVP));
		m_pUniformBuffer->Unmap();
	}
}

void VmGameObject::PreDrawSetting()
{
	// 객체로 쪼개질때 추가해줘야함
	//vkCmdBindDescriptorSets(m_pDeviceManager->vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pDeviceManager->vkPipelineLayout, 0, 1, &m_vkDescriptorSet, 0, nullptr);
}

bool VmGameObject::memory_type_from_properties(uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex)
{
	for (uint32_t i = 0; i < m_pDeviceManager->vkPhysicalDeviceMemoryProperties.memoryTypeCount; i++)
	{
		if ((typeBits & 1) == 1)
		{
			// 사용할 수 있는 타입인지, 사용자 속성과 일치하는지
			if ((m_pDeviceManager->vkPhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask)
			{
				*typeIndex = i;
				return true;
			}
		}
		typeBits >>= 1;
	}
	return false;
}