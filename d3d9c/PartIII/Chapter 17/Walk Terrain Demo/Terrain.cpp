//=============================================================================
// Terrain.cpp by Frank Luna (C) 2004 All Rights Reserved.
//=============================================================================

#include "Terrain.h"
#include "Camera.h"

struct SubGrid
{
	const static int NUM_ROWS  = 33;
	const static int NUM_COLS  = 33;
	const static int NUM_TRIS  = (NUM_ROWS-1)*(NUM_COLS-1)*2;
	const static int NUM_VERTS = NUM_ROWS*NUM_COLS;
};


Terrain::Terrain(UINT vertRows, UINT vertCols, float dx, float dz, 
		std::string heightmap, std::string tex0, std::string tex1, 
		std::string tex2, std::string blendMap, float heightScale, 
		float yOffset)
{
	mVertRows = vertRows;
	mVertCols = vertCols;

	mDX = dx;
	mDZ = dz;

	mWidth = (mVertCols-1)*mDX;
	mDepth = (mVertRows-1)*mDZ;

	mHeightmap.loadRAW(vertRows, vertCols, heightmap, heightScale, yOffset);

	HR(D3DXCreateTextureFromFile(gd3dDevice, tex0.c_str(), &mTex0));
	HR(D3DXCreateTextureFromFile(gd3dDevice, tex1.c_str(), &mTex1));
	HR(D3DXCreateTextureFromFile(gd3dDevice, tex2.c_str(), &mTex2));
	HR(D3DXCreateTextureFromFile(gd3dDevice, blendMap.c_str(), &mBlendMap));

	buildGeometry();
	buildEffect();
}

Terrain::~Terrain()
{
	for(UINT i = 0; i < mSubGridMeshes.size(); ++i)
		ReleaseCOM(mSubGridMeshes[i]);

	ReleaseCOM(mFX);
	ReleaseCOM(mTex0);
	ReleaseCOM(mTex1);
	ReleaseCOM(mTex2);
	ReleaseCOM(mBlendMap);
}

DWORD Terrain::getNumTriangles()
{
	return (DWORD)mSubGridMeshes.size()*mSubGridMeshes[0]->GetNumFaces();
}

DWORD Terrain::getNumVertices()
{
	return (DWORD)mSubGridMeshes.size()*mSubGridMeshes[0]->GetNumVertices();
}

float Terrain::getWidth()
{
	return mWidth;
}

float Terrain::getDepth()
{
	return mDepth;
}

void Terrain::onLostDevice()
{
	HR(mFX->OnLostDevice());
}

void Terrain::onResetDevice()
{
	HR(mFX->OnResetDevice());
}

float Terrain::getHeight(float x, float z)
{
	// Transform from terrain local space to "cell" space.
	float c = (x + 0.5f*mWidth) /  mDX;
	float d = (z - 0.5f*mDepth) / -mDZ;

	// Get the row and column we are in.
	int row = (int)floorf(d);
	int col = (int)floorf(c);

	// Grab the heights of the cell we are in.
	// A*--*B
	//  | /|
	//  |/ |
	// C*--*D
	float A = mHeightmap(row, col);
	float B = mHeightmap(row, col+1);
	float C = mHeightmap(row+1, col);
	float D = mHeightmap(row+1, col+1);

	// Where we are relative to the cell.
	float s = c - (float)col;
	float t = d - (float)row;

	// If upper triangle ABC.
	if(t < 1.0f - s)
	{
		float uy = B - A;
		float vy = C - A;
		return A + s*uy + t*vy;
	}
	else // lower triangle DCB.
	{
		float uy = C - D;
		float vy = B - D;
		return D + (1.0f-s)*uy + (1.0f-t)*vy;
	}
}

void Terrain::setDirToSunW(const D3DXVECTOR3& d)
{
	HR(mFX->SetValue(mhDirToSunW, &d, sizeof(D3DXVECTOR3)));
}

