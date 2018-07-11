#pragma once

class VmBuffer;
class FBXModel;
class Texture;

// Base로 일단 단색큐브 그릴 수 있게 만들자.
class VmGameObject
{
public:
	explicit VmGameObject(VkDeviceManager* pvkDeviceManager, FBXModel* pMesh, texture_object* pTexture);
	~VmGameObject();

	virtual void Initialize();
	virtual void Tick();
	virtual void Render();
	virtual void SetPosition(float x, float y, float z);
	virtual void SetDescriptorSet();
	virtual VkDescriptorSet* GetDescriptorSet() { return &m_vkDescriptorSet; }
	virtual void UpdateUniformBuffer(MatrixManager* pMatrixManager);
	virtual void PreDrawSetting();

private:
	void CreateUniformBuffer();
	bool memory_type_from_properties(uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex);
	VkDescriptorSet m_vkDescriptorSet;

	VkDeviceManager* m_pDeviceManager;

	Matrix4 m_mtxModel;
	VmBuffer* m_pUniformBuffer;
	FBXModel* m_pMesh;
	texture_object* m_pTexture;
	MatrixManager m_MatrixManager;

	// Vertex Buffer
	VkBuffer m_vkVertexBuffer;
	VkDeviceMemory m_vkVertexBufferDeviceMemory;
	VkDescriptorBufferInfo m_vkVertexBufferDescriptorBufferInfo;
	VkVertexInputBindingDescription m_VIBinding;
	VkVertexInputAttributeDescription m_VIAttribs[4];

	// Uniform Buffer
	VkDescriptorBufferInfo m_vkDescriptorBufferInfo;
};