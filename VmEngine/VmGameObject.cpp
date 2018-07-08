#include "stdafx.h"
#include "FBXModel.h"
#include "VmShader.h"
#include "VmGameObject.h"

VmGameObject::VmGameObject(FBXModel* pMesh) :
	m_pMesh(pMesh),
	m_pShader(nullptr)
{
	m_pShader = new VmShader();
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
