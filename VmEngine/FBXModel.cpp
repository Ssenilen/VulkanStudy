#include "stdafx.h"
#include "FBXModel.h"

const int TRIANGLE_VERTEX_COUNT = 3;

namespace std {
	template<> struct hash<VertexData> {
		size_t operator()(VertexData const& vertexData) const {
			return (hash<glm::vec3>()(vertexData.position) ^
				(hash<glm::vec3>()(vertexData.normal) << 1) >> 1) ^
				(hash<glm::vec2>()(vertexData.uv) << 1);
		}
	};
}

FBXModel::FBXModel(const std::string& sFileName) :
	mHasNormal(false), mHasUV(false), mAllByControlPoint(false),
	m_bFindBoneNode(false), m_nNowFrame(0), m_nMaxFrame(0)
{
	bool bResult = false;

	FbxManager* lSdkManager;
	FbxScene* lScene;
	
	InitializeSdkObjects(lSdkManager, lScene);

	bResult = LoadScene(lSdkManager, lScene, sFileName.data());
	assert(bResult);

	FbxGeometryConverter lGeomConverter(lSdkManager);
	lGeomConverter.Triangulate(lScene, true);

	GetBoneHierarchyData(lScene->GetRootNode(), 0, -1);
	LoadCacheRecursive(lScene, lScene->GetRootNode());
	GetSkinningVertexData(lScene->GetRootNode());

	BindingVertexAndBoneData(lScene->GetRootNode());
	//GetPoseMatrix(lScene);


	//std::cout << "[Vertex]" << std::endl;
	//for (int i = 0; i < m_vVertexData.size(); ++i)
	//{
	//	std::cout << "　v"<<i+1<<" { " << m_vVertexData.at(i).position.x << ", " << m_vVertexData.at(i).position.y << ", " << m_vVertexData.at(i).position.z << std::endl;
	//	std::cout << "　i"<<i+1<<" { " << m_vVertexData.at(i).boneIndice.x << ", " << m_vVertexData.at(i).boneIndice.y << ", " << m_vVertexData.at(i).boneIndice.z << std::endl;
	//	std::cout << "　w"<<i+1<<" { " << m_vVertexData.at(i).boneWeights.x << ", " << m_vVertexData.at(i).boneWeights.y << ", " << m_vVertexData.at(i).boneWeights.z << std::endl;
	//	std::cout << std::endl;
	//}

	//std::cout << "\n\n[INDEX]" << std::endl;
	//for (int i = 0; i < m_vIndex.size(); i+=3)
	//{
	//	std::cout << m_vIndex.at(i) << " " << m_vIndex.at(i + 1) << " " << m_vIndex.at(i + 2) << std::endl;
	//}

	//std::cout << "\n\n[Quaternion]" << std::endl;
	//for (int j = 0; j < 3; j++)
	//{
	//	std::cout << "　Bone 1:" << std::endl;
	//	for (int i = 0; i < 41; i++)
	//	{
	//		std::cout << "　　"
	//			<< m_AnimData[j][i].GetQ()[0] << " "
	//			<< m_AnimData[j][i].GetQ()[1] << " "
	//			<< m_AnimData[j][i].GetQ()[2] << " "
	//			<< m_AnimData[j][i].GetQ()[3] << std::endl;
	//	}
	//}
}

FBXModel::~FBXModel()
{
}

