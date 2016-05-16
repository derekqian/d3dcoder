//=============================================================================
// BasicTerrainDemo.cpp by Frank Luna (C) 2005 All Rights Reserved.
//
// Builds off the Multi-Tex demo from chapter 10 by using a heightmap
// to adjust the grid heights to create hills and valleys.
//
// Controls: Use mouse to orbit and zoom; use the 'W' and 'S' keys to 
//           alter the height of the camera.
//=============================================================================

#include "d3dApp.h"
#include "DirectInput.h"
#include <crtdbg.h>
#include "GfxStats.h"
#include <list>
#include "Vertex.h"
#include "Heightmap.h"

class BasicTerrainDemo : public D3DApp
{
public:
	BasicTerrainDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP);
	~BasicTerrainDemo();

	bool checkDeviceCaps();
	void onLostDevice();
	void onResetDevice();
	void updateScene(float dt);
	void drawScene();

	// Helper methods
	void buildGridGeometry();
	void buildFX();
	void buildViewMtx();
	void buildProjMtx();

private:
	GfxStats* mGfxStats;

	Heightmap mHeightmap;

	ID3DXMesh* mTerrainMesh;

	IDirect3DTexture9* mTex0;
	IDirect3DTexture9* mTex1;
	IDirect3DTexture9* mTex2;
	IDirect3DTexture9* mBlendMap;

	ID3DXEffect* mFX;
	D3DXHANDLE   mhTech;
	D3DXHANDLE   mhViewProj;
	D3DXHANDLE   mhDirToSunW;
	D3DXHANDLE   mhTex0;
	D3DXHANDLE   mhTex1;
	D3DXHANDLE   mhTex2;
	D3DXHANDLE   mhBlendMap;

	float mCameraRotationY;
	float mCameraRadius;
	float mCameraHeight;

	D3DXMATRIX mWorld;
	D3DXMATRIX mView;
	D3DXMATRIX mProj;
};


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
	#if defined(DEBUG) | defined(_DEBUG)
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	#endif

	BasicTerrainDemo app(hInstance, "Basic Terrain Demo", D3DDEVTYPE_HAL, D3DCREATE_HARDWARE_VERTEXPROCESSING);
	gd3dApp = &app;

	DirectInput di(DISCL_NONEXCLUSIVE|DISCL_FOREGROUND, DISCL_NONEXCLUSIVE|DISCL_FOREGROUND);
	gDInput = &di;

    return gd3dApp->run();
}

BasicTerrainDemo::BasicTerrainDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP)
: D3DApp(hInstance, winCaption, devType, requestedVP)
{
	if(!checkDeviceCaps())
	{
		MessageBox(0, "checkDeviceCaps() Failed", 0, 0);
		PostQuitMessage(0);
	}

	InitAllVertexDeclarations();

	mGfxStats = new GfxStats();

	mCameraRadius    = 80.0f;
	mCameraRotationY = 1.2 * D3DX_PI;
	mCameraHeight    = 40.0f;

	D3DXMatrixIdentity(&mWorld);

	mHeightmap.loadRAW(129, 129, "heightmap17_129.raw", 0.25f, 0.0f);

	// Load textures from file.
	HR(D3DXCreateTextureFromFile(gd3dDevice, "grass.dds", &mTex0));
	HR(D3DXCreateTextureFromFile(gd3dDevice, "dirt.dds", &mTex1));
	HR(D3DXCreateTextureFromFile(gd3dDevice, "stone.dds", &mTex2));
	HR(D3DXCreateTextureFromFile(gd3dDevice, "blend_hm17.dds", &mBlendMap));
	
	buildGridGeometry();
	mGfxStats->addVertices(mTerrainMesh->GetNumVertices());
	mGfxStats->addTriangles(mTerrainMesh->GetNumFaces());

	buildFX();

	onResetDevice();
}

BasicTerrainDemo::~BasicTerrainDemo()
{
	delete mGfxStats;
	ReleaseCOM(mTerrainMesh);
	ReleaseCOM(mTex0);
	ReleaseCOM(mTex1);
	ReleaseCOM(mTex2);
	ReleaseCOM(mBlendMap);
	ReleaseCOM(mFX);

	DestroyAllVertexDeclarations();
}

bool BasicTerrainDemo::checkDeviceCaps()
{
	D3DCAPS9 caps;
	HR(gd3dDevice->GetDeviceCaps(&caps));

	// Check for vertex shader version 2.0 support.
	if( caps.VertexShaderVersion < D3DVS_VERSION(2, 0) )
		return false;

	// Check for pixel shader version 2.0 support.
	if( caps.PixelShaderVersion < D3DPS_VERSION(2, 0) )
		return false;

	return true;
}

void BasicTerrainDemo::onLostDevice()
{
	mGfxStats->onLostDevice();
	HR(mFX->OnLostDevice());
}

void BasicTerrainDemo::onResetDevice()
{
	mGfxStats->onResetDevice();
	HR(mFX->OnResetDevice());


	// The aspect ratio depends on the backbuffer dimensions, which can 
	// possibly change after a reset.  So rebuild the projection matrix.
	buildProjMtx();
}

