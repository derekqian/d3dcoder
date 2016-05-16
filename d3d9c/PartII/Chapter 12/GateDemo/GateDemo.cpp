//=============================================================================
// GateDemo.cpp by Frank Luna (C) 2005 All Rights Reserved.
//
// Demonstrates the alpha test by drawing a gate from a textured quad.
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

class GateDemo : public D3DApp
{
public:
	GateDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP);
	~GateDemo();

	bool checkDeviceCaps();
	void onLostDevice();
	void onResetDevice();
	void updateScene(float dt);
	void drawScene();

	// Helper methods
	void buildGridGeometry();
	void buildGateGeometry();
	void buildFX();
	void buildViewMtx();
	void buildProjMtx();

	void drawGround();
	void drawGate();

private:
	GfxStats* mGfxStats;

	DWORD mNumGridVertices;
	DWORD mNumGridTriangles;
	
	IDirect3DVertexBuffer9* mGridVB;
	IDirect3DIndexBuffer9*  mGridIB;
	IDirect3DVertexBuffer9* mGateVB;
	IDirect3DIndexBuffer9*  mGateIB;
	IDirect3DTexture9*     mGroundTex;
	IDirect3DTexture9*     mGateTex;

	ID3DXEffect* mFX;
	D3DXHANDLE   mhTech;
	D3DXHANDLE   mhWVP;
	D3DXHANDLE   mhWorldInvTrans;
	D3DXHANDLE   mhLightVecW;
	D3DXHANDLE   mhDiffuseMtrl;
	D3DXHANDLE   mhDiffuseLight;
	D3DXHANDLE   mhAmbientMtrl;
	D3DXHANDLE   mhAmbientLight;
	D3DXHANDLE   mhSpecularMtrl;
	D3DXHANDLE   mhSpecularLight;
	D3DXHANDLE   mhSpecularPower;
	D3DXHANDLE   mhEyePos;
	D3DXHANDLE   mhWorld;
	D3DXHANDLE   mhTex;

	Mtrl mGroundMtrl;
	Mtrl mGateMtrl;

	D3DXVECTOR3 mLightVecW;
	D3DXCOLOR   mAmbientLight;
	D3DXCOLOR   mDiffuseLight;
	D3DXCOLOR   mSpecularLight;

	float mCameraRotationY;
	float mCameraRadius;
	float mCameraHeight;

	D3DXMATRIX mGroundWorld;
	D3DXMATRIX mGateWorld;
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

	GateDemo app(hInstance, "Gate Demo", D3DDEVTYPE_HAL, D3DCREATE_HARDWARE_VERTEXPROCESSING);
	gd3dApp = &app;

	DirectInput di(DISCL_NONEXCLUSIVE|DISCL_FOREGROUND, DISCL_NONEXCLUSIVE|DISCL_FOREGROUND);
	gDInput = &di;

    return gd3dApp->run();
}

GateDemo::GateDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP)
: D3DApp(hInstance, winCaption, devType, requestedVP)
{
	if(!checkDeviceCaps())
	{
		MessageBox(0, "checkDeviceCaps() Failed", 0, 0);
		PostQuitMessage(0);
	}

	mGfxStats = new GfxStats();
	
	mCameraRadius    = 6.0f;
	mCameraRotationY = 1.2 * D3DX_PI;
	mCameraHeight    = 3.0f;

	mLightVecW     = D3DXVECTOR3(0.0, 0.707f, -0.707f);
	mDiffuseLight  = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	mAmbientLight  = D3DXCOLOR(0.6f, 0.6f, 0.6f, 1.0f);
	mSpecularLight = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);

	mGroundMtrl.ambient   = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	mGroundMtrl.diffuse   = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	mGroundMtrl.spec      = D3DXCOLOR(0.4f, 0.4f, 0.4f, 1.0f);
	mGroundMtrl.specPower = 8.0f;

	mGateMtrl.ambient   = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	mGateMtrl.diffuse   = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	mGateMtrl.spec      = D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f);
	mGateMtrl.specPower = 8.0f;

	D3DXMatrixIdentity(&mGroundWorld);
	D3DXMatrixIdentity(&mGateWorld);

	HR(D3DXCreateTextureFromFile(gd3dDevice, "ground0.dds", &mGroundTex));
	HR(D3DXCreateTextureFromFile(gd3dDevice, "gatea.dds", &mGateTex));

	buildGridGeometry();
	buildGateGeometry();

	mGfxStats->addVertices(mNumGridVertices);
	mGfxStats->addTriangles(mNumGridTriangles);

	// Add gate quad vertices.
	mGfxStats->addVertices(4);
	mGfxStats->addTriangles(2);

	buildFX();

	onResetDevice();

	InitAllVertexDeclarations();
}

GateDemo::~GateDemo()
{
	delete mGfxStats;
	ReleaseCOM(mGridVB);
	ReleaseCOM(mGridIB);
	ReleaseCOM(mGroundTex);
	ReleaseCOM(mGateVB);
	ReleaseCOM(mGateIB);
	ReleaseCOM(mGateTex);
	ReleaseCOM(mFX);

	DestroyAllVertexDeclarations();
}

bool GateDemo::checkDeviceCaps()
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

void GateDemo::onLostDevice()
{
	mGfxStats->onLostDevice();
	HR(mFX->OnLostDevice());
}

void GateDemo::onResetDevice()
{
	mGfxStats->onResetDevice();
	HR(mFX->OnResetDevice());


	// The aspect ratio depends on the backbuffer dimensions, which can 
	// possibly change after a reset.  So rebuild the projection matrix.
	buildProjMtx();
}

void GateDemo::updateScene(float dt)
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


void GateDemo::drawScene()
{
	// Clear the backbuffer and depth buffer.
	HR(gd3dDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffeeeeee, 1.0f, 0));

	HR(gd3dDevice->BeginScene());

	// Setup the rendering FX
	HR(mFX->SetTechnique(mhTech));
	HR(mFX->SetValue(mhLightVecW, &mLightVecW, sizeof(D3DXVECTOR3)));
	HR(mFX->SetValue(mhDiffuseLight, &mDiffuseLight, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhAmbientLight, &mAmbientLight, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhSpecularLight, &mSpecularLight, sizeof(D3DXCOLOR)));
	
	drawGround();
	drawGate();
	
	mGfxStats->display();

	HR(gd3dDevice->EndScene());

	// Present the backbuffer.
	HR(gd3dDevice->Present(0, 0, 0, 0));
}

void GateDemo::buildGridGeometry()
{
	std::vector<D3DXVECTOR3> verts;
	std::vector<DWORD> indices;

	GenTriGrid(100, 100, 1.0f, 1.0f, 
		D3DXVECTOR3(0.0f, 0.0f, 0.0f), verts, indices);

	// Save vertex count and triangle count for DrawIndexedPrimitive arguments.
	mNumGridVertices  = 100*100;
	mNumGridTriangles = 99*99*2;

	// Obtain a pointer to a new vertex buffer.
	HR(gd3dDevice->CreateVertexBuffer(mNumGridVertices * sizeof(VertexPNT), 
		D3DUSAGE_WRITEONLY,	0, D3DPOOL_MANAGED, &mGridVB, 0));

	// Now lock it to obtain a pointer to its internal data, and write the
	// grid's vertex data.
	VertexPNT* v = 0;
	HR(mGridVB->Lock(0, 0, (void**)&v, 0));

	float texScale = 0.2f;
	for(int i = 0; i < 100; ++i)
	{
		for(int j = 0; j < 100; ++j)
		{
			DWORD index = i * 100 + j;
			v[index].pos    = verts[index];
			v[index].normal = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
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

void GateDemo::buildGateGeometry()
{
	// Gate is just a rectangle aligned with the xy-plane.
	

	// Obtain a pointer to a new vertex buffer.
	HR(gd3dDevice->CreateVertexBuffer(4* sizeof(VertexPNT), 
		D3DUSAGE_WRITEONLY,	0, D3DPOOL_MANAGED, &mGateVB, 0));

	// Now lock it to obtain a pointer to its internal data, and write the
	// grid's vertex data.
	VertexPNT* v = 0;
	HR(mGateVB->Lock(0, 0, (void**)&v, 0));

	// Scale texture coordinates by 4 units in the v-direction for tiling.
	v[0] = VertexPNT(-20.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[1] = VertexPNT(-20.0f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[2] = VertexPNT( 20.0f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 4.0f, 0.0f);
	v[3] = VertexPNT( 20.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 4.0f, 1.0f);
	HR(mGateVB->Unlock());


	// Obtain a pointer to a new index buffer.
	HR(gd3dDevice->CreateIndexBuffer(6*sizeof(WORD), D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16, D3DPOOL_MANAGED, &mGateIB, 0));

	// Now lock it to obtain a pointer to its internal data, and write the
	// grid's index data.

	WORD* k = 0;
	HR(mGateIB->Lock(0, 0, (void**)&k, 0));

	k[0] = 0;  k[1] = 1;  k[2] = 2; // Triangle 0
	k[3] = 0;  k[4] = 2;  k[5] = 3; // Triangle 1

	HR(mGateIB->Unlock());
}

void GateDemo::buildFX()
{
	// Create the FX from a .fx file.
	ID3DXBuffer* errors = 0;
	HR(D3DXCreateEffectFromFile(gd3dDevice, "DirLightTex.fx", 
		0, 0, D3DXSHADER_DEBUG, 0, &mFX, &errors));
	if( errors )
		MessageBox(0, (char*)errors->GetBufferPointer(), 0, 0);

	// Obtain handles.
	mhTech          = mFX->GetTechniqueByName("DirLightTexTech");
	mhWVP           = mFX->GetParameterByName(0, "gWVP");
	mhWorldInvTrans = mFX->GetParameterByName(0, "gWorldInvTrans");
	mhLightVecW     = mFX->GetParameterByName(0, "gLightVecW");
	mhDiffuseMtrl   = mFX->GetParameterByName(0, "gDiffuseMtrl");
	mhDiffuseLight  = mFX->GetParameterByName(0, "gDiffuseLight");
	mhAmbientMtrl   = mFX->GetParameterByName(0, "gAmbientMtrl");
	mhAmbientLight  = mFX->GetParameterByName(0, "gAmbientLight");
	mhSpecularMtrl  = mFX->GetParameterByName(0, "gSpecularMtrl");
	mhSpecularLight = mFX->GetParameterByName(0, "gSpecularLight");
	mhSpecularPower = mFX->GetParameterByName(0, "gSpecularPower");
	mhEyePos        = mFX->GetParameterByName(0, "gEyePosW");
	mhWorld         = mFX->GetParameterByName(0, "gWorld");
	mhTex           = mFX->GetParameterByName(0, "gTex");
}

void GateDemo::buildViewMtx()
{
	float x = mCameraRadius * cosf(mCameraRotationY);
	float z = mCameraRadius * sinf(mCameraRotationY);
	D3DXVECTOR3 pos(x, mCameraHeight, z);
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
	D3DXMatrixLookAtLH(&mView, &pos, &target, &up);

	HR(mFX->SetValue(mhEyePos, &pos, sizeof(D3DXVECTOR3)));
}

void GateDemo::buildProjMtx()
{
	float w = (float)md3dPP.BackBufferWidth;
	float h = (float)md3dPP.BackBufferHeight;
	D3DXMatrixPerspectiveFovLH(&mProj, D3DX_PI * 0.25f, w/h, 1.0f, 5000.0f);
}

void GateDemo::drawGround()
{
	HR(mFX->SetValue(mhAmbientMtrl, &mGroundMtrl.ambient, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhDiffuseMtrl, &mGroundMtrl.diffuse, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhSpecularMtrl, &mGroundMtrl.spec, sizeof(D3DXCOLOR)));
	HR(mFX->SetFloat(mhSpecularPower, mGroundMtrl.specPower));

	HR(mFX->SetMatrix(mhWVP, &(mGroundWorld*mView*mProj)));
	D3DXMATRIX worldInvTrans;
	D3DXMatrixInverse(&worldInvTrans, 0, &mGroundWorld);
	D3DXMatrixTranspose(&worldInvTrans, &worldInvTrans);
	HR(mFX->SetMatrix(mhWorldInvTrans, &worldInvTrans));
	HR(mFX->SetMatrix(mhWorld, &mGroundWorld));
	HR(mFX->SetTexture(mhTex, mGroundTex));

	HR(gd3dDevice->SetVertexDeclaration(VertexPNT::Decl));
	HR(gd3dDevice->SetStreamSource(0, mGridVB, 0, sizeof(VertexPNT)));
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
}

void GateDemo::drawGate()
{
	// Enable alpha test.
	HR(gd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, true));
	HR(gd3dDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL));
	HR(gd3dDevice->SetRenderState(D3DRS_ALPHAREF, 100));

	// Turn off backface culling so you can see both sides of the gate.
	HR(gd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));

	HR(mFX->SetValue(mhAmbientMtrl, &mGateMtrl.ambient, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhDiffuseMtrl, &mGateMtrl.diffuse, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhSpecularMtrl, &mGateMtrl.spec, sizeof(D3DXCOLOR)));
	HR(mFX->SetFloat(mhSpecularPower, mGateMtrl.specPower));

	HR(mFX->SetMatrix(mhWVP, &(mGateWorld*mView*mProj)));
	D3DXMATRIX worldInvTrans;
	D3DXMatrixInverse(&worldInvTrans, 0, &mGateWorld);
	D3DXMatrixTranspose(&worldInvTrans, &worldInvTrans);
	HR(mFX->SetMatrix(mhWorldInvTrans, &worldInvTrans));
	HR(mFX->SetMatrix(mhWorld, &mGateWorld));
	HR(mFX->SetTexture(mhTex, mGateTex));

	HR(gd3dDevice->SetVertexDeclaration(VertexPNT::Decl));
	HR(gd3dDevice->SetStreamSource(0, mGateVB, 0, sizeof(VertexPNT)));
	HR(gd3dDevice->SetIndices(mGateIB));

	// Begin passes.
	UINT numPasses = 0;
	HR(mFX->Begin(&numPasses, 0));
	for(UINT i = 0; i < numPasses; ++i)
	{
		HR(mFX->BeginPass(i));
		HR(gd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 4, 0, 2));
		HR(mFX->EndPass());
	}
	HR(mFX->End());

	// Disable alpha test.
	HR(gd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, false));

	// Turn culling back on.
	HR(gd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW));
}