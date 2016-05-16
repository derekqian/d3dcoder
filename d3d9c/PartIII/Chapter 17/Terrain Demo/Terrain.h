//=============================================================================
// Terrain.h by Frank Luna (C) 2004 All Rights Reserved.
//
// Assumptions: In this book, we only use one terrain per demo and we 
//              construct the terrain geometry directly in world space.
//=============================================================================

#ifndef TERRAIN_H
#define TERRAIN_H

#include "Heightmap.h"
#include "d3dUtil.h"
#include "Vertex.h"

 
class Terrain
{
public:
	Terrain(UINT vertRows, UINT vertCols, float dx, float dz, 
		std::string heightmap, std::string tex0, std::string tex1, 
		std::string tex2, std::string blendMap, float heightScale, 
		float yOffset);
	~Terrain();

	DWORD getNumTriangles();
	DWORD getNumVertices();

	float getWidth();
	float getDepth();

	void onLostDevice();
	void onResetDevice();

	// (x, z) relative to terrain's local space.
	float getHeight(float x, float z);

	void setDirToSunW(const D3DXVECTOR3& d);

	void draw(D3DXMATRIX& view, D3DXMATRIX& proj);

private:
	void buildGeometry();
	void buildSubGridMesh(RECT& R, VertexPNT* gridVerts); 
	void buildEffect();

private:
	Heightmap mHeightmap;

	std::vector<ID3DXMesh*> mSubGridMeshes;
	std::vector<AABB> mSubGridBndBoxes;

	DWORD mVertRows;
	DWORD mVertCols;

	float mWidth;
	float mDepth;

	float mDX;
	float mDZ;

	IDirect3DTexture9* mTex0;
	IDirect3DTexture9* mTex1;
	IDirect3DTexture9* mTex2;
	IDirect3DTexture9* mBlendMap;
	ID3DXEffect*       mFX;
	D3DXHANDLE         mhTech;
	D3DXHANDLE         mhViewProj;
	D3DXHANDLE         mhDirToSunW;
	D3DXHANDLE         mhTex0;
	D3DXHANDLE         mhTex1;
	D3DXHANDLE         mhTex2;
	D3DXHANDLE         mhBlendMap;
};

#endif // TERRAIN_H