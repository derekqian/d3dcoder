#ifndef CYLINDER_H
#define CYLINDER_H

#include "d3dUtil.h"
#include "Vertex.h"

class Cylinder
{
public:
	Cylinder();
	~Cylinder();

	void init(ID3D10Device* device, float topRadius, float bottomRadius, 
		float height, UINT numSlices, UINT numStacks);

	void draw();
 
private:
	typedef std::vector<Vertex> VertexList;
	typedef std::vector<DWORD> IndexList;

	void buildStacks(VertexList& vertices, IndexList& indices);
	void buildBottomCap(VertexList& vertices, IndexList& indices);
	void buildTopCap(VertexList& vertices, IndexList& indices);
private:
	float mTopRadius;
	float mBottomRadius;
	float mHeight;
	UINT  mNumSlices;
	UINT  mNumStacks;

 	DWORD mNumVertices;
	DWORD mNumFaces;

	ID3D10Device* md3dDevice;
	ID3D10Buffer* mVB;
	ID3D10Buffer* mIB;
};

#endif // CYLINDER_H