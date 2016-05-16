//=============================================================================
// WaterDMap.cpp by Frank Luna (C) 2004 All Rights Reserved.
//=============================================================================

#include "WaterDMap.h"
#include "Vertex.h"
#include "Camera.h"

WaterDMap::WaterDMap(InitInfo& initInfo)
{
	mInitInfo = initInfo;

	mWidth = (initInfo.vertCols-1)*initInfo.dx;
	mDepth = (initInfo.vertRows-1)*initInfo.dz;

	mWaveNMapOffset0 = D3DXVECTOR2(0.0f, 0.0f);
	mWaveNMapOffset1 = D3DXVECTOR2(0.0f, 0.0f);

	mWaveDMapOffset0 = D3DXVECTOR2(0.0f, 0.0f);
	mWaveDMapOffset1 = D3DXVECTOR2(0.0f, 0.0f);

	DWORD numTris  = (initInfo.vertRows-1)*(initInfo.vertCols-1)*2;
	DWORD numVerts = initInfo.vertRows*initInfo.vertCols;


	//===============================================================
	// Allocate the mesh.


	D3DVERTEXELEMENT9 elems[MAX_FVF_DECL_SIZE];
	UINT numElems = 0;
	HR(WaterDMapVertex::Decl->GetDeclaration(elems, &numElems));
	HR(D3DXCreateMesh(numTris, numVerts, 
		D3DXMESH_MANAGED, elems, gd3dDevice, &mMesh));


	//===============================================================
	// Write the grid vertices and triangles to the mesh.

	WaterDMapVertex* v = 0;
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
			v[index].scaledTexC = D3DXVECTOR2((float)j/mInitInfo.vertCols, 
				                        (float)i/mInitInfo.vertRows)
										* initInfo.texScale;
			v[index].normalizedTexC = D3DXVECTOR2((float)j/mInitInfo.vertCols, 
				                        (float)i/mInitInfo.vertRows);
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

	int m = mInitInfo.vertRows;
	int n = mInitInfo.vertCols;
	HR(D3DXCreateTextureFromFile(gd3dDevice, initInfo.waveMapFilename0.c_str(), &mWaveMap0));
	HR(D3DXCreateTextureFromFile(gd3dDevice, initInfo.waveMapFilename1.c_str(), &mWaveMap1));
	HR(D3DXCreateTextureFromFileEx(gd3dDevice, initInfo.dmapFilename0.c_str(), m, n,
		1, 0, D3DFMT_R32F, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, 0, 0, &mDispMap0));
	HR(D3DXCreateTextureFromFileEx(gd3dDevice, initInfo.dmapFilename1.c_str(), m, n,
		1, 0, D3DFMT_R32F, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, 0, 0, &mDispMap1));

	buildFX();
}

WaterDMap::~WaterDMap()
{
	ReleaseCOM(mMesh);
	ReleaseCOM(mFX);
	ReleaseCOM(mWaveMap0);
	ReleaseCOM(mWaveMap1);
	ReleaseCOM(mDispMap0);
	ReleaseCOM(mDispMap1);
}

DWORD WaterDMap::getNumTriangles()
{
	return mMesh->GetNumFaces();
}

DWORD WaterDMap::getNumVertices()
{
	return mMesh->GetNumVertices();
}

void WaterDMap::onLostDevice()
{
	HR(mFX->OnLostDevice());
}

void WaterDMap::onResetDevice()
{
	HR(mFX->OnResetDevice());
}

void WaterDMap::update(float dt)
{
	// Update texture coordinate offsets.  These offsets are added to the
	// texture coordinates in the vertex shader to animate them.
	mWaveNMapOffset0 += mInitInfo.waveNMapVelocity0 * dt;
	mWaveNMapOffset1 += mInitInfo.waveNMapVelocity1 * dt;

	mWaveDMapOffset0 += mInitInfo.waveDMapVelocity0 * dt;
	mWaveDMapOffset1 += mInitInfo.waveDMapVelocity1 * dt;

	// Textures repeat every 1.0 unit, so reset back down to zero
	// so the coordinates do not grow too large.
	if(mWaveNMapOffset0.x >= 1.0f || mWaveNMapOffset0.x <= -1.0f)
		mWaveNMapOffset0.x = 0.0f;
	if(mWaveNMapOffset1.x >= 1.0f || mWaveNMapOffset1.x <= -1.0f)
		mWaveNMapOffset1.x = 0.0f;
	if(mWaveNMapOffset0.y >= 1.0f || mWaveNMapOffset0.y <= -1.0f)
		mWaveNMapOffset0.y = 0.0f;
	if(mWaveNMapOffset1.y >= 1.0f || mWaveNMapOffset1.y <= -1.0f)
		mWaveNMapOffset1.y = 0.0f;

	if(mWaveDMapOffset0.x >= 1.0f || mWaveDMapOffset0.x <= -1.0f)
		mWaveDMapOffset0.x = 0.0f;
	if(mWaveDMapOffset1.x >= 1.0f || mWaveDMapOffset1.x <= -1.0f)
		mWaveDMapOffset1.x = 0.0f;
	if(mWaveDMapOffset0.y >= 1.0f || mWaveDMapOffset0.y <= -1.0f)
		mWaveDMapOffset0.y = 0.0f;
	if(mWaveDMapOffset1.y >= 1.0f || mWaveDMapOffset1.y <= -1.0f)
		mWaveDMapOffset1.y = 0.0f;
}

