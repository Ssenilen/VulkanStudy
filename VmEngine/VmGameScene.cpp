#include "stdafx.h"
#include "VmGameObject.h"
#include "VmGameScene.h"


VmGameScene::VmGameScene()
{
}


VmGameScene::~VmGameScene()
{
}

void VmGameScene::Initialize()
{
}

void VmGameScene::Tick()
{
	for (auto objectIter = m_vObject.begin(); objectIter != m_vObject.end(); ++objectIter)
	{
		//objectIter->Tick();
	}
}

void VmGameScene::Render()
{
	for (auto objectIter = m_vObject.begin(); objectIter != m_vObject.end(); ++objectIter)
	{
		objectIter->Render();
	}
}
