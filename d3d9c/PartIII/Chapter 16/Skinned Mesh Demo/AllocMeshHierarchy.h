//=============================================================================
// AllocMeshHierarchy.h by Frank Luna (C) 2005 All Rights Reserved.
//=============================================================================

#ifndef ALLOC_MESH_HIERARCHY_H
#define ALLOC_MESH_HIERARCHY_H

#include <d3dx9.h>

// Implements the ID3DXAllocateHierarchy interface.  In order to create and destroy an animation 
// hierarchy using the D3DXLoadMeshHierarchyFromX and D3DXFrameDestroy functions, we must implement
// the ID3DXAllocateHierarchy interface, which defines how meshes and frames are created and 
// destroyed, thereby giving us some flexibility in the construction and destruction process.

class AllocMeshHierarchy : public ID3DXAllocateHierarchy 
{
public:
	HRESULT STDMETHODCALLTYPE CreateFrame(THIS_ PCSTR Name, D3DXFRAME** ppNewFrame);                     

	HRESULT STDMETHODCALLTYPE CreateMeshContainer(PCSTR Name, const D3DXMESHDATA* pMeshData,               
		const D3DXMATERIAL* pMaterials, const D3DXEFFECTINSTANCE* pEffectInstances, DWORD NumMaterials, 
		const DWORD *pAdjacency, ID3DXSkinInfo* pSkinInfo, D3DXMESHCONTAINER** ppNewMeshContainer);     

	HRESULT STDMETHODCALLTYPE DestroyFrame(THIS_ D3DXFRAME* pFrameToFree);              
	HRESULT STDMETHODCALLTYPE DestroyMeshContainer(THIS_ D3DXMESHCONTAINER* pMeshContainerBase);
};

#endif // ALLOC_MESH_HIERARCHY_H