void FBXModel::GetMeshData(FbxMesh* pMesh)
{
	if (!pMesh)
		return;

	const int lPolygonCount = pMesh->GetPolygonCount();

	std::cout << lPolygonCount << "개의 Polygon" << std::endl;

	// Count the polygon count of each material
	FbxLayerElementArrayTemplate<int>* lMaterialIndice = NULL;
	FbxGeometryElement::EMappingMode lMaterialMappingMode = FbxGeometryElement::eNone;
	if (pMesh->GetElementMaterial())
	{
		lMaterialIndice = &pMesh->GetElementMaterial()->GetIndexArray();
		lMaterialMappingMode = pMesh->GetElementMaterial()->GetMappingMode();
		if (lMaterialIndice && lMaterialMappingMode == FbxGeometryElement::eByPolygon)
		{
			FBX_ASSERT(lMaterialIndice->GetCount() == lPolygonCount);
			if (lMaterialIndice->GetCount() == lPolygonCount)
			{
				// Count the faces of each material
				for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
				{
					const int lMaterialIndex = lMaterialIndice->GetAt(lPolygonIndex);
					if (mSubMeshes.GetCount() < lMaterialIndex + 1)
					{
						mSubMeshes.Resize(lMaterialIndex + 1);
					}
					if (mSubMeshes[lMaterialIndex] == NULL)
					{
						mSubMeshes[lMaterialIndex] = new SubMesh;
					}
					mSubMeshes[lMaterialIndex]->TriangleCount += 1;
				}

				// Make sure we have no "holes" (NULL) in the mSubMeshes table. This can happen
				// if, in the loop above, we resized the mSubMeshes by more than one slot.
				for (int i = 0; i < mSubMeshes.GetCount(); i++)
				{
					if (mSubMeshes[i] == NULL)
						mSubMeshes[i] = new SubMesh;
				}

				// Record the offset (how many vertex)
				const int lMaterialCount = mSubMeshes.GetCount();
				int lOffset = 0;
				for (int lIndex = 0; lIndex < lMaterialCount; ++lIndex)
				{
					mSubMeshes[lIndex]->IndexOffset = lOffset;
					lOffset += mSubMeshes[lIndex]->TriangleCount * 3;
					// This will be used as counter in the following procedures, reset to zero
					mSubMeshes[lIndex]->TriangleCount = 0;
				}
				FBX_ASSERT(lOffset == lPolygonCount * 3);
			}
		}
	}

	// All faces will use the same material.
	if (mSubMeshes.GetCount() == 0)
	{
		mSubMeshes.Resize(1);
		mSubMeshes[0] = new SubMesh();
	}

	// 메시 데이터를 캐싱
	// Normal이나 UV가 Polygon Vertex라면, 모든 vertex를 polygon의 vertex 기준으로 계산한다.
	mHasNormal = pMesh->GetElementNormalCount() > 0;
	mHasUV = pMesh->GetElementUVCount() > 0;
	FbxGeometryElement::EMappingMode lNormalMappingMode = FbxGeometryElement::eNone;
	FbxGeometryElement::EMappingMode lUVMappingMode = FbxGeometryElement::eNone;
	if (mHasNormal)
	{
		lNormalMappingMode = pMesh->GetElementNormal(0)->GetMappingMode();
		if (lNormalMappingMode == FbxGeometryElement::eNone)
		{
			mHasNormal = false;
		}
		if (mHasNormal && lNormalMappingMode != FbxGeometryElement::eByControlPoint)
		{
			mAllByControlPoint = false;
		}
	}
	if (mHasUV)
	{
		lUVMappingMode = pMesh->GetElementUV(0)->GetMappingMode();
		if (lUVMappingMode == FbxGeometryElement::eNone)
		{
			mHasUV = false;
		}
		if (mHasUV && lUVMappingMode != FbxGeometryElement::eByControlPoint)
		{
			mAllByControlPoint = false;
		}
	}

	// Control Point나 Polygon 별로 메모리를 할당
	int lPolygonVertexCount = pMesh->GetControlPointsCount();
	if (!mAllByControlPoint)
	{
		lPolygonVertexCount = lPolygonCount * TRIANGLE_VERTEX_COUNT;
	}

	// Control Point를 기준으로 vertex 속성 배열을 채운다.
	const FbxVector4 * lControlPoints = pMesh->GetControlPoints();
	FbxVector4 lCurrentVertex;
	FbxVector4 lCurrentNormal;
	FbxVector2 lCurrentUV;

	FbxStringList lUVNames;
	pMesh->GetUVSetNames(lUVNames);
	const char * lUVName = NULL;
	if (mHasUV && lUVNames.GetCount())
	{
		lUVName = lUVNames[0];
	}
	
	std::unordered_map<VertexData, uint32_t> uniqueVertices = {};
	
	/// 사용하지 않는다.
	//if (mAllByControlPoint)
	//{
	//	int lIndex = 0;
	//	const FbxGeometryElementNormal * lNormalElement = NULL;
	//	const FbxGeometryElementUV * lUVElement = NULL;
	//	if (mHasNormal)
	//	{
	//		lNormalElement = pMesh->GetElementNormal(0);
	//	}
	//	if (mHasUV)
	//	{
	//		lUVElement = pMesh->GetElementUV(0);
	//	}
	//	for (int lIndex = 0; lIndex < lPolygonVertexCount; ++lIndex)
	//	{
	//		// Save the vertex position.
	//		lCurrentVertex = lControlPoints[lIndex];
	//		vec3 vertice;
	//		vertice[0] = static_cast<float>(lCurrentVertex[0]);
	//		vertice[1] = static_cast<float>(lCurrentVertex[1]);
	//		vertice[2] = static_cast<float>(lCurrentVertex[2]);
	//		//m_vPosition.push_back(vertice);

	//		// Save the normal.
	//		if (mHasNormal)
	//		{
	//			int lNormalIndex = lIndex;
	//			if (lNormalElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
	//			{
	//				lNormalIndex = lNormalElement->GetIndexArray().GetAt(lIndex);
	//			}
	//			lCurrentNormal = lNormalElement->GetDirectArray().GetAt(lNormalIndex);
	//			vec3 normal;
	//			normal[0] = static_cast<float>(lCurrentNormal[0]);
	//			normal[1] = static_cast<float>(lCurrentNormal[1]);
	//			normal[2] = static_cast<float>(lCurrentNormal[2]);
	//			//m_vNormal.push_back(normal);
	//		}

	//		// Save the UV.
	//		if (mHasUV)
	//		{
	//			int lUVIndex = lIndex;
	//			if (lUVElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
	//			{
	//				lUVIndex = lUVElement->GetIndexArray().GetAt(lIndex);
	//			}
	//			lCurrentUV = lUVElement->GetDirectArray().GetAt(lUVIndex);
	//			vec2 uv;
	//			uv[0] = static_cast<float>(lCurrentUV[0]);
	//			uv[1] = static_cast<float>(lCurrentUV[1]);
	//			//m_vUV.push_back(uv);
	//		}
	//	}
	//}

	int lVertexCount = 0;
	for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
	{
		// The material for current face.
		int lMaterialIndex = 0;
		if (lMaterialIndice && lMaterialMappingMode == FbxGeometryElement::eByPolygon)
		{
			lMaterialIndex = lMaterialIndice->GetAt(lPolygonIndex);
		}

		// Where should I save the vertex attribute index, according to the material
		/// 굳이 필요없을 것 같아서 아래 lIndexOffset은 생략
		const int lIndexOffset = mSubMeshes[lMaterialIndex]->IndexOffset +
			mSubMeshes[lMaterialIndex]->TriangleCount * 3;
		for (int lVerticeIndex = 0; lVerticeIndex < TRIANGLE_VERTEX_COUNT; ++lVerticeIndex)
		{
			const int lControlPointIndex = pMesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);
			VertexData vertexData;

			if (mAllByControlPoint)
			{
				//m_vIndex.push_back(static_cast<unsigned int>(lControlPointIndex));
				assert(false);
			}
			// Populate the array with vertex attribute, if by polygon vertex.
			else
			{
				//m_vIndex.push_back(static_cast<unsigned int>(lVertexCount));

				lCurrentVertex = lControlPoints[lControlPointIndex];
				vertexData.position[0] = static_cast<float>(lCurrentVertex[0]);
				vertexData.position[1] = static_cast<float>(lCurrentVertex[1]);
				vertexData.position[2] = static_cast<float>(lCurrentVertex[2]);

				if (mHasNormal)
				{
					pMesh->GetPolygonVertexNormal(lPolygonIndex, lVerticeIndex, lCurrentNormal);
					vertexData.normal[0] = static_cast<float>(lCurrentNormal[0]);
					vertexData.normal[1] = static_cast<float>(lCurrentNormal[1]);
					vertexData.normal[2] = static_cast<float>(lCurrentNormal[2]);
				}

				if (mHasUV)
				{
					bool lUnmappedUV;
					pMesh->GetPolygonVertexUV(lPolygonIndex, lVerticeIndex, lUVName, lCurrentUV, lUnmappedUV);
					vertexData.uv[0] = static_cast<float>(lCurrentUV[0]);
					vertexData.uv[1] = 1.0f - static_cast<float>(lCurrentUV[1]);
				}
			}

			if (uniqueVertices.count(vertexData) == 0)
			{
				uniqueVertices[vertexData] = static_cast<uint32_t>(m_vVertexData.size());
				m_vVertexData.push_back(vertexData);
			}
			m_vIndex.push_back(uniqueVertices[vertexData]);
		}
		mSubMeshes[lMaterialIndex]->TriangleCount += 1;
	}

	//for (int i = 0; i < m_vIndex.size(); i += 3)
	//{
	//	int temp = m_vIndex.at(i + 1);
	//	m_vIndex.at(i + 1) = m_vIndex.at(i + 2);
	//	m_vIndex.at(i + 2) = temp;
	//}
}

