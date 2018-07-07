#pragma once
class VmGameScene
{
public:
	VmGameScene();
	~VmGameScene();

	void Initialize();
	void Tick();
	void Render();

private:
	std::vector<VmGameObject> m_vObject;
};

