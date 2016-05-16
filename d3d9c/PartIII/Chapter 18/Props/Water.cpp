//=============================================================================
// Water.cpp by Frank Luna (C) 2004 All Rights Reserved.
//=============================================================================

#include "Water.h"
#include "Vertex.h"
#include "Camera.h"

Water::Water(int m, int n, float dx, float dz, const D3DXMATRIX& toWorld)
{
	mVertRows = m;
	mVertCols = n;

	mWidth = (m-1)*dz;
	mDepth = (n-1)*dx;

	mDX = dx;
	mDZ = dz;

	mToWorld = toWorld;


	DWORD numTris  = (m-1)*(n-1)*2;
	DWORD numVerts = m*n;


	//===============================================================
	// Allocate the mesh.


	D3DVERTEXELEMENT9 elems[MAX_FVF_DECL_SIZE];
	UINT numElems = 0;
	HR(VertexPos::Decl->GetDeclaration(elems, &numElems));
	HR(D3DXCreateMesh(numTris, numVerts, 
		D3DXMESH_MANAGED, elems, gd3dDevice, &mMesh));


	//===============================================================
	// Write the grid vertices and triangles to the mesh.

	VertexPos* v = 0;
	HR(mMesh->LockVertexBuffer(0, (void**)&v));
	
	std::vector<D3DXVECTOR3> verts;
	std::vector<DWORD> indices;
	GenTriGrid(m, n, dx, dz, D3DXVECTOR3(0.0f, 0.0f, 0.0f), verts, indices);

	for(UINT i = 0; i < mMesh->GetNumVertices(); ++i)
	{
		v[i].pos = verts[i];
	}
	HR(mMesh->UnlockVertexBuffer());

	//===============================================================
	// Write triangle data so we can compute normals.

	WORD* indexBuffPtr = 0;
	HR(mMesh->LockIndexBuffer(0, (void**)&indexBuffPtr));
	DWORD* attBuff = 0;
	HR(mMesh->LockAttributeBuffer(0, &attBuff));
	for(UINT i = 0; i < mMesh->GetNumFaces(); ++i)
	{
		indexBuffPtr[i*3+0] = (WORD)indices[i*3+0];
		indexBuffPtr[i*3+1] = (WORD)indices[i*3+1];
		indexBuffPtr[i*3+2] = (WORD)indices[i*3+2];

		attBuff[i] = 0; // All in subset 0.
	}
	HR(mMesh->UnlockIndexBuffer());
	HR(mMesh->UnlockAttributeBuffer());

	//===============================================================
	// Optimize for the vertex cache and build attribute table.

	DWORD* adj = new DWORD[mMesh->GetNumFaces()*3];
	HR(mMesh->GenerateAdjacency(EPSILON, adj));
	HR(mMesh->OptimizeInplace(D3DXMESHOPT_VERTEXCACHE|D3DXMESHOPT_ATTRSORT,
		adj, 0, 0, 0));
	delete[] adj;


	//===============================================================
	// Build the water effect.

	buildFX();
}

Water::~Water()
{
	ReleaseCOM(mMesh);
	ReleaseCOM(mFX);
}

DWORD Water::getNumTriangles()
{
	return mMesh->GetNumFaces();
}

DWORD Water::getNumVertices()
{
	return mMesh->GetNumVertices();
}

void Water::onLostDevice()
{
	HR(mFX->OnLostDevice());
}

void Water::onResetDevice()
{
	HR(mFX->OnResetDevice());
}

void Water::update(float dt)
{
}

void Water::draw()
{
	HR(mFX->SetMatrix(mhWVP, &(mToWorld*gCamera->viewProj())));
	HR(mFX->SetValue(mhEyePosW, gCamera->pos(), sizeof(D3DXVECTOR3)));

	UINT numPasses = 0;
	HR(mFX->Begin(&numPasses, 0));
	HR(mFX->BeginPass(0));

	HR(mMesh->DrawSubset(0));

	HR(mFX->EndPass());
	HR(mFX->End());
}

void Water::buildFX()
{
	ID3DXBuffer* errors = 0;
	HR(D3DXCreateEffectFromFile(gd3dDevice, "Water.fx",
		0, 0, D3DXSHADER_DEBUG, 0, &mFX, &errors));
	if( errors )
		MessageBox(0, (char*)errors->GetBufferPointer(), 0, 0);

	mhTech    = mFX->GetTechniqueByName("WaterTech");
	mhWVP     = mFX->GetParameterByName(0, "gWVP");
	mhWorld   = mFX->GetParameterByName(0, "gWorld");
	mhEyePosW = mFX->GetParameterByName(0, "gEyePosW");

	// We don't need to set these every frame since they do not change.
	HR(mFX->SetTechnique(mhTech));
	HR(mFX->SetMatrix(mhWorld, &mToWorld));
}