void FBXModel::GetBoneHierarchyData(FbxNode* pNode, int nNowIndex, int nParentIndex)
{
	if (pNode->GetNodeAttribute() &&
		pNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		FbxSkeleton* lSkeleton = (FbxSkeleton*)pNode->GetNodeAttribute();
		BoneData bd;
		bd.boneName = pNode->GetName();
		bd.nowIndex = nNowIndex;
		bd.parentIndex = nParentIndex;
		m_vBoneData.push_back(bd);

		if (!m_bFindBoneNode)
		{
			m_bFindBoneNode = !m_bFindBoneNode;
		}
	}

	for (int i = 0; i < pNode->GetChildCount(); ++i)
	{
		GetBoneHierarchyData(pNode->GetChild(i), m_vBoneData.size(), m_bFindBoneNode ? nNowIndex : -1);
	}
}

void FBXModel::GetSkinningVertexData(FbxNode *pNode)
{
	if (pNode->GetNodeAttribute() &&
		pNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh)
	{
		FbxGeometry* pGeometry = static_cast<FbxMesh*>(pNode->GetNodeAttribute());

		int nSkinCount = pGeometry->GetDeformerCount(FbxDeformer::eSkin);

		for (int i = 0; i < nSkinCount; ++i)
		{
			int nClusterCount = ((FbxSkin*)pGeometry->GetDeformer(i, FbxDeformer::eSkin))->GetClusterCount();

			for (int j = 0; j < nClusterCount; ++j)
			{
				FbxCluster* pCluster = ((FbxSkin *)pGeometry->GetDeformer(i, FbxDeformer::eSkin))->GetCluster(j);
				int nIndexCount = pCluster->GetControlPointIndicesCount();
				int* pnIndices = pCluster->GetControlPointIndices();
				double* pdWeights = pCluster->GetControlPointWeights();

				std::vector<BoneData>::iterator iter;

				iter = std::find(m_vBoneData.begin(), m_vBoneData.end(), pCluster->GetLink()->GetName());

				// boneName이 매칭되지 않는 것을 오류 상황으로 간주한다.
				assert(iter != m_vBoneData.end());

				for (int k = 0; k < nIndexCount; ++k)
				{
					VertexBoneData vb;
					vb.index = iter->nowIndex;
					vb.weight = pdWeights[k];

					m_SkinningMap[pnIndices[k]].push_back(vb);
				}
			}
		}
	}

	const int lChildCount = pNode->GetChildCount();
	for (int lChildIndex = 0; lChildIndex < lChildCount; ++lChildIndex)
	{
		GetSkinningVertexData(pNode->GetChild(lChildIndex));
	}
}

