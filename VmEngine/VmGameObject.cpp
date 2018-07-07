#include "stdafx.h"
#include "FBXModel.h"
#include "VmGameObject.h"


VmGameObject::VmGameObject() : m_pMesh(nullptr)
{
	m_pMesh = new FBXModel("cube.fbx");
}


VmGameObject::~VmGameObject()
{
}

void VmGameObject::Tick()
{
}

void VmGameObject::Render()
{
}
