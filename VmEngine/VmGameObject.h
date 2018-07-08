#pragma once

class FBXModel;
class VmShader;

// Base�� �ϴ� �ܻ�ť�� �׸� �� �ְ� ������.
class VmGameObject
{
public:
	explicit VmGameObject(FBXModel* pMesh);
	~VmGameObject();

	virtual void Tick();
	virtual void Render();	

private:
	FBXModel* m_pMesh;
	VmShader* m_pShader;
};