void FBXModel::BindingVertexAndBoneData(FbxNode* pNode)
{
	if (pNode->GetNodeAttribute() &&
		pNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh)
	{
		FbxMesh* pMesh = pNode->GetMesh();
		int nControlPointCounts = pMesh->GetControlPointsCount();
		FbxVector4* pVertices = pMesh->GetControlPoints();

		for (int i = 0; i < nControlPointCounts; ++i)
		{
			if (m_SkinningMap[i].size() > 4)
			{
				std::sort(m_SkinningMap[i].begin(), m_SkinningMap[i].end(), [](const VertexBoneData& first, const VertexBoneData& second) {
					return first.weight > second.weight;
				});

				float sumWeight = 0.0f, interpolateWeight = 0.0f;
				for (int j = 0; j < 4; ++j)
				{
					sumWeight += m_SkinningMap[i].at(j).weight;
				}
				interpolateWeight = 1.0f - sumWeight;

				auto removeIter = m_SkinningMap[i].begin() + 4;
				m_SkinningMap[i].erase(removeIter, m_SkinningMap[i].end());

				m_SkinningMap[i].at(0).weight += interpolateWeight;				
			}
		}

		for (auto& vertexData : m_vVertexData)
		{
			for (int i = 0; i < nControlPointCounts; ++i)
			{
				// Position을 비교하도록 overriding
				if (vertexData == pVertices[i])
				{
					int nBoneIndicesIndex = 0; /// boneIndices의 index... ㅡㅡ
					for (auto vertexBoneData : m_SkinningMap[i])
					{
						vertexData.boneIndice[nBoneIndicesIndex] = vertexBoneData.index;
						vertexData.boneWeights[nBoneIndicesIndex] = vertexBoneData.weight;

						if (++nBoneIndicesIndex >= 4)
							break;
					}
				}
			}
		}
	}

	for (int i = 0; i < pNode->GetChildCount(); ++i)
	{
		BindingVertexAndBoneData(pNode->GetChild(i));
	}
}