void BasicTerrainDemo::updateScene(float dt)
{
	mGfxStats->update(dt);

	// Get snapshot of input devices.
	gDInput->poll();

	// Check input.
	if( gDInput->keyDown(DIK_W) )	 
		mCameraHeight   += 25.0f * dt;
	if( gDInput->keyDown(DIK_S) )	 
		mCameraHeight   -= 25.0f * dt;

	// Divide by 50 to make mouse less sensitive. 
	mCameraRotationY += gDInput->mouseDX() / 100.0f;
	mCameraRadius    += gDInput->mouseDY() / 25.0f;

	// If we rotate over 360 degrees, just roll back to 0
	if( fabsf(mCameraRotationY) >= 2.0f * D3DX_PI ) 
		mCameraRotationY = 0.0f;

	// Don't let radius get too small.
	if( mCameraRadius < 5.0f )
		mCameraRadius = 5.0f;

	// The camera position/orientation relative to world space can 
	// change every frame based on input, so we need to rebuild the
	// view matrix every frame with the latest changes.
	buildViewMtx();
}

void BasicTerrainDemo::drawScene()
{
	// Clear the backbuffer and depth buffer.
	HR(gd3dDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffeeeeee, 1.0f, 0));

	HR(gd3dDevice->BeginScene());

	// Setup the rendering FX
	HR(mFX->SetMatrix(mhViewProj, &(mView*mProj)));
	HR(mFX->SetTechnique(mhTech));
	UINT numPasses = 0;
	HR(mFX->Begin(&numPasses, 0));
	HR(mFX->BeginPass(0));

	HR(mTerrainMesh->DrawSubset(0));

	HR(mFX->EndPass());
	HR(mFX->End());
	
	mGfxStats->display();

	HR(gd3dDevice->EndScene());

	// Present the backbuffer.
	HR(gd3dDevice->Present(0, 0, 0, 0));
}

void BasicTerrainDemo::buildGridGeometry()
{
	std::vector<D3DXVECTOR3> verts;
	std::vector<DWORD> indices;

	int vertRows = 129;
	int vertCols = 129;
	float dx = 1.0f;
	float dz = 1.0f;

	GenTriGrid(vertRows, vertCols, dx, dz, 
		D3DXVECTOR3(0.0f, 0.0f, 0.0f), verts, indices);

	int numVerts = vertRows*vertCols;
	int numTris  = (vertRows-1)*(vertCols-1)*2;

	// Create the mesh.
	D3DVERTEXELEMENT9 elems[MAX_FVF_DECL_SIZE];
	UINT numElems = 0;
	HR(VertexPNT::Decl->GetDeclaration(elems, &numElems));
	HR(D3DXCreateMesh(numTris, numVerts, 
		D3DXMESH_MANAGED, elems, gd3dDevice, &mTerrainMesh));

	// Write the vertices.
	VertexPNT* v = 0;
	HR(mTerrainMesh->LockVertexBuffer(0,(void**)&v));

	// width/depth
	float w = (vertCols-1) * dx; 
	float d = (vertRows-1) * dz;
	for(int i = 0; i < vertRows; ++i)
	{
		for(int j = 0; j < vertCols; ++j)
		{
			DWORD index = i * vertCols + j;
			v[index].pos    = verts[index];
			v[index].pos.y  = mHeightmap(i, j);
			v[index].normal = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
			v[index].tex0.x = (v[index].pos.x + (0.5f*w)) / w;
			v[index].tex0.y = (v[index].pos.z - (0.5f*d)) / -d;
		}
	}

	HR(mTerrainMesh->UnlockVertexBuffer());


	// Write the indices and attribute buffer.
	WORD* k = 0;
	HR(mTerrainMesh->LockIndexBuffer(0, (void**)&k));
	DWORD* attBuffer = 0;
	HR(mTerrainMesh->LockAttributeBuffer(0, &attBuffer));

	// Compute the indices for each triangle.
	for(int i = 0; i < numTris; ++i)
	{
		k[i*3+0] = (WORD)indices[i*3+0];
		k[i*3+1] = (WORD)indices[i*3+1];
		k[i*3+2] = (WORD)indices[i*3+2];

		attBuffer[i] = 0; // Always subset 0
	}

	HR(mTerrainMesh->UnlockIndexBuffer());
	HR(mTerrainMesh->UnlockAttributeBuffer());

	// Generate normals and then optimize the mesh.
	HR(D3DXComputeNormals(mTerrainMesh, 0));

	DWORD* adj = new DWORD[mTerrainMesh->GetNumFaces()*3];
	HR(mTerrainMesh->GenerateAdjacency(EPSILON, adj));
	HR(mTerrainMesh->OptimizeInplace(D3DXMESHOPT_VERTEXCACHE|D3DXMESHOPT_ATTRSORT,
		adj, 0, 0, 0));
	delete[] adj;
}

void BasicTerrainDemo::buildFX()
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

	D3DXVECTOR3 d(0.0f, 1.0f, 0.0f);
	HR(mFX->SetValue(mhDirToSunW, &d, sizeof(D3DXVECTOR3)));
}

void BasicTerrainDemo::buildViewMtx()
{
	float x = mCameraRadius * cosf(mCameraRotationY);
	float z = mCameraRadius * sinf(mCameraRotationY);
	D3DXVECTOR3 pos(x, mCameraHeight, z);
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
	D3DXMatrixLookAtLH(&mView, &pos, &target, &up);
}

void BasicTerrainDemo::buildProjMtx()
{
	float w = (float)md3dPP.BackBufferWidth;
	float h = (float)md3dPP.BackBufferHeight;
	D3DXMatrixPerspectiveFovLH(&mProj, D3DX_PI * 0.25f, w/h, 1.0f, 5000.0f);
}