#pragma once

class FBXModel;

// Base�� �ϴ� �ܻ�ť�� �׸� �� �ְ� ������.
class VmGameObject
{
public:
	VmGameObject();
	~VmGameObject();

	virtual void Tick();
	virtual void Render();	

private:
	FBXModel* m_pMesh;
};

