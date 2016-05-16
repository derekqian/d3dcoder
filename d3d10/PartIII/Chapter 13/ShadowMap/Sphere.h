#ifndef SPHERE_H
#define SPHERE_H

#include "d3dUtil.h"
#include "Vertex.h"

class Sphere
{
public:
	Sphere();
	~Sphere();

	void init(ID3D10Device* device, float radius, UINT numSlices, UINT numStacks);
	void draw();

private:
	typedef std::vector<Vertex> VertexList;
	typedef std::vector<DWORD> IndexList;

	void buildStacks(VertexList& vertices, IndexList& indices);

private:
	float mRadius;
	UINT  mNumSlices;
	UINT  mNumStacks;

	DWORD mNumVertices;
	DWORD mNumFaces;

	ID3D10Device* md3dDevice;
	ID3D10Buffer* mVB;
	ID3D10Buffer* mIB;
};

#endif // SPHERE_H