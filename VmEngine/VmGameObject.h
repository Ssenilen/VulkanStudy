#pragma once

class FBXModel;

// Base로 일단 단색큐브 그릴 수 있게 만들자.
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

