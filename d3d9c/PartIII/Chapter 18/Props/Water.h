//=============================================================================
// Water.h by Frank Luna (C) 2004 All Rights Reserved.
//
// Note: If you want large bodies of water, then you should break
//       the water mesh into sub-grids like we did with the terrain
//       so that you can frustum cull.
//=============================================================================

#ifndef WATER_H
#define WATER_H

#include "d3dUtil.h"

class Water
{
public:
	Water(int m, int n, float dx, float dz, const D3DXMATRIX& toWorld);
	~Water();

	DWORD getNumTriangles();
	DWORD getNumVertices();

	void onLostDevice();
	void onResetDevice();

	void update(float dt);
	void draw();

private:
	void buildFX();

private:
	ID3DXMesh* mMesh;
	D3DXMATRIX mToWorld;
	ID3DXEffect* mFX;

	DWORD mVertRows;
	DWORD mVertCols;

	float mWidth;
	float mDepth;

	float mDX;
	float mDZ;

	D3DXHANDLE mhTech;
	D3DXHANDLE mhWVP;
	D3DXHANDLE mhWorld;
	D3DXHANDLE mhEyePosW;
};

#endif // WATER_H