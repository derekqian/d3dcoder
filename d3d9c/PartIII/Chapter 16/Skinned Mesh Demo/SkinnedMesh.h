//=============================================================================
// SkinnedMesh.h by Frank Luna (C) 2005 All Rights Reserved.
//=============================================================================

#ifndef SKINNED_MESH_H
#define SKINNED_MESH_H

#include "d3dUtil.h"

struct FrameEx : public D3DXFRAME
{
	D3DXMATRIX toRoot;
};

class SkinnedMesh
{
public:
	SkinnedMesh(std::string XFilename);
	~SkinnedMesh();

	UINT numVertices();
	UINT numTriangles();
	UINT numBones();
	const D3DXMATRIX* getFinalXFormArray();

	void update(float deltaTime);
	void draw();

protected:
	D3DXFRAME* findNodeWithMesh(D3DXFRAME* frame);
	bool hasNormals(ID3DXMesh* mesh);
	void buildSkinnedMesh(ID3DXMesh* mesh);
	void buildToRootXFormPtrArray();
	void buildToRootXForms(FrameEx* frame, D3DXMATRIX& parentsToRoot);

	// We do not implement the required functionality to do deep copies,
	// so restrict copying.
	SkinnedMesh(const SkinnedMesh& rhs);
	SkinnedMesh& operator=(const SkinnedMesh& rhs);

protected:
	ID3DXMesh*     mSkinnedMesh;
	D3DXFRAME*     mRoot;
	DWORD          mMaxVertInfluences;
	DWORD          mNumBones;
	ID3DXSkinInfo* mSkinInfo;
	ID3DXAnimationController* mAnimCtrl;  
	
	std::vector<D3DXMATRIX>  mFinalXForms;
	std::vector<D3DXMATRIX*> mToRootXFormPtrs;

	static const int MAX_NUM_BONES_SUPPORTED = 35; 
};

#endif // SKINNED_MESH_H