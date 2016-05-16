//=============================================================================
// CloudDemo.cpp by Frank Luna (C) 2005 All Rights Reserved.
//
// Demonstrates texture animation by scrolling two cloud textures.
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
 
class CloudDemo : public D3DApp
{
public:
	CloudDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP);
	~CloudDemo();

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

	DWORD mNumGridVertices;
	DWORD mNumGridTriangles;
	
	IDirect3DVertexBuffer9* mGridVB;
	IDirect3DIndexBuffer9*  mGridIB;
	IDirect3DTexture9*      mCloudTex0;
	IDirect3DTexture9*      mCloudTex1;

	ID3DXEffect* mFX;
	D3DXHANDLE   mhTech;
	D3DXHANDLE   mhWVP;
	D3DXHANDLE   mhCloudTex0;
	D3DXHANDLE   mhCloudTex1;
	D3DXHANDLE   mhTexOffset0;
	D3DXHANDLE   mhTexOffset1;

	D3DXVECTOR2 mTexOffset0;
	D3DXVECTOR2 mTexOffset1;

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

	CloudDemo app(hInstance, "Cloud Demo", D3DDEVTYPE_HAL, D3DCREATE_HARDWARE_VERTEXPROCESSING);
	gd3dApp = &app;

	DirectInput di(DISCL_NONEXCLUSIVE|DISCL_FOREGROUND, DISCL_NONEXCLUSIVE|DISCL_FOREGROUND);
	gDInput = &di;

    return gd3dApp->run();
}

CloudDemo::CloudDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP)
: D3DApp(hInstance, winCaption, devType, requestedVP)
{
	if(!checkDeviceCaps())
	{
		MessageBox(0, "checkDeviceCaps() Failed", 0, 0);
		PostQuitMessage(0);
	}

	mGfxStats = new GfxStats();
	
	mCameraRadius    = 30.0f;
	mCameraRotationY = 1.2 * D3DX_PI;
	mCameraHeight    = 15.0f;

	D3DXMatrixIdentity(&mWorld);

	HR(D3DXCreateTextureFromFile(gd3dDevice, "cloud0.dds", &mCloudTex0));
	HR(D3DXCreateTextureFromFile(gd3dDevice, "cloud1.dds", &mCloudTex1));

	mTexOffset0 = D3DXVECTOR2(0.0f, 0.0f);
	mTexOffset1 = D3DXVECTOR2(0.0f, 0.0f);

	buildGridGeometry();
	mGfxStats->addVertices(mNumGridVertices);
	mGfxStats->addTriangles(mNumGridTriangles);

	buildFX();

	onResetDevice();

	InitAllVertexDeclarations();
}

CloudDemo::~CloudDemo()
{
	delete mGfxStats;
	ReleaseCOM(mGridVB);
	ReleaseCOM(mGridIB);
	ReleaseCOM(mCloudTex0);
	ReleaseCOM(mCloudTex1);
	ReleaseCOM(mFX);

	DestroyAllVertexDeclarations();
}

bool CloudDemo::checkDeviceCaps()
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

void CloudDemo::onLostDevice()
{
	mGfxStats->onLostDevice();
	HR(mFX->OnLostDevice());
}

void CloudDemo::onResetDevice()
{
	mGfxStats->onResetDevice();
	HR(mFX->OnResetDevice());


	// The aspect ratio depends on the backbuffer dimensions, which can 
	// possibly change after a reset.  So rebuild the projection matrix.
	buildProjMtx();
}

void CloudDemo::updateScene(float dt)
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

	// Update texture coordinate offsets.  These offsets are added to the
	// texture coordinates in the vertex shader to animate them.
	mTexOffset0 += D3DXVECTOR2(0.11f, 0.05f) * dt;
	mTexOffset1 += D3DXVECTOR2(0.25f, 0.1f) * dt;

	// Textures repeat every 1.0 unit, so reset back down to zero
	// so the coordinates do not grow.
	if(mTexOffset0.x >= 1.0f || mTexOffset0.x <= -1.0f)
		mTexOffset0.x = 0.0f;
	if(mTexOffset1.x >= 1.0f || mTexOffset1.x <= -1.0f)
		mTexOffset1.x = 0.0f;
	if(mTexOffset0.y >= 1.0f || mTexOffset0.y <= -1.0f)
		mTexOffset0.y = 0.0f;
	if(mTexOffset1.y >= 1.0f || mTexOffset1.y <= -1.0f)
		mTexOffset1.y = 0.0f;
}