void WaterDMap::draw()
{
	HR(mFX->SetMatrix(mhWVP, &(mInitInfo.toWorld*gCamera->viewProj())));
	HR(mFX->SetValue(mhEyePosW, &gCamera->pos(), sizeof(D3DXVECTOR3)));
	HR(mFX->SetValue(mhWaveNMapOffset0, &mWaveNMapOffset0, sizeof(D3DXVECTOR2)));
	HR(mFX->SetValue(mhWaveNMapOffset1, &mWaveNMapOffset1, sizeof(D3DXVECTOR2)));
	HR(mFX->SetValue(mhWaveDMapOffset0, &mWaveDMapOffset0, sizeof(D3DXVECTOR2)));
	HR(mFX->SetValue(mhWaveDMapOffset1, &mWaveDMapOffset1, sizeof(D3DXVECTOR2)));

	UINT numPasses = 0;
	HR(mFX->Begin(&numPasses, 0));
	HR(mFX->BeginPass(0));

	HR(mMesh->DrawSubset(0));

	HR(mFX->EndPass());
	HR(mFX->End());
}

void WaterDMap::buildFX()
{
	ID3DXBuffer* errors = 0;
	HR(D3DXCreateEffectFromFile(gd3dDevice, mInitInfo.fxFilename.c_str(),
		0, 0, D3DXSHADER_DEBUG, 0, &mFX, &errors));
	if( errors )
		MessageBox(0, (char*)errors->GetBufferPointer(), 0, 0);

 	mhTech            = mFX->GetTechniqueByName("WaterTech");
	mhWorld           = mFX->GetParameterByName(0, "gWorld");
	mhWorldInv        = mFX->GetParameterByName(0, "gWorldInv");
	mhWVP             = mFX->GetParameterByName(0, "gWVP");
	mhEyePosW         = mFX->GetParameterByName(0, "gEyePosW");
	mhLight           = mFX->GetParameterByName(0, "gLight");
	mhMtrl            = mFX->GetParameterByName(0, "gMtrl");
	mhWaveMap0        = mFX->GetParameterByName(0, "gWaveMap0");
    mhWaveMap1        = mFX->GetParameterByName(0, "gWaveMap1");
	mhWaveNMapOffset0 = mFX->GetParameterByName(0, "gWaveNMapOffset0");
	mhWaveNMapOffset1 = mFX->GetParameterByName(0, "gWaveNMapOffset1");
	mhWaveDMapOffset0 = mFX->GetParameterByName(0, "gWaveDMapOffset0");
	mhWaveDMapOffset1 = mFX->GetParameterByName(0, "gWaveDMapOffset1");
	mhWaveDispMap0    = mFX->GetParameterByName(0, "gWaveDispMap0");
	mhWaveDispMap1    = mFX->GetParameterByName(0, "gWaveDispMap1");
	mhScaleHeights    = mFX->GetParameterByName(0, "gScaleHeights");
	mhGridStepSizeL   = mFX->GetParameterByName(0, "gGridStepSizeL");


	// We don't need to set these every frame since they do not change.
	HR(mFX->SetMatrix(mhWorld, &mInitInfo.toWorld));
	D3DXMATRIX worldInv;
	D3DXMatrixInverse(&worldInv, 0, &mInitInfo.toWorld);
	HR(mFX->SetMatrix(mhWorldInv, &worldInv));
	HR(mFX->SetTechnique(mhTech));
	HR(mFX->SetTexture(mhWaveMap0, mWaveMap0));
	HR(mFX->SetTexture(mhWaveMap1, mWaveMap1));
	HR(mFX->SetTexture(mhWaveDispMap0, mDispMap0));
	HR(mFX->SetTexture(mhWaveDispMap1, mDispMap1));
	HR(mFX->SetValue(mhLight, &mInitInfo.dirLight, sizeof(DirLight)));
	HR(mFX->SetValue(mhMtrl, &mInitInfo.mtrl, sizeof(Mtrl)));
	HR(mFX->SetValue(mhScaleHeights, &mInitInfo.scaleHeights, sizeof(D3DXVECTOR2)));

	D3DXVECTOR2 stepSizes(mInitInfo.dx, mInitInfo.dz);
	HR(mFX->SetValue(mhGridStepSizeL, &stepSizes, sizeof(D3DXVECTOR2)));
}

