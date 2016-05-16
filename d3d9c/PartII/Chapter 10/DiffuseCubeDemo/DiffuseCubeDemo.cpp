//=============================================================================
// DiffuseCubeDemo.cpp by Frank Luna (C) 2005 All Rights Reserved.
//
// Demonstrates diffuse lighting and manually specifying the vertex
// and vertex normal data.
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

class DiffuseCubeDemo : public D3DApp
{
public:
	DiffuseCubeDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP);
	~DiffuseCubeDemo();

	bool checkDeviceCaps();
	void onLostDevice();
	void onResetDevice();
	void updateScene(float dt);
	void drawScene();

	// Helper methods
	void buildVertexBuffer();
	void buildIndexBuffer();
	void buildFX();
	void buildViewMtx();
	void buildProjMtx();

private:
	GfxStats* mGfxStats;
	
	IDirect3DVertexBuffer9* mVB;
	IDirect3DIndexBuffer9*  mIB;
	ID3DXEffect*            mFX;
	D3DXHANDLE              mhTech;
	D3DXHANDLE              mhWVP;
	D3DXHANDLE              mhWorldInverseTranspose;
	D3DXHANDLE              mhLightVecW;
	D3DXHANDLE              mhDiffuseMtrl;
	D3DXHANDLE              mhDiffuseLight;

	D3DXVECTOR3 mLightVecW;
	D3DXCOLOR   mDiffuseMtrl;
	D3DXCOLOR   mDiffuseLight;

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

	DiffuseCubeDemo app(hInstance, "Diffuse Cube Demo", D3DDEVTYPE_HAL, D3DCREATE_HARDWARE_VERTEXPROCESSING);
	gd3dApp = &app;

	DirectInput di(DISCL_NONEXCLUSIVE|DISCL_FOREGROUND, DISCL_NONEXCLUSIVE|DISCL_FOREGROUND);
	gDInput = &di;

    return gd3dApp->run();
}

DiffuseCubeDemo::DiffuseCubeDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP)
: D3DApp(hInstance, winCaption, devType, requestedVP)
{
	if(!checkDeviceCaps())
	{
		MessageBox(0, "checkDeviceCaps() Failed", 0, 0);
		PostQuitMessage(0);
	}

	mGfxStats = new GfxStats();

	mCameraRadius    = 10.0f;
	mCameraRotationY = 1.2 * D3DX_PI;
	mCameraHeight    = 5.0f;

	mLightVecW    = D3DXVECTOR3(-0.5, 0.75f, -2.0f);
	D3DXVec3Normalize(&mLightVecW, &mLightVecW);
	mDiffuseMtrl  = D3DXCOLOR(0.0f, 0.0f, 1.0f, 1.0f);
	mDiffuseLight = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);

	D3DXMatrixIdentity(&mWorld);

	buildVertexBuffer();
	buildIndexBuffer();
	buildFX();

	onResetDevice();

	InitAllVertexDeclarations();
}

DiffuseCubeDemo::~DiffuseCubeDemo()
{
	delete mGfxStats;
	ReleaseCOM(mVB);
	ReleaseCOM(mIB);
	ReleaseCOM(mFX);

	DestroyAllVertexDeclarations();
}

bool DiffuseCubeDemo::checkDeviceCaps()
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

void DiffuseCubeDemo::onLostDevice()
{
	mGfxStats->onLostDevice();
	HR(mFX->OnLostDevice());
}

void DiffuseCubeDemo::onResetDevice()
{
	mGfxStats->onResetDevice();
	HR(mFX->OnResetDevice());


	// The aspect ratio depends on the backbuffer dimensions, which can 
	// possibly change after a reset.  So rebuild the projection matrix.
	buildProjMtx();
}

void DiffuseCubeDemo::updateScene(float dt)
{
	mGfxStats->setVertexCount(24);
	mGfxStats->setTriCount(12);
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


void DiffuseCubeDemo::drawScene()
{
	// Clear the backbuffer and depth buffer.
	HR(gd3dDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffeeeeee, 1.0f, 0));

	HR(gd3dDevice->BeginScene());

	// Let Direct3D know the vertex buffer, index buffer and vertex 
	// declaration we are using.
	HR(gd3dDevice->SetStreamSource(0, mVB, 0, sizeof(VertexPN)));
	HR(gd3dDevice->SetIndices(mIB));
	HR(gd3dDevice->SetVertexDeclaration(VertexPN::Decl));

	// Setup the rendering FX
	HR(mFX->SetTechnique(mhTech));

	HR(mFX->SetMatrix(mhWVP, &(mWorld*mView*mProj)));
	D3DXMATRIX worldInverseTranspose;
	D3DXMatrixInverse(&worldInverseTranspose, 0, &mWorld);
	D3DXMatrixTranspose(&worldInverseTranspose, &worldInverseTranspose);
	HR(mFX->SetMatrix(mhWorldInverseTranspose, &worldInverseTranspose));
	HR(mFX->SetValue(mhLightVecW, &mLightVecW, sizeof(D3DXVECTOR3)));
	HR(mFX->SetValue(mhDiffuseMtrl, &mDiffuseMtrl, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhDiffuseLight, &mDiffuseLight, sizeof(D3DXCOLOR)));

	// Begin passes.
	UINT numPasses = 0;
	HR(mFX->Begin(&numPasses, 0));
	for(UINT i = 0; i < numPasses; ++i)
	{
		HR(mFX->BeginPass(i));
		HR(gd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 8, 0, 12));
		HR(mFX->EndPass());
	}
	HR(mFX->End());

	
	mGfxStats->display();

	HR(gd3dDevice->EndScene());

	// Present the backbuffer.
	HR(gd3dDevice->Present(0, 0, 0, 0));
}

void DiffuseCubeDemo::buildVertexBuffer()
{
	// Obtain a pointer to a new vertex buffer.
	HR(gd3dDevice->CreateVertexBuffer(24 * sizeof(VertexPN), D3DUSAGE_WRITEONLY,
		0, D3DPOOL_MANAGED, &mVB, 0));

	// Now lock it to obtain a pointer to its internal data, and write the
	// cube's vertex data.

	// NOTE: One key point to note is that with normals, we no longer have eight
	// unique vertices for the cube.  This is because, even though the vertices 
	// share spatial data, they do not share normal data.  
	VertexPN* v = 0;
	HR(mVB->Lock(0, 0, (void**)&v, 0));

	// fill in the front face vertex data
	v[0] = VertexPN(-1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f);
	v[1] = VertexPN(-1.0f,  1.0f, -1.0f, 0.0f, 0.0f, -1.0f);
	v[2] = VertexPN( 1.0f,  1.0f, -1.0f, 0.0f, 0.0f, -1.0f);
	v[3] = VertexPN( 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f);

	// fill in the back face vertex data
	v[4] = VertexPN(-1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f);
	v[5] = VertexPN( 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f);
	v[6] = VertexPN( 1.0f,  1.0f, 1.0f, 0.0f, 0.0f, 1.0f);
	v[7] = VertexPN(-1.0f,  1.0f, 1.0f, 0.0f, 0.0f, 1.0f);

	// fill in the top face vertex data
	v[8]  = VertexPN(-1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f);
	v[9]  = VertexPN(-1.0f, 1.0f,  1.0f, 0.0f, 1.0f, 0.0f);
	v[10] = VertexPN( 1.0f, 1.0f,  1.0f, 0.0f, 1.0f, 0.0f);
	v[11] = VertexPN( 1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f);

	// fill in the bottom face vertex data
	v[12] = VertexPN(-1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f);
	v[13] = VertexPN( 1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f);
	v[14] = VertexPN( 1.0f, -1.0f,  1.0f, 0.0f, -1.0f, 0.0f);
	v[15] = VertexPN(-1.0f, -1.0f,  1.0f, 0.0f, -1.0f, 0.0f);

	// fill in the left face vertex data
	v[16] = VertexPN(-1.0f, -1.0f,  1.0f, -1.0f, 0.0f, 0.0f);
	v[17] = VertexPN(-1.0f,  1.0f,  1.0f, -1.0f, 0.0f, 0.0f);
	v[18] = VertexPN(-1.0f,  1.0f, -1.0f, -1.0f, 0.0f, 0.0f);
	v[19] = VertexPN(-1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f);

	// fill in the right face vertex data
	v[20] = VertexPN( 1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f);
	v[21] = VertexPN( 1.0f,  1.0f, -1.0f, 1.0f, 0.0f, 0.0f);
	v[22] = VertexPN( 1.0f,  1.0f,  1.0f, 1.0f, 0.0f, 0.0f);
	v[23] = VertexPN( 1.0f, -1.0f,  1.0f, 1.0f, 0.0f, 0.0f);

	HR(mVB->Unlock());
}

void DiffuseCubeDemo::buildIndexBuffer()
{
	// Obtain a pointer to a new index buffer.
	HR(gd3dDevice->CreateIndexBuffer(36 * sizeof(WORD), D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16, D3DPOOL_MANAGED, &mIB, 0));

	// Now lock it to obtain a pointer to its internal data, and write the
	// cube's index data.

	WORD* k = 0;

	HR(mIB->Lock(0, 0, (void**)&k, 0));

	// fill in the front face index data
	k[0] = 0; k[1] = 1; k[2] = 2;
	k[3] = 0; k[4] = 2; k[5] = 3;

	// fill in the back face index data
	k[6] = 4; k[7]  = 5; k[8]  = 6;
	k[9] = 4; k[10] = 6; k[11] = 7;

	// fill in the top face index data
	k[12] = 8; k[13] =  9; k[14] = 10;
	k[15] = 8; k[16] = 10; k[17] = 11;

	// fill in the bottom face index data
	k[18] = 12; k[19] = 13; k[20] = 14;
	k[21] = 12; k[22] = 14; k[23] = 15;

	// fill in the left face index data
	k[24] = 16; k[25] = 17; k[26] = 18;
	k[27] = 16; k[28] = 18; k[29] = 19;

	// fill in the right face index data
	k[30] = 20; k[31] = 21; k[32] = 22;
	k[33] = 20; k[34] = 22; k[35] = 23;

	HR(mIB->Unlock());
}

void DiffuseCubeDemo::buildFX()
{
	// Create the FX from a .fx file.
	ID3DXBuffer* errors = 0;
	HR(D3DXCreateEffectFromFile(gd3dDevice, "diffuse.fx", 
		0, 0, D3DXSHADER_DEBUG, 0, &mFX, &errors));
	if( errors )
		MessageBox(0, (char*)errors->GetBufferPointer(), 0, 0);

	// Obtain handles.
	mhTech                  = mFX->GetTechniqueByName("DiffuseTech");
	mhWVP                   = mFX->GetParameterByName(0, "gWVP");
	mhWorldInverseTranspose = mFX->GetParameterByName(0, "gWorldInverseTranspose");
	mhLightVecW             = mFX->GetParameterByName(0, "gLightVecW");
	mhDiffuseMtrl           = mFX->GetParameterByName(0, "gDiffuseMtrl");
	mhDiffuseLight          = mFX->GetParameterByName(0, "gDiffuseLight");
}

void DiffuseCubeDemo::buildViewMtx()
{
	float x = mCameraRadius * cosf(mCameraRotationY);
	float z = mCameraRadius * sinf(mCameraRotationY);
	D3DXVECTOR3 pos(x, mCameraHeight, z);
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
	D3DXMatrixLookAtLH(&mView, &pos, &target, &up);
}

void DiffuseCubeDemo::buildProjMtx()
{
	float w = (float)md3dPP.BackBufferWidth;
	float h = (float)md3dPP.BackBufferHeight;
	D3DXMatrixPerspectiveFovLH(&mProj, D3DX_PI * 0.25f, w/h, 1.0f, 5000.0f);
}