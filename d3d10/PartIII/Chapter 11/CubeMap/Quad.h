#ifndef QUAD_H
#define QUAD_H

#include "d3dUtil.h"

// Builds a quad in xz-plane.
class Quad
{
public:
	Quad();
	~Quad();

	void init(ID3D10Device* device, DWORD m, DWORD n, float dx);
	void draw();

private:
	DWORD mNumRows;
	DWORD mNumCols;

	DWORD mNumVertices;
	DWORD mNumFaces;

	ID3D10Device* md3dDevice;
	ID3D10Buffer* mVB;
	ID3D10Buffer* mIB;
};

#endif // QUAD_H