void CloudDemo::drawScene()
{
	// Clear the backbuffer and depth buffer.
	HR(gd3dDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffeeeeee, 1.0f, 0));

	HR(gd3dDevice->BeginScene());

	// Setup the rendering FX
	HR(mFX->SetTechnique(mhTech));

	HR(mFX->SetMatrix(mhWVP, &(mWorld*mView*mProj)));
	HR(mFX->SetTexture(mhCloudTex0, mCloudTex0));
	HR(mFX->SetTexture(mhCloudTex1, mCloudTex1));
	HR(mFX->SetValue(mhTexOffset0, &mTexOffset0, sizeof(D3DXVECTOR2)));
	HR(mFX->SetValue(mhTexOffset1, &mTexOffset1, sizeof(D3DXVECTOR2)));

	HR(gd3dDevice->SetVertexDeclaration(VertexPT::Decl));
	HR(gd3dDevice->SetStreamSource(0, mGridVB, 0, sizeof(VertexPT)));
	HR(gd3dDevice->SetIndices(mGridIB));

	// Begin passes.
	UINT numPasses = 0;
	HR(mFX->Begin(&numPasses, 0));
	for(UINT i = 0; i < numPasses; ++i)
	{
		HR(mFX->BeginPass(i));
		HR(gd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, mNumGridVertices, 0, mNumGridTriangles));
		HR(mFX->EndPass());
	}
	HR(mFX->End());

	
	mGfxStats->display();

	HR(gd3dDevice->EndScene());

	// Present the backbuffer.
	HR(gd3dDevice->Present(0, 0, 0, 0));
}

void CloudDemo::buildGridGeometry()
{
	std::vector<D3DXVECTOR3> verts;
	std::vector<DWORD> indices;

	GenTriGrid(100, 100, 1.0f, 1.0f, 
		D3DXVECTOR3(0.0f, 0.0f, 0.0f), verts, indices);

	// Save vertex count and triangle count for DrawIndexedPrimitive arguments.
	mNumGridVertices  = 100*100;
	mNumGridTriangles = 99*99*2;

	// Obtain a pointer to a new vertex buffer.
	HR(gd3dDevice->CreateVertexBuffer(mNumGridVertices * sizeof(VertexPT), 
		D3DUSAGE_WRITEONLY,	0, D3DPOOL_MANAGED, &mGridVB, 0));

	// Now lock it to obtain a pointer to its internal data, and write the
	// grid's vertex data.
	VertexPT* v = 0;
	HR(mGridVB->Lock(0, 0, (void**)&v, 0));

	float texScale = 0.02f;
	for(int i = 0; i < 100; ++i)
	{
		for(int j = 0; j < 100; ++j)
		{
			DWORD index = i * 100 + j;
			v[index].pos  = verts[index];
			v[index].tex0 = D3DXVECTOR2((float)j, (float)i) * texScale;
		}
	}

	HR(mGridVB->Unlock());


	// Obtain a pointer to a new index buffer.
	HR(gd3dDevice->CreateIndexBuffer(mNumGridTriangles*3*sizeof(WORD), D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16, D3DPOOL_MANAGED, &mGridIB, 0));

	// Now lock it to obtain a pointer to its internal data, and write the
	// grid's index data.

	WORD* k = 0;
	HR(mGridIB->Lock(0, 0, (void**)&k, 0));

	for(DWORD i = 0; i < mNumGridTriangles*3; ++i)
		k[i] = (WORD)indices[i];

	HR(mGridIB->Unlock());
}

void CloudDemo::buildFX()
{
	// Create the FX from a .fx file.
	ID3DXBuffer* errors = 0;
	HR(D3DXCreateEffectFromFile(gd3dDevice, "clouds.fx", 
		0, 0, D3DXSHADER_DEBUG, 0, &mFX, &errors));
	if( errors )
		MessageBox(0, (char*)errors->GetBufferPointer(), 0, 0);

	// Obtain handles.
	mhTech       = mFX->GetTechniqueByName("CloudsTech");
	mhWVP        = mFX->GetParameterByName(0, "gWVP");
	mhCloudTex0  = mFX->GetParameterByName(0, "gCloudTex0");
	mhCloudTex1  = mFX->GetParameterByName(0, "gCloudTex1");
	mhTexOffset0 = mFX->GetParameterByName(0, "gTexOffset0");
	mhTexOffset1 = mFX->GetParameterByName(0, "gTexOffset1");
}

void CloudDemo::buildViewMtx()
{
	float x = mCameraRadius * cosf(mCameraRotationY);
	float z = mCameraRadius * sinf(mCameraRotationY);
	D3DXVECTOR3 pos(x, mCameraHeight, z);
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
	D3DXMatrixLookAtLH(&mView, &pos, &target, &up);
}

void CloudDemo::buildProjMtx()
{
	float w = (float)md3dPP.BackBufferWidth;
	float h = (float)md3dPP.BackBufferHeight;
	D3DXMatrixPerspectiveFovLH(&mProj, D3DX_PI * 0.25f, w/h, 1.0f, 5000.0f);
}