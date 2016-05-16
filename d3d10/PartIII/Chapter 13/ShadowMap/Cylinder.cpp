#include "Cylinder.h"
#include "Vertex.h"

Cylinder::Cylinder()
: mNumVertices(0), mNumFaces(0), md3dDevice(0), mVB(0), mIB(0),
  mTopRadius(0.0f), mBottomRadius(0.0f), mHeight(0.0f), mNumSlices(0), mNumStacks(0)
{
}

Cylinder::~Cylinder()
{
	ReleaseCOM(mVB);
	ReleaseCOM(mIB);
}

void Cylinder::init(ID3D10Device* device, float topRadius, float bottomRadius, 
		            float height, UINT numSlices, UINT numStacks)
{
	md3dDevice = device;

	mTopRadius    = topRadius;
	mBottomRadius = bottomRadius;
	mHeight       = height;
	mNumSlices    = numSlices;
	mNumStacks    = numStacks;

 	
	std::vector<Vertex> vertices;
	std::vector<DWORD> indices;

	buildStacks(vertices, indices);
	buildTopCap(vertices, indices);
	buildBottomCap(vertices, indices);

	
	mNumVertices = (UINT)vertices.size();
	mNumFaces    = (UINT)indices.size()/3;

	D3D10_BUFFER_DESC vbd;
    vbd.Usage = D3D10_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(Vertex) * mNumVertices;
    vbd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D10_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = &vertices[0];
    HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mVB));

	D3D10_BUFFER_DESC ibd;
    ibd.Usage = D3D10_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(DWORD) * mNumFaces*3;
    ibd.BindFlags = D3D10_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D10_SUBRESOURCE_DATA iinitData;
    iinitData.pSysMem = &indices[0];
    HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mIB));
}

void Cylinder::draw()
{
	UINT stride = sizeof(Vertex);
    UINT offset = 0;
    md3dDevice->IASetVertexBuffers(0, 1, &mVB, &stride, &offset);
	md3dDevice->IASetIndexBuffer(mIB, DXGI_FORMAT_R32_UINT, 0);
	md3dDevice->DrawIndexed(mNumFaces*3, 0, 0);
}

void Cylinder::buildStacks(VertexList& vertices, IndexList& indices)
{
	float stackHeight = mHeight / mNumStacks;

	// Amount to increment radius as we move up each stack level from bottom to top.
	float radiusStep = (mTopRadius - mBottomRadius) / mNumStacks;

	UINT numRings = mNumStacks+1;

	// Compute vertices for each stack ring.
	for(UINT i = 0; i < numRings; ++i)
	{
		float y = -0.5f*mHeight + i*stackHeight;
		float r = mBottomRadius + i*radiusStep;

		// Height and radius of next ring up.
		float y_next = -0.5f*mHeight + (i+1)*stackHeight;
		float r_next = mBottomRadius + (i+1)*radiusStep;
	 
		// vertices of ring
		float dTheta = 2.0f*PI/mNumSlices;
		for(UINT j = 0; j <= mNumSlices; ++j)
		{
			float c = cosf(j*dTheta);
			float s = sinf(j*dTheta);

			float u = (float)j/mNumSlices;
			float v = 1.0f - (float)i/mNumStacks;

			// Partial derivative in theta direction to get tangent vector (this is a unit vector).
			D3DXVECTOR3 T(-s, 0.0f, c);

			// Compute tangent vector down the slope of the cone (if the top/bottom 
			// radii differ then we get a cone and not a true cylinder).
			D3DXVECTOR3 P(r*c, y, r*s);
			D3DXVECTOR3 P_next(r_next*c, y_next, r_next*s);
			D3DXVECTOR3 B = P - P_next;
			D3DXVec3Normalize(&B, &B);

			D3DXVECTOR3 N;
			D3DXVec3Cross(&N, &T, &B);
			D3DXVec3Normalize(&N, &N);

			vertices.push_back( Vertex(P.x, P.y, P.z, T.x, T.y, T.z, N.x, N.y, N.z, u, v) );
		}
	}

	UINT numRingVertices = mNumSlices+1;

	// Compute indices for each stack.
	for(UINT i = 0; i < mNumStacks; ++i)
	{
		for(UINT j = 0; j < mNumSlices; ++j)
		{
			indices.push_back(i*numRingVertices + j);
			indices.push_back((i+1)*numRingVertices + j);
			indices.push_back((i+1)*numRingVertices + j+1);

			indices.push_back(i*numRingVertices + j);
			indices.push_back((i+1)*numRingVertices + j+1);
			indices.push_back(i*numRingVertices + j+1);
		}
	}
}
 
void Cylinder::buildBottomCap(VertexList& vertices, IndexList& indices)
{
	UINT baseIndex = (UINT)vertices.size();

	// Duplicate cap vertices because the texture coordinates and normals differ.
	float y = -0.5f*mHeight;

	// vertices of ring
	float dTheta = 2.0f*PI/mNumSlices;
	for(UINT i = 0; i <= mNumSlices; ++i)
	{
		float x = mBottomRadius*cosf(i*dTheta);
		float z = mBottomRadius*sinf(i*dTheta);

		// Map [-1,1]-->[0,1] for planar texture coordinates.
		float u = +0.5f*x/mBottomRadius + 0.5f;
		float v = -0.5f*z/mBottomRadius + 0.5f;

		vertices.push_back( Vertex(x, y, z, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, u, v) );
	}

	// cap center vertex
	vertices.push_back( Vertex(0.0f, y, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.5f, 0.5f) );

	// index of center vertex
	UINT centerIndex = (UINT)vertices.size()-1;

	for(UINT i = 0; i < mNumSlices; ++i)
	{
		indices.push_back(centerIndex);
		indices.push_back(baseIndex + i);
		indices.push_back(baseIndex + i+1);
	}
}

void Cylinder::buildTopCap(VertexList& vertices, IndexList& indices)
{
	UINT baseIndex = (UINT)vertices.size();

	// Duplicate cap vertices because the texture coordinates and normals differ.
	float y = 0.5f*mHeight;

	// vertices of ring
	float dTheta = 2.0f*PI/mNumSlices;
	for(UINT i = 0; i <= mNumSlices; ++i)
	{
		float x = mTopRadius*cosf(i*dTheta);
		float z = mTopRadius*sinf(i*dTheta);

		// Map [-1,1]-->[0,1] for planar texture coordinates.
		float u = +0.5f*x/mTopRadius + 0.5f;
		float v = -0.5f*z/mTopRadius + 0.5f;

		vertices.push_back( Vertex(x, y, z, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, u, v) );
	}

	// cap center vertex
	vertices.push_back( Vertex(0.0f, y, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f) );

	// index of center vertex
	UINT centerIndex = (UINT)vertices.size()-1;

	for(UINT i = 0; i < mNumSlices; ++i)
	{
		indices.push_back(centerIndex);
		indices.push_back(baseIndex + i+1);
		indices.push_back(baseIndex + i);
	}
}