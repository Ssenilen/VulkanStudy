#include "stdafx.h"
#include "Texture.h"
#include "FBXModel.h"
#include "VmShader.h"
#include "VmGameObject.h"

VmGameObject::VmGameObject(VkDeviceManager* pvkDeviceManager, FBXModel* pMesh, texture_object* pTexture) :
	m_pMesh(pMesh),
	m_pTexture(pTexture)
{
	//SetDescriptorSet(pvkDeviceManager);
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
}

void VmGameObject::SetDescriptorSet(VkDeviceManager* pvkDeviceManager)
{
	VkResult result;

	// 앞서 호출한 InitDescriptorPool을 이용해 DiscriptorPool을 생성하고 나면,
	// Pool에서 Descriptor를 할당할 수 있게 된다.
	VkDescriptorSetAllocateInfo alloc_info[1];
	alloc_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info[0].pNext = nullptr;
	alloc_info[0].descriptorPool = pvkDeviceManager->vkDescriptorPool;
	alloc_info[0].descriptorSetCount = 1;
	alloc_info[0].pSetLayouts = &pvkDeviceManager->vkDescriptorSetLayout;

	result = vkAllocateDescriptorSets(pvkDeviceManager->vkDevice, alloc_info, &m_vkDescriptorSet);
	assert(result == VK_SUCCESS);

	// 디스크립터 셋 업데이트
	VkWriteDescriptorSet writes[3];

	writes[0] = {};
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].pNext = nullptr;
	writes[0].dstSet = m_vkDescriptorSet;
	writes[0].descriptorCount = 1;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writes[0].pBufferInfo = &m_UniformData.buffer_info;
	writes[0].dstArrayElement = 0;
	writes[0].dstBinding = 0;

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

	vkUpdateDescriptorSets(pvkDeviceManager->vkDevice, 3, writes, 0, nullptr);
}

void VmGameObject::UpdateUniformBuffer(VkDeviceManager* pvkDeviceManager, uniform_data* pUniformBuffer, Matrix4& mtxProjection)
{
	VkResult result;

	float fov = glm::radians(45.0f);
	Matrix4 mtxView = glm::lookAt(
		Vector3(25.0f, 0.0f, 0.0f),	// CameraPos
		Vector3(0.0f, 0.0f, 0.0f),	// View
		Vector3(0.0f, 1.0f, 0.0f));	// Up Vector

	m_mtxModel = Matrix4(1.0f);
	// Vulkan clip space has inverted Y and half Z.
	Matrix4 mtxClip = Matrix4(1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.5f,
		0.0f, 0.0f, 0.0f, 1.0f);

	Matrix4 mtxVp = mtxClip * mtxProjection * mtxView;
	Matrix4 mtxMVP = mtxVp * m_mtxModel;
	
	uint8_t *pData = nullptr;
	memcpy(pData, &mtxMVP, sizeof(mtxMVP));
}
