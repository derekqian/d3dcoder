#include "Quad.h"
#include "Vertex.h"

Quad::Quad()
: mNumVertices(0), mNumFaces(0), md3dDevice(0), mVB(0), mIB(0)
{
}

Quad::~Quad()
{
	ReleaseCOM(mVB);
	ReleaseCOM(mIB);
}

void Quad::init(ID3D10Device* device, DWORD m, DWORD n, float dx)
{
	md3dDevice = device;

	mNumRows  = m;
	mNumCols  = n;

	mNumVertices = m*n;
	mNumFaces    = (m-1)*(n-1)*2;


	// Create the geometry and fill the vertex buffer. 

	std::vector<Vertex> vertices(mNumVertices);
	float halfWidth = (n-1)*dx*0.5f;
	float halfDepth = (m-1)*dx*0.5f;

	float du = 1.0f / (n-1);
	float dv = 1.0f / (m-1);
	for(DWORD i = 0; i < m; ++i)
	{
		float z = halfDepth - i*dx;
		for(DWORD j = 0; j < n; ++j)
		{
			float x = -halfWidth + j*dx;

			vertices[i*n+j].pos    = D3DXVECTOR3(x, 0.0f, z);
			vertices[i*n+j].normal = D3DXVECTOR3(0.0f, 1.0f, 0.0f);

			// Stretch texture over grid.
			vertices[i*n+j].texC.x = j*du;
			vertices[i*n+j].texC.y = i*dv;
		}
	}
 
    D3D10_BUFFER_DESC vbd;
    vbd.Usage = D3D10_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(Vertex) * mNumVertices;
    vbd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
	D3D10_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = &vertices[0];
    HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mVB));


	// Create the index buffer. 

	std::vector<DWORD> indices(mNumFaces*3); // 3 indices per face

	// Iterate over each quad and compute indices.
	int k = 0;
	for(DWORD i = 0; i < m-1; ++i)
	{
		for(DWORD j = 0; j < n-1; ++j)
		{
			indices[k]   = i*n+j;
			indices[k+1] = i*n+j+1;
			indices[k+2] = (i+1)*n+j;

			indices[k+3] = (i+1)*n+j;
			indices[k+4] = i*n+j+1;
			indices[k+5] = (i+1)*n+j+1;

			k += 6; // next quad
		}
	}

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

void Quad::draw()
{
	UINT stride = sizeof(Vertex);
    UINT offset = 0;
    md3dDevice->IASetVertexBuffers(0, 1, &mVB, &stride, &offset);
	md3dDevice->IASetIndexBuffer(mIB, DXGI_FORMAT_R32_UINT, 0);
	md3dDevice->DrawIndexed(mNumFaces*3, 0, 0);
}