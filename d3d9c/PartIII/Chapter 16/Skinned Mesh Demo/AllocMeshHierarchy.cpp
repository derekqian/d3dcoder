//=============================================================================
// AllocMeshHierarchy.cpp by Frank Luna (C) 2005 All Rights Reserved.
//=============================================================================

#include "AllocMeshHierarchy.h"
#include "SkinnedMesh.h"

void CopyString(const char* input, char** output)
{
	if( input )
	{
		UINT length = (UINT)::strlen(input) + 1; // add 1 for terminating null charater.
		*output = new char[length];
		::strcpy(*output, input);
	}
	else
	{
		*output = 0;
	}
}

HRESULT AllocMeshHierarchy::CreateFrame(PCTSTR Name, D3DXFRAME** ppNewFrame)
{
	// Remark: CreateFrame must return a non-null value for ppNewFrame,
	// otherwise D3DXLoadMeshHierarchyFromX will interpret it as 
	// a failed operation.


	FrameEx* frameEx = new FrameEx();

	if( Name )	CopyString(Name, &frameEx->Name);
	else		CopyString("<no name>", &frameEx->Name);

	frameEx->pMeshContainer = 0;
	frameEx->pFrameSibling = 0;
	frameEx->pFrameFirstChild = 0;
	D3DXMatrixIdentity(&frameEx->TransformationMatrix);
	D3DXMatrixIdentity(&frameEx->toRoot);

	*ppNewFrame = frameEx;

    return D3D_OK;
}

HRESULT AllocMeshHierarchy::CreateMeshContainer(PCTSTR Name, 
	const D3DXMESHDATA* pMeshData, const D3DXMATERIAL* pMaterials, 
	const D3DXEFFECTINSTANCE* pEffectInstances, DWORD NumMaterials, 
	const DWORD *pAdjacency, ID3DXSkinInfo* pSkinInfo, 
	D3DXMESHCONTAINER** ppNewMeshContainer)
{	
	// Remark: CreateMeshContainer should always return a non-null value
	// for ppNewMeshContainer, even if we are not interested in the mesh 
	// (i.e., it contains no skin info), otherwise D3DXLoadMeshHierarchyFromX
	// will interpret it as  a failed operation.


	//===============================================================
	// Allocate a new D3DXMESHCONTAINER, and set its name.  

    D3DXMESHCONTAINER* meshContainer = new D3DXMESHCONTAINER();
	::ZeroMemory(meshContainer, sizeof(D3DXMESHCONTAINER));	
	if( Name )	CopyString(Name, &meshContainer->Name);
	else        CopyString("<no name>", &meshContainer->Name);


	//===============================================================
	// Save our created mesh container now because we might return 
	// early, and we must _always_ return an allocated container.

	*ppNewMeshContainer = meshContainer;


	//===============================================================
	// Only interested in meshes with skin info and regular meshes.
	// If a given mesh does not satisfy these requirements we do
	// not return an error; rather we simply skip loading anything
	// for this mesh container.

	if( pSkinInfo == 0 || pMeshData->Type != D3DXMESHTYPE_MESH)
		return D3D_OK;
	

	//===============================================================
	// Copy material data, and allocate memory for texture file names.

	meshContainer->NumMaterials = NumMaterials;
	meshContainer->pMaterials   = new D3DXMATERIAL[NumMaterials];
	for(DWORD i = 0; i < NumMaterials; ++i)
	{
		D3DXMATERIAL* mtrls = meshContainer->pMaterials;
		mtrls[i].MatD3D = pMaterials[i].MatD3D;
		mtrls[i].MatD3D.Ambient = pMaterials[i].MatD3D.Diffuse;

		CopyString(pMaterials[i].pTextureFilename, 
			&mtrls[i].pTextureFilename);
	}


	//===============================================================
	// Ignore effect instances and adjacency info for this demo.

	meshContainer->pEffects   = 0;
	meshContainer->pAdjacency = 0;


	//===============================================================
	// Save mesh and skininfo.

	meshContainer->MeshData.Type  = D3DXMESHTYPE_MESH;
	meshContainer->MeshData.pMesh = pMeshData->pMesh; 
	meshContainer->pSkinInfo      = pSkinInfo;
	pMeshData->pMesh->AddRef();
	pSkinInfo->AddRef();

    return D3D_OK;
}

HRESULT AllocMeshHierarchy::DestroyFrame(D3DXFRAME* pFrameToFree) 
{
	delete[] pFrameToFree->Name;
	delete pFrameToFree;
 
    return D3D_OK; 
}

HRESULT AllocMeshHierarchy::DestroyMeshContainer(D3DXMESHCONTAINER* pMeshContainerBase)
{
	delete[] pMeshContainerBase->Name;
	delete[] pMeshContainerBase->pAdjacency;
	delete[] pMeshContainerBase->pEffects;
 
	for(DWORD i = 0; i < pMeshContainerBase->NumMaterials; ++i)
		delete[] pMeshContainerBase->pMaterials[i].pTextureFilename;

	delete[] pMeshContainerBase->pMaterials;

	ReleaseCOM(pMeshContainerBase->MeshData.pMesh);
	ReleaseCOM(pMeshContainerBase->pSkinInfo);
 
	delete pMeshContainerBase;
 
    return D3D_OK;
}