void FBXModel::GetBoneOffsetData(FbxScene* pScene, FbxNode* pNode)
{
	FbxMesh* pMesh = pNode->GetMesh();
	int nDeformerCount = pMesh->GetDeformerCount(FbxDeformer::eSkin);

	assert(pMesh); // 왜 걸어야 할까?

	const FbxVector4 lT = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
	const FbxVector4 lR = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
	const FbxVector4 lS = pNode->GetGeometricScaling(FbxNode::eSourcePivot);

	FbxAMatrix geometryTransform = FbxAMatrix(lT, lR, lS);

	// 클러스터는 링크를 품고 있다.
	// Mesh에는 Deformer가 일반적으로 하나 있음.
	for (int deformerIndex = 0; deformerIndex < nDeformerCount; ++deformerIndex)
	{
		FbxSkin* pCurrSkin = static_cast<FbxSkin*>(pMesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));
		if (!pCurrSkin) continue;

		int nClusterCount = pCurrSkin->GetClusterCount();

		for (int clusterIndex = 0; clusterIndex < nClusterCount; ++clusterIndex)
		{
			FbxCluster* pCluster = pCurrSkin->GetCluster(clusterIndex);
			if (!pCluster->GetLink()) continue;

			std::string sCurrJointName = pCluster->GetLink()->GetName();

			std::vector<BoneData>::iterator findIter;
			findIter = std::find(m_vBoneData.begin(), m_vBoneData.end(), sCurrJointName);

			assert(findIter != m_vBoneData.end()); // 매칭되는 본이 없으면 안된다.

			FbxAMatrix transformMatrix;
			FbxAMatrix transformLinkMatrix;
			FbxAMatrix globalBindPoseInverseMatrix;

			// 바인딩 시 mesh의 transformation
			pCluster->GetTransformMatrix(transformMatrix);
			// 클러스터가 local space에서 worldspace로 변환
			pCluster->GetTransformLinkMatrix(transformLinkMatrix);
			globalBindPoseInverseMatrix = transformLinkMatrix.Inverse() * transformMatrix * geometryTransform;

			m_BoneOffsetMap[findIter->nowIndex] = globalBindPoseInverseMatrix;


			// 이 아래부터는 애니메이션 정보를 얻어온다.
			FbxAnimStack* currAnimStack = pScene->GetSrcObject<FbxAnimStack>(0);
			if (currAnimStack == nullptr) return;

			FbxString animStackName = currAnimStack->GetName();
			FbxTakeInfo* pTakeInfo = pScene->GetTakeInfo(animStackName.Buffer());
			FbxTime start = pTakeInfo->mLocalTimeSpan.GetStart();
			FbxTime end = pTakeInfo->mLocalTimeSpan.GetStop();
			
			// 음. 일단 공통된 Animation Length를 가지고 있다고 합시다.
			m_nMaxFrame = end.GetFrameCount(FbxTime::eFrames30) - start.GetFrameCount(FbxTime::eFrames30);

			BoneAnimMatrix currentTransformOffsetMap;
			for (FbxLongLong i = start.GetFrameCount(FbxTime::eFrames30); i <= end.GetFrameCount(FbxTime::eFrames30); ++i)
			{
				FbxTime currTime;
				currTime.SetFrame(i, FbxTime::eFrames30);

				FbxAMatrix currentTransformOffset = pNode->EvaluateGlobalTransform(currTime) * geometryTransform;
				currentTransformOffsetMap[i] = currentTransformOffset.Inverse() * pCluster->GetLink()->EvaluateGlobalTransform(currTime);
			}
			m_AnimData[findIter->nowIndex] = currentTransformOffsetMap;
		}
	}
}

