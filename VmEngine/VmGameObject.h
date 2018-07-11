#pragma once

class FBXModel;
class Texture;

// Base�� �ϴ� �ܻ�ť�� �׸� �� �ְ� ������.
class VmGameObject
{
public:
	explicit VmGameObject(VkDeviceManager* pvkDeviceManager, FBXModel* pMesh, texture_object* pTexture);
	~VmGameObject();

	virtual void Initialize();
	virtual void Tick();
	virtual void Render();
	virtual void SetPosition(float x, float y, float z);

private:
	void CreateUniformBuffer();
	void SetDescriptorSet(VkDeviceManager* pvkDeviceManager);
	void UpdateUniformBuffer(VkDeviceManager* pvkDeviceManager, uniform_data* pUniformBuffer, Matrix4& mtxProjection);
	uniform_data m_UniformData;
	VkDescriptorSet m_vkDescriptorSet;

	Matrix4 m_mtxModel;
	FBXModel* m_pMesh;
	texture_object* m_pTexture;
	MatrixManager m_MatrixManager;
};