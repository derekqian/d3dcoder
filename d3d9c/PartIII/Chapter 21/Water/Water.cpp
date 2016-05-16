//=============================================================================
// Water.cpp by Frank Luna (C) 2004 All Rights Reserved.
//=============================================================================

#include "Water.h"
#include "Vertex.h"
#include "Camera.h"

Water::Water(InitInfo& initInfo)
{
	mInitInfo = initInfo;

	mWidth = (initInfo.vertCols-1)*initInfo.dx;
	mDepth = (initInfo.vertRows-1)*initInfo.dz;

	mWaveMapOffset0 = D3DXVECTOR2(0.0f, 0.0f);
	mWaveMapOffset1 = D3DXVECTOR2(0.0f, 0.0f);

	DWORD numTris  = (initInfo.vertRows-1)*(initInfo.vertCols-1)*2;
	DWORD numVerts = initInfo.vertRows*initInfo.vertCols;


	//===============================================================
	// Allocate the mesh.


	D3DVERTEXELEMENT9 elems[MAX_FVF_DECL_SIZE];
	UINT numElems = 0;
	HR(VertexPT::Decl->GetDeclaration(elems, &numElems));
	HR(D3DXCreateMesh(numTris, numVerts, 
		D3DXMESH_MANAGED, elems, gd3dDevice, &mMesh));


	//===============================================================
	// Write the grid vertices and triangles to the mesh.

	VertexPT* v = 0;
	HR(mMesh->LockVertexBuffer(0, (void**)&v));
	
	std::vector<D3DXVECTOR3> verts;
	std::vector<DWORD> indices;
	GenTriGrid(mInitInfo.vertRows, mInitInfo.vertCols, mInitInfo.dx, 
		mInitInfo.dz, D3DXVECTOR3(0.0f, 0.0f, 0.0f), verts, indices);

	for(int i = 0; i < mInitInfo.vertRows; ++i)
	{
		for(int j = 0; j < mInitInfo.vertCols; ++j)
		{
			DWORD index   = i * mInitInfo.vertCols + j;
			v[index].pos  = verts[index];
			v[index].tex0 = D3DXVECTOR2((float)j/mInitInfo.vertCols, 
				                        (float)i/mInitInfo.vertRows)
										* initInfo.texScale;
		}
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
	// Create textures/effect.

	HR(D3DXCreateTextureFromFile(gd3dDevice, initInfo.waveMapFilename0.c_str(), &mWaveMap0));
	HR(D3DXCreateTextureFromFile(gd3dDevice, initInfo.waveMapFilename1.c_str(), &mWaveMap1));
	buildFX();
}

Water::~Water()
{
	ReleaseCOM(mMesh);
	ReleaseCOM(mFX);
	ReleaseCOM(mWaveMap0);
	ReleaseCOM(mWaveMap1);
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
	// Update texture coordinate offsets.  These offsets are added to the
	// texture coordinates in the vertex shader to animate them.
	mWaveMapOffset0 += mInitInfo.waveMapVelocity0 * dt;
	mWaveMapOffset1 += mInitInfo.waveMapVelocity1 * dt;

	// Textures repeat every 1.0 unit, so reset back down to zero
	// so the coordinates do not grow too large.
	if(mWaveMapOffset0.x >= 1.0f || mWaveMapOffset0.x <= -1.0f)
		mWaveMapOffset0.x = 0.0f;
	if(mWaveMapOffset1.x >= 1.0f || mWaveMapOffset1.x <= -1.0f)
		mWaveMapOffset1.x = 0.0f;
	if(mWaveMapOffset0.y >= 1.0f || mWaveMapOffset0.y <= -1.0f)
		mWaveMapOffset0.y = 0.0f;
	if(mWaveMapOffset1.y >= 1.0f || mWaveMapOffset1.y <= -1.0f)
		mWaveMapOffset1.y = 0.0f;
}

void Water::draw()
{
	HR(mFX->SetMatrix(mhWVP, &(mInitInfo.toWorld*gCamera->viewProj())));
	HR(mFX->SetValue(mhEyePosW, &gCamera->pos(), sizeof(D3DXVECTOR3)));
	HR(mFX->SetValue(mhWaveMapOffset0, &mWaveMapOffset0, sizeof(D3DXVECTOR2)));
	HR(mFX->SetValue(mhWaveMapOffset1, &mWaveMapOffset1, sizeof(D3DXVECTOR2)));

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

 	mhTech           = mFX->GetTechniqueByName("WaterTech");
	mhWorld          = mFX->GetParameterByName(0, "gWorld");
	mhWorldInv       = mFX->GetParameterByName(0, "gWorldInv");
	mhWVP            = mFX->GetParameterByName(0, "gWVP");
	mhEyePosW        = mFX->GetParameterByName(0, "gEyePosW");
	mhLight          = mFX->GetParameterByName(0, "gLight");
	mhMtrl           = mFX->GetParameterByName(0, "gMtrl");
	mhWaveMap0       = mFX->GetParameterByName(0, "gWaveMap0");
    mhWaveMap1       = mFX->GetParameterByName(0, "gWaveMap1");
	mhWaveMapOffset0 = mFX->GetParameterByName(0, "gWaveMapOffset0");
	mhWaveMapOffset1 = mFX->GetParameterByName(0, "gWaveMapOffset1");
	mhEnvMap         = mFX->GetParameterByName(0, "gEnvMap");


	// We don't need to set these every frame since they do not change.
	HR(mFX->SetMatrix(mhWorld, &mInitInfo.toWorld));
	D3DXMATRIX worldInv;
	D3DXMatrixInverse(&worldInv, 0, &mInitInfo.toWorld);
	HR(mFX->SetMatrix(mhWorldInv, &worldInv));
	HR(mFX->SetTechnique(mhTech));
	HR(mFX->SetTexture(mhWaveMap0, mWaveMap0));
	HR(mFX->SetTexture(mhWaveMap1, mWaveMap1));
	HR(mFX->SetValue(mhLight, &mInitInfo.dirLight, sizeof(DirLight)));
	HR(mFX->SetValue(mhMtrl, &mInitInfo.mtrl, sizeof(Mtrl)));
}

void Water::setEnvMap(IDirect3DCubeTexture9* envMap)
{
	HR(mFX->SetTexture(mhEnvMap, envMap));
}