#pragma once
#include "Common.h"

struct BoneAnimQT;
typedef float vec2[2];
//typedef std::map<int, FbxAMatrix> BoneAnimMatrix;
typedef std::map<int, BoneAnimQT> BoneAnimData;

// Quaternion & Translation
struct BoneAnimQT
{
	FbxVector4 translation;
	FbxQuaternion quaternion;
	FbxVector4 rotation; // 임시로 보관한다.
};

struct SubMesh
{
	SubMesh() : IndexOffset(0), TriangleCount(0) {}

	int IndexOffset;
	int TriangleCount;
};

struct VertexBoneData
{
	int		index;
	double	weight;
};

struct BoneData
{
	int			parentIndex;
	int			nowIndex;
	std::string boneName;

	bool operator==(const std::string& str) const { return str == boneName; }
};

class VertexData
{
public:
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
	glm::vec4 boneIndice;
	glm::vec4 boneWeights;

	bool operator==(const VertexData& other) const;
	bool operator==(const FbxVector4& other) const;
};

class FBXModel
{
private:
	std::vector<VertexData> m_vVertexData;
	std::vector<uint16_t> m_vIndex;

	std::map<int, std::vector<VertexBoneData>> m_SkinningMap;
	std::map<int, FbxMatrix> m_BoneOffsetMap;
	std::vector<BoneData> m_vBoneData;
	//std::map<int, BoneAnimMatrix> m_AnimData;
	std::map<int, BoneAnimData> m_AnimData;

	FbxArray<SubMesh*> mSubMeshes;

	bool mHasNormal;
	bool mHasUV;
	bool mAllByControlPoint;

	bool m_bFindBoneNode;

	int m_nNowFrame;
	int m_nMaxFrame;
	FbxTime::EMode m_FbxTime;

public:
	explicit FBXModel(const std::string& sFileName);
	~FBXModel();

	void GetMeshData(FbxMesh* pMesh);
	void LoadCacheRecursive(FbxScene* pScene, FbxNode* pNode);
	void GetBoneHierarchyData(FbxNode* pNode, int nNowIndex, int nParentIndex);
	void GetSkinningVertexData(FbxNode* pNode);
	void BindingVertexAndBoneData(FbxNode* pNode);
	//void GetPoseMatrix(FbxScene* pScene);
	void GetBoneOffsetData(FbxScene* pScene, FbxNode* pNode);

	int GetVertexDataSize();
	int GetIndices();
	int GetIndicesSize();
	void GetVBDataAndMemcpy(uint8_t* pData);
	void GetIndexDataAndMemcpy(uint16_t* pData);
	void GetBoneUniformData(uint8_t* pData);

	void AdvanceFrame();

	void FrameInterpolation(int nConvertFrame);
};