void Terrain::draw()
{
	// TODO: Sort front to back, and frustum cull subgrids.

	HR(mFX->SetMatrix(mhViewProj, &gCamera->viewProj()));
	HR(mFX->SetTechnique(mhTech));
	UINT numPasses = 0;
	HR(mFX->Begin(&numPasses, 0));
	HR(mFX->BeginPass(0));

	for(UINT i = 0; i < mSubGridMeshes.size(); ++i)
		HR(mSubGridMeshes[i]->DrawSubset(0));

	HR(mFX->EndPass());
	HR(mFX->End());
}

void Terrain::buildGeometry()
{
	//===============================================================
	// Create one large mesh for the grid in system memory.

	DWORD numTris  = (mVertRows-1)*(mVertCols-1)*2;
	DWORD numVerts = mVertRows*mVertCols;

	ID3DXMesh* mesh = 0;
	D3DVERTEXELEMENT9 elems[MAX_FVF_DECL_SIZE];
	UINT numElems = 0;
	HR(VertexPNT::Decl->GetDeclaration(elems, &numElems));
	HR(D3DXCreateMesh(numTris, numVerts, 
		D3DXMESH_SYSTEMMEM|D3DXMESH_32BIT, elems, gd3dDevice, &mesh));


	//===============================================================
	// Write the grid vertices and triangles to the mesh.

	VertexPNT* v = 0;
	HR(mesh->LockVertexBuffer(0, (void**)&v));
	
	std::vector<D3DXVECTOR3> verts;
	std::vector<DWORD> indices;
	GenTriGrid(mVertRows, mVertCols, mDX, mDZ, D3DXVECTOR3(0.0f, 0.0f, 0.0f), verts, indices);

	float w = mWidth;
	float d = mDepth;
	for(UINT i = 0; i < mesh->GetNumVertices(); ++i)
	{
		// We store the grid vertices in a linear array, but we can
		// convert the linear array index to an (r, c) matrix index.
		int r = i / mVertCols;
		int c = i % mVertCols;

		v[i].pos   = verts[i];
		v[i].pos.y = mHeightmap(r, c);

		v[i].tex0.x = (v[i].pos.x + (0.5f*w)) / w;
		v[i].tex0.y = (v[i].pos.z - (0.5f*d)) / -d;
	}

	// Write triangle data so we can compute normals.

	DWORD* indexBuffPtr = 0;
	HR(mesh->LockIndexBuffer(0, (void**)&indexBuffPtr));
	for(UINT i = 0; i < mesh->GetNumFaces(); ++i)
	{
		indexBuffPtr[i*3+0] = indices[i*3+0];
		indexBuffPtr[i*3+1] = indices[i*3+1];
		indexBuffPtr[i*3+2] = indices[i*3+2];
	}
	HR(mesh->UnlockIndexBuffer());

	// Compute Vertex Normals.
	HR(D3DXComputeNormals(mesh, 0));

	
	//===============================================================
	// Now break the grid up into subgrid meshes.

	// Find out the number of subgrids we'll have.  For example, if
	// m = 513, n = 257, SUBGRID_VERT_ROWS = SUBGRID_VERT_COLS = 33,
	// then subGridRows = 512/32 = 16 and sibGridCols = 256/32 = 8.
	int subGridRows = (mVertRows-1) / (SubGrid::NUM_ROWS-1);
	int subGridCols = (mVertCols-1) / (SubGrid::NUM_COLS-1);

	for(int r = 0; r < subGridRows; ++r)
	{
		for(int c = 0; c < subGridCols; ++c)
		{
			// Rectangle that indicates (via matrix indices ij) the
			// portion of grid vertices to use for this subgrid.
			RECT R = 
			{
					c * (SubGrid::NUM_COLS-1),
					r * (SubGrid::NUM_ROWS-1),
				(c+1) * (SubGrid::NUM_COLS-1),
				(r+1) * (SubGrid::NUM_ROWS-1)
			};

			buildSubGridMesh(R, v); 
		}
	}

	HR(mesh->UnlockVertexBuffer());

	ReleaseCOM(mesh); // Done with global mesh.
}

void Terrain::buildSubGridMesh(RECT& R, VertexPNT* gridVerts)
{
	//===============================================================
	// Get indices for subgrid (we don't use the verts here--the verts
	// are given by the parameter gridVerts).

	std::vector<D3DXVECTOR3> tempVerts;
	std::vector<DWORD> tempIndices;
	GenTriGrid(SubGrid::NUM_ROWS, SubGrid::NUM_COLS, mDX, mDZ, 
		D3DXVECTOR3(0.0f, 0.0f, 0.0f), tempVerts, tempIndices);

	ID3DXMesh* subMesh = 0;
	D3DVERTEXELEMENT9 elems[MAX_FVF_DECL_SIZE];
	UINT numElems = 0;
	HR(VertexPNT::Decl->GetDeclaration(elems, &numElems));
	HR(D3DXCreateMesh(SubGrid::NUM_TRIS, SubGrid::NUM_VERTS, 
		D3DXMESH_MANAGED, elems, gd3dDevice, &subMesh));


	//===============================================================
	// Build Vertex Buffer.  Copy rectangle of vertices from the
	// grid into the subgrid structure.
	VertexPNT* v = 0;
	HR(subMesh->LockVertexBuffer(0, (void**)&v));
	int k = 0;
	for(int i = R.top; i <= R.bottom; ++i)
	{
		for(int j = R.left; j <= R.right; ++j)
		{
			v[k++] = gridVerts[i*mVertCols+j];
		}
	}

	//===============================================================
	// Compute the bounding box before unlocking the vertex buffer.
	AABB bndBox;
	HR(D3DXComputeBoundingBox((D3DXVECTOR3*)v, subMesh->GetNumVertices(), 
		sizeof(VertexPNT), &bndBox.minPt, &bndBox.maxPt));

	HR(subMesh->UnlockVertexBuffer());


	//===============================================================
	// Build Index and Attribute Buffer.
	WORD* indices  = 0;
	DWORD* attBuff = 0;
	HR(subMesh->LockIndexBuffer(0, (void**)&indices));
	HR(subMesh->LockAttributeBuffer(0, &attBuff));
	for(int i = 0; i < SubGrid::NUM_TRIS; ++i)
	{
		indices[i*3+0] = (WORD)tempIndices[i*3+0];
		indices[i*3+1] = (WORD)tempIndices[i*3+1];
		indices[i*3+2] = (WORD)tempIndices[i*3+2];

		attBuff[i] = 0; // All in subset 0.
	}
	HR(subMesh->UnlockIndexBuffer());
	HR(subMesh->UnlockAttributeBuffer());


	//===============================================================
	// Optimize for the vertex cache and build attribute table.
	DWORD* adj = new DWORD[subMesh->GetNumFaces()*3];
	HR(subMesh->GenerateAdjacency(EPSILON, adj));
	HR(subMesh->OptimizeInplace(D3DXMESHOPT_VERTEXCACHE|D3DXMESHOPT_ATTRSORT,
		adj, 0, 0, 0));
	delete[] adj;

	
	//===============================================================
	// Save the mesh and bounding box.
	mSubGridMeshes.push_back(subMesh);
	mSubGridBndBoxes.push_back(bndBox);
}

void Terrain::buildEffect()
{
	ID3DXBuffer* errors = 0;
	HR(D3DXCreateEffectFromFile(gd3dDevice, "Terrain.fx",
		0, 0, D3DXSHADER_DEBUG, 0, &mFX, &errors));
	if( errors )
		MessageBox(0, (char*)errors->GetBufferPointer(), 0, 0);

	mhTech      = mFX->GetTechniqueByName("TerrainTech");
	mhViewProj  = mFX->GetParameterByName(0, "gViewProj");
	mhDirToSunW = mFX->GetParameterByName(0, "gDirToSunW");
	mhTex0      = mFX->GetParameterByName(0, "gTex0");
	mhTex1      = mFX->GetParameterByName(0, "gTex1");
	mhTex2      = mFX->GetParameterByName(0, "gTex2");
	mhBlendMap  = mFX->GetParameterByName(0, "gBlendMap");

	HR(mFX->SetTexture(mhTex0, mTex0));
	HR(mFX->SetTexture(mhTex1, mTex1));
	HR(mFX->SetTexture(mhTex2, mTex2));
	HR(mFX->SetTexture(mhBlendMap, mBlendMap));
}