// m_vBoneData가 완성된 이후에 실행되어야 한다.
//void FBXModel::GetPoseMatrix(FbxScene* pScene)
//{
//	int nPoseCount = pScene->GetPoseCount();
//
//	for (int i = 0; i < nPoseCount; ++i)
//	{
//		FbxPose* pPose = pScene->GetPose(i);
//		// 첫 Pose는 객체의 기본 Location을 불러오므로, 굳이 수집하지 않았다.
//		for (int j = 1; j < pPose->GetCount(); ++j)
//		{
//			std::string sName = pPose->GetNodeName(j).GetCurrentName();
//
//			std::vector<BoneData>::iterator iter;
//			iter = std::find(m_vBoneData.begin(), m_vBoneData.end(), sName);
//
//			// 매칭되는 본이 없으면 안된다.
//			assert(iter != m_vBoneData.end());
//
//			m_BoneOffsetMap[iter->nowIndex] = pPose->GetMatrix(j);
//		}
//	}
//
//	for (int i = 0; i < m_BoneOffsetMap.size(); i++)
//	{
//		std::cout << m_vBoneData.at(i).boneName.data() << std::endl;
//		std::cout << "\t" << m_BoneOffsetMap[i][0][0] << "\t" << m_BoneOffsetMap[i][0][1] << "\t" << m_BoneOffsetMap[i][0][2] << "\t" << m_BoneOffsetMap[i][0][3] << std::endl;
//		std::cout << "\t" << m_BoneOffsetMap[i][1][0] << "\t" << m_BoneOffsetMap[i][1][1] << "\t" << m_BoneOffsetMap[i][0][2] << "\t" << m_BoneOffsetMap[i][1][3] << std::endl;
//		std::cout << "\t" << m_BoneOffsetMap[i][2][0] << "\t" << m_BoneOffsetMap[i][2][1] << "\t" << m_BoneOffsetMap[i][0][2] << "\t" << m_BoneOffsetMap[i][2][3] << std::endl;
//		std::cout << "\t" << m_BoneOffsetMap[i][3][0] << "\t" << m_BoneOffsetMap[i][3][1] << "\t" << m_BoneOffsetMap[i][0][2] << "\t" << m_BoneOffsetMap[i][3][3] << std::endl;
//	}
//}

void FBXModel::LoadCacheRecursive(FbxScene* pScene, FbxNode* pNode)
{
	FbxNodeAttribute* lNodeAttribute = pNode->GetNodeAttribute();
	if (lNodeAttribute)
	{
		if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
		{
			FbxMesh * lMesh = pNode->GetMesh();
			if (lMesh && !lMesh->GetUserDataPtr())
			{
				GetMeshData(lMesh);
			}
			GetBoneOffsetData(pScene, pNode);
		}
	}

	const int lChildCount = pNode->GetChildCount();
	for (int lChildIndex = 0; lChildIndex < lChildCount; ++lChildIndex)
	{
		LoadCacheRecursive(pScene, pNode->GetChild(lChildIndex));
	}
}

int FBXModel::GetVertexDataSize()
{
	return m_vVertexData.size();
}

int FBXModel::GetIndices()
{
	return m_vIndex.size();
}

int FBXModel::GetIndicesSize()
{
	return m_vIndex.size() * sizeof(uint16_t);
}

void FBXModel::GetPositionData()
{
}

void FBXModel::GetUVData()
{
}

void FBXModel::GetVBDataAndMemcpy(uint8_t* pData)
{
	if (m_vVertexData.empty())
		return;

	std::vector<float> vData;	
	vData.reserve(m_vVertexData.size() * 16);

	for (auto it = m_vVertexData.begin(); it != m_vVertexData.end(); ++it)
	{
		vData.push_back(it->position[0]);
		vData.push_back(it->position[1]);
		vData.push_back(it->position[2]);
		vData.push_back(1.0f);

		vData.push_back(it->uv[0]);
		vData.push_back(it->uv[1]);
		vData.push_back(0.0f);
		vData.push_back(0.0f);

		vData.push_back(it->boneIndice[0]);
		vData.push_back(it->boneIndice[1]);
		vData.push_back(it->boneIndice[2]);
		vData.push_back(it->boneIndice[3]);

		vData.push_back(it->boneWeights[0]);
		vData.push_back(it->boneWeights[1]);
		vData.push_back(it->boneWeights[2]);
		vData.push_back(it->boneWeights[3]);
	}

	memcpy(pData, vData.data(), vData.size() * sizeof(float));
}

void FBXModel::GetIndexDataAndMemcpy(uint16_t* pData)
{
	if (m_vIndex.empty())
		return;
	
	memcpy(pData, m_vIndex.data(), m_vIndex.size() * sizeof(uint16_t));
}

void FBXModel::GetBoneUniformData(uint8_t* pData)
{
	if (m_BoneOffsetMap.empty())
		return;

	std::vector<float> vData;
	vData.reserve(m_BoneOffsetMap.size() * 16);
	
	for (int i = 0; i < m_BoneOffsetMap.size(); ++i)
	{
		BoneAnimMatrix& AnimData = m_AnimData[i];
		FbxVector4 translation = AnimData[m_nNowFrame].GetT();
		FbxVector4 scale = AnimData[m_nNowFrame].GetS();
		FbxVector4 rotation = AnimData[m_nNowFrame].GetR();
		FbxQuaternion quaternion = AnimData[m_nNowFrame].GetQ();
		glm::vec3 tran(translation[0], translation[1], translation[2]);
		glm::quat quat(quaternion[3], quaternion[0], quaternion[1], quaternion[2]);
		//		
		glm::mat4 mtxRotation = glm::toMat4(quat);
		glm::mat4 mtxTranslation = glm::translate(glm::mat4(), tran);
		glm::mat4 mtxScale = glm::mat4();
		
		glm::mat4 currentBoneOffset;

		for (int j = 0; j < 4; j++)
		{
			for (int k = 0; k < 4; k++)
			{
				currentBoneOffset[j][k] = m_BoneOffsetMap[i][j][k];
			}
		}

		// opengl & vulkan은 T * R * S | DX는 S * R * T 순으로 곱한다.
		glm::mat4 transformMatrix = mtxTranslation * mtxRotation * mtxScale;
		glm::mat4 finalMatrix = transformMatrix * currentBoneOffset;

		vData.push_back(finalMatrix[0][0]);
		vData.push_back(finalMatrix[0][1]);
		vData.push_back(finalMatrix[0][2]);
		vData.push_back(finalMatrix[0][3]);

		vData.push_back(finalMatrix[1][0]);
		vData.push_back(finalMatrix[1][1]);
		vData.push_back(finalMatrix[1][2]);
		vData.push_back(finalMatrix[1][3]);

		vData.push_back(finalMatrix[2][0]);
		vData.push_back(finalMatrix[2][1]);
		vData.push_back(finalMatrix[2][2]);
		vData.push_back(finalMatrix[2][3]);
					
		vData.push_back(finalMatrix[3][0]);
		vData.push_back(finalMatrix[3][1]);
		vData.push_back(finalMatrix[3][2]);
		vData.push_back(finalMatrix[3][3]);
	}

	memcpy(pData, vData.data(), vData.size() * sizeof(float));
	AdvanceFrame();
}

void FBXModel::AdvanceFrame()
{
	if (++m_nNowFrame > m_nMaxFrame)
		m_nNowFrame = 0;	
}

bool VertexData::operator==(const VertexData& other) const
{
	if (abs(this->position[0] - other.position[0]) > 0.001f)
		return false;
	if (abs(this->position[1] - other.position[1]) > 0.001f)
		return false;
	if (abs(this->position[2] - other.position[2]) > 0.001f)
		return false;
	if (abs(this->normal[0] - other.normal[0]) > 0.001f)
		return false;
	if (abs(this->normal[1] - other.normal[1]) > 0.001f)
		return false;
	if (abs(this->normal[2] - other.normal[2]) > 0.001f)
		return false;
	if (abs(this->uv[0] - other.uv[0]) > 0.001f)
		return false;
	if (abs(this->uv[1] - other.uv[1]) > 0.001f)
		return false;
	return true;
}

bool VertexData::operator==(const FbxVector4 & other) const
{
	if (abs(this->position[0] - other[0]) > 0.001f)
		return false;
	if (abs(this->position[1] - other[1]) > 0.001f)
		return false;
	if (abs(this->position[2] - other[2]) > 0.001f)
		return false;
	return true;
}