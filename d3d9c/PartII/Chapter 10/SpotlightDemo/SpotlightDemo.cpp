//=============================================================================
// Spotlight.cpp by Frank Luna (C) 2005 All Rights Reserved.
//
// Demonstrates an animated spotlight.
//
// Controls: Use mouse to orbit and zoom; use the 'W' and 'S' keys to 
//           alter the height of the camera.
//           Use 'G' and 'H' to decrease and increase the spotlight cone,
//           respectively.
//=============================================================================

#include "d3dApp.h"
#include "DirectInput.h"
#include <crtdbg.h>
#include "GfxStats.h"
#include <list>
#include "Vertex.h"

class SpotlightDemo : public D3DApp
{
public:
	SpotlightDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP);
	~SpotlightDemo();

	bool checkDeviceCaps();
	void onLostDevice();
	void onResetDevice();
	void updateScene(float dt);
	void drawScene();

	// Helper methods
	void buildGeoBuffers();
	void buildFX();
	void buildViewMtx();
	void buildProjMtx();

	void drawGrid();
	void drawCylinders();
	void drawSpheres();

private:
	GfxStats* mGfxStats;

	DWORD mNumGridVertices;
	DWORD mNumGridTriangles;

	ID3DXMesh* mCylinder;
	ID3DXMesh* mSphere;

	IDirect3DVertexBuffer9* mVB;
	IDirect3DIndexBuffer9*  mIB;

	ID3DXEffect* mFX;
	D3DXHANDLE   mhTech;
	D3DXHANDLE   mhWVP;
	D3DXHANDLE   mhWorldInvTrans;
	D3DXHANDLE   mhAmbientLight;
	D3DXHANDLE   mhDiffuseLight;
	D3DXHANDLE   mhSpecLight;
	D3DXHANDLE   mhLightPosW;
	D3DXHANDLE   mhLightDirW;
	D3DXHANDLE   mhAttenuation012;
	D3DXHANDLE   mhSpotPower;
	D3DXHANDLE   mhAmbientMtrl;
	D3DXHANDLE   mhDiffuseMtrl;
	D3DXHANDLE   mhSpecMtrl;
	D3DXHANDLE   mhSpecPower;
	D3DXHANDLE   mhEyePos;
	D3DXHANDLE   mhWorld;

	D3DXCOLOR   mAmbientLight;
	D3DXCOLOR   mDiffuseLight;
	D3DXCOLOR   mSpecLight;
	D3DXVECTOR3 mAttenuation012;
	float       mSpotPower;

	Mtrl  mGridMtrl;
	Mtrl  mCylinderMtrl;
	Mtrl  mSphereMtrl;

	float mCameraRotationY;
	float mCameraRadius;
	float mCameraHeight;

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

	SpotlightDemo app(hInstance, "Spotlight Demo", D3DDEVTYPE_HAL, D3DCREATE_HARDWARE_VERTEXPROCESSING);
	gd3dApp = &app;

	DirectInput di(DISCL_NONEXCLUSIVE|DISCL_FOREGROUND, DISCL_NONEXCLUSIVE|DISCL_FOREGROUND);
	gDInput = &di;

    return gd3dApp->run();
}

SpotlightDemo::SpotlightDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP)
: D3DApp(hInstance, winCaption, devType, requestedVP)
{
	if(!checkDeviceCaps())
	{
		MessageBox(0, "checkDeviceCaps() Failed", 0, 0);
		PostQuitMessage(0);
	}

	mGfxStats = new GfxStats();

	mCameraRadius    = 50.0f;
	mCameraRotationY = 1.2 * D3DX_PI;
	mCameraHeight    = 20.0f;

	mAmbientLight   = 0.4f*WHITE;
	mDiffuseLight   = WHITE;
	mSpecLight      = WHITE;
	mAttenuation012 = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
	mSpotPower      = 16.0f;

	mGridMtrl     = Mtrl(BLUE, BLUE, WHITE, 16.0f);
	mCylinderMtrl = Mtrl(RED, RED, WHITE, 8.0f);
	mSphereMtrl   = Mtrl(GREEN, GREEN, WHITE, 8.0f);

	HR(D3DXCreateCylinder(gd3dDevice, 1.0f, 1.0f, 6.0f, 20, 20, &mCylinder, 0));
	HR(D3DXCreateSphere(gd3dDevice, 1.0f, 20, 20, &mSphere, 0));

	buildGeoBuffers();
	buildFX();

	// If you look at the drawCylinders and drawSpheres functions, you see
	// that we draw 14 cylinders and 14 spheres.
	int numCylVerts    = mCylinder->GetNumVertices() * 14;
	int numSphereVerts = mSphere->GetNumVertices()   * 14;
	int numCylTris     = mCylinder->GetNumFaces()    * 14;
	int numSphereTris  = mSphere->GetNumFaces()      * 14;

	mGfxStats->addVertices(mNumGridVertices);
	mGfxStats->addVertices(numCylVerts);
	mGfxStats->addVertices(numSphereVerts);
	mGfxStats->addTriangles(mNumGridTriangles);
	mGfxStats->addTriangles(numCylTris);
	mGfxStats->addTriangles(numSphereTris);

	onResetDevice();

	InitAllVertexDeclarations();
}

SpotlightDemo::~SpotlightDemo()
{
	delete mGfxStats;
	ReleaseCOM(mVB);
	ReleaseCOM(mIB);
	ReleaseCOM(mFX);
	ReleaseCOM(mCylinder);
	ReleaseCOM(mSphere);

	DestroyAllVertexDeclarations();
}

bool SpotlightDemo::checkDeviceCaps()
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

void SpotlightDemo::onLostDevice()
{
	mGfxStats->onLostDevice();
	HR(mFX->OnLostDevice());
}

void SpotlightDemo::onResetDevice()
{
	mGfxStats->onResetDevice();
	HR(mFX->OnResetDevice());


	// The aspect ratio depends on the backbuffer dimensions, which can 
	// possibly change after a reset.  So rebuild the projection matrix.
	buildProjMtx();
}

void SpotlightDemo::updateScene(float dt)
{
	mGfxStats->update(dt);

	// Get snapshot of input devices.
	gDInput->poll();

	// Check input.
	if( gDInput->keyDown(DIK_W) )	 
		mCameraHeight   += 25.0f * dt;
	if( gDInput->keyDown(DIK_S) )	 
		mCameraHeight   -= 25.0f * dt;

	// Divide to make mouse less sensitive. 
	mCameraRotationY += gDInput->mouseDX() / 100.0f;
	mCameraRadius    += gDInput->mouseDY() / 25.0f;

	// If we rotate over 360 degrees, just roll back to 0
	if( fabsf(mCameraRotationY) >= 2.0f * D3DX_PI ) 
		mCameraRotationY = 0.0f;

	// Don't let radius get too small.
	if( mCameraRadius < 5.0f )
		mCameraRadius = 5.0f;

	// Control spotlight cone.
	if( gDInput->keyDown(DIK_G) )	 
		mSpotPower += 25.0f * dt;
	if( gDInput->keyDown(DIK_H) )	 
		mSpotPower   -= 25.0f * dt;

	// Clamp spot Power.
	if( mSpotPower < 1.0f )
		mSpotPower = 1.0f;
	if( mSpotPower > 64.0f )
		mSpotPower = 64.0f;

	// The camera position/orientation relative to world space can 
	// change every frame based on input, so we need to rebuild the
	// view matrix every frame with the latest changes.
	buildViewMtx();
}


void SpotlightDemo::drawScene()
{
	// Clear the backbuffer and depth buffer.
	HR(gd3dDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0));

	HR(gd3dDevice->BeginScene());

	// Setup the rendering FX
	HR(mFX->SetValue(mhAmbientLight, &mAmbientLight, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhDiffuseLight, &mDiffuseLight, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhSpecLight, &mSpecLight, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhAttenuation012, &mAttenuation012, sizeof(D3DXVECTOR3)));
	HR(mFX->SetFloat(mhSpotPower, mSpotPower));

	// Begin passes.
	UINT numPasses = 0;
	HR(mFX->Begin(&numPasses, 0));
	for(UINT i = 0; i < numPasses; ++i)
	{
		HR(mFX->BeginPass(i));

		drawGrid();
		drawCylinders();
		drawSpheres();

		HR(mFX->EndPass());
	}
	HR(mFX->End());

	
	mGfxStats->display();

	HR(gd3dDevice->EndScene());

	// Present the backbuffer.
	HR(gd3dDevice->Present(0, 0, 0, 0));
}
 
void SpotlightDemo::buildGeoBuffers()
{
	std::vector<D3DXVECTOR3> verts;
	std::vector<DWORD> indices;

	GenTriGrid(100, 100, 1.0f, 1.0f, 
		D3DXVECTOR3(0.0f, 0.0f, 0.0f), verts, indices);

	// Save vertex count and triangle count for DrawIndexedPrimitive arguments.
	mNumGridVertices  = 100*100;
	mNumGridTriangles = 99*99*2;

	// Obtain a pointer to a new vertex buffer.
	HR(gd3dDevice->CreateVertexBuffer(mNumGridVertices * sizeof(VertexPN), 
		D3DUSAGE_WRITEONLY,	0, D3DPOOL_MANAGED, &mVB, 0));

	// Now lock it to obtain a pointer to its internal data, and write the
	// grid's vertex data.
	VertexPN* v = 0;
	HR(mVB->Lock(0, 0, (void**)&v, 0));

	for(DWORD i = 0; i < mNumGridVertices; ++i)
	{
		v[i].pos = verts[i];
		v[i].normal = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
	}

	HR(mVB->Unlock());


	// Obtain a pointer to a new index buffer.
	HR(gd3dDevice->CreateIndexBuffer(mNumGridTriangles*3*sizeof(WORD), D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16, D3DPOOL_MANAGED, &mIB, 0));

	// Now lock it to obtain a pointer to its internal data, and write the
	// grid's index data.

	WORD* k = 0;
	HR(mIB->Lock(0, 0, (void**)&k, 0));

	for(DWORD i = 0; i < mNumGridTriangles*3; ++i)
		k[i] = (WORD)indices[i];

	HR(mIB->Unlock());
}

void SpotlightDemo::buildFX()
{
	// Create the FX from a .fx file.
	ID3DXBuffer* errors = 0;
	HR(D3DXCreateEffectFromFile(gd3dDevice, "spotlight.fx", 
		0, 0, D3DXSHADER_DEBUG, 0, &mFX, &errors));
	if( errors )
		MessageBox(0, (char*)errors->GetBufferPointer(), 0, 0);

	// Obtain handles.
	mhTech           = mFX->GetTechniqueByName("SpotlightTech");
	mhWVP            = mFX->GetParameterByName(0, "gWVP");
	mhWorldInvTrans  = mFX->GetParameterByName(0, "gWorldInvTrans");
	mhEyePos         = mFX->GetParameterByName(0, "gEyePosW");
	mhWorld          = mFX->GetParameterByName(0, "gWorld");
	mhAmbientLight   = mFX->GetParameterByName(0, "gAmbientLight");
	mhDiffuseLight   = mFX->GetParameterByName(0, "gDiffuseLight");
	mhSpecLight      = mFX->GetParameterByName(0, "gSpecLight");
	mhLightPosW      = mFX->GetParameterByName(0, "gLightPosW");
	mhLightDirW      = mFX->GetParameterByName(0, "gLightDirW");
	mhAttenuation012 = mFX->GetParameterByName(0, "gAttenuation012");
	mhAmbientMtrl    = mFX->GetParameterByName(0, "gAmbientMtrl");
	mhDiffuseMtrl    = mFX->GetParameterByName(0, "gDiffuseMtrl");
	mhSpecMtrl       = mFX->GetParameterByName(0, "gSpecMtrl");
	mhSpecPower      = mFX->GetParameterByName(0, "gSpecPower");
	mhSpotPower      = mFX->GetParameterByName(0, "gSpotPower");
}

void SpotlightDemo::buildViewMtx()
{
	float x = mCameraRadius * cosf(mCameraRotationY);
	float z = mCameraRadius * sinf(mCameraRotationY);
	D3DXVECTOR3 pos(x, mCameraHeight, z);
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
	D3DXMatrixLookAtLH(&mView, &pos, &target, &up);

	HR(mFX->SetValue(mhEyePos, &pos, sizeof(D3DXVECTOR3)));

	// Spotlight position is the same as the camera position.
	HR(mFX->SetValue(mhLightPosW, &pos, sizeof(D3DXVECTOR3)));

	// Spotlight direction is the same as the camera forward direction.
	D3DXVECTOR3 lightDir = target - pos;
	D3DXVec3Normalize(&lightDir, &lightDir);
	HR(mFX->SetValue(mhLightDirW, &lightDir, sizeof(D3DXVECTOR3)));
}

void SpotlightDemo::buildProjMtx()
{
	float w = (float)md3dPP.BackBufferWidth;
	float h = (float)md3dPP.BackBufferHeight;
	D3DXMatrixPerspectiveFovLH(&mProj, D3DX_PI * 0.25f, w/h, 1.0f, 5000.0f);
}

void SpotlightDemo::drawGrid()
{
	HR(gd3dDevice->SetStreamSource(0, mVB, 0, sizeof(VertexPN)));
	HR(gd3dDevice->SetIndices(mIB));
	HR(gd3dDevice->SetVertexDeclaration(VertexPN::Decl));

	D3DXMATRIX W, WIT;
	D3DXMatrixIdentity(&W);
	D3DXMatrixInverse(&WIT, 0, &W);
	D3DXMatrixTranspose(&WIT, &WIT);
	HR(mFX->SetMatrix(mhWorld, &W));
	HR(mFX->SetMatrix(mhWVP, &(W*mView*mProj)));
	HR(mFX->SetMatrix(mhWorldInvTrans, &WIT));

	HR(mFX->SetValue(mhAmbientMtrl, &mGridMtrl.ambient, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhDiffuseMtrl, &mGridMtrl.diffuse, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhSpecMtrl, &mGridMtrl.spec, sizeof(D3DXCOLOR)));
	HR(mFX->SetFloat(mhSpecPower, mGridMtrl.specPower));

	HR(mFX->CommitChanges());
	HR(gd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, mNumGridVertices, 0, mNumGridTriangles));
}

void SpotlightDemo::drawCylinders()
{
	D3DXMATRIX T, R, W, WIT;

	D3DXMatrixRotationX(&R, D3DX_PI*0.5f);

	HR(mFX->SetValue(mhAmbientMtrl, &mCylinderMtrl.ambient, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhDiffuseMtrl, &mCylinderMtrl.diffuse, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhSpecMtrl, &mCylinderMtrl.spec, sizeof(D3DXCOLOR)));
	HR(mFX->SetFloat(mhSpecPower, mCylinderMtrl.specPower));
	for(int z = -30; z <= 30; z+= 10)
	{
		D3DXMatrixTranslation(&T, -10.0f, 3.0f, (float)z);
		W = R*T;
		D3DXMatrixInverse(&WIT, 0, &W);
		D3DXMatrixTranspose(&WIT, &WIT);

		HR(mFX->SetMatrix(mhWVP, &(W*mView*mProj)));
		HR(mFX->SetMatrix(mhWorld, &W));
		HR(mFX->SetMatrix(mhWorldInvTrans, &WIT));
		HR(mFX->CommitChanges());
		HR(mCylinder->DrawSubset(0));

		D3DXMatrixTranslation(&T, 10.0f, 3.0f, (float)z);
		W = R*T;
		D3DXMatrixInverse(&WIT, 0, &W);
		D3DXMatrixTranspose(&WIT, &WIT);

		HR(mFX->SetMatrix(mhWVP, &(W*mView*mProj)));
		HR(mFX->SetMatrix(mhWorld, &W));
		HR(mFX->SetMatrix(mhWorldInvTrans, &WIT));
		HR(mFX->CommitChanges());
		HR(mCylinder->DrawSubset(0));
	}
}

void SpotlightDemo::drawSpheres()
{
	D3DXMATRIX W, WIT;

	HR(mFX->SetValue(mhAmbientMtrl, &mSphereMtrl.ambient, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhDiffuseMtrl, &mSphereMtrl.diffuse, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhSpecMtrl, &mSphereMtrl.spec, sizeof(D3DXCOLOR)));
	HR(mFX->SetFloat(mhSpecPower, mSphereMtrl.specPower));
	for(int z = -30; z <= 30; z+= 10)
	{
		D3DXMatrixTranslation(&W, -10.0f, 7.5f, (float)z);
		D3DXMatrixInverse(&WIT, 0, &W);
		D3DXMatrixTranspose(&WIT, &WIT);

		HR(mFX->SetMatrix(mhWVP, &(W*mView*mProj)));
		HR(mFX->SetMatrix(mhWorld, &W));
		HR(mFX->SetMatrix(mhWorldInvTrans, &WIT));
		HR(mFX->CommitChanges());
		HR(mSphere->DrawSubset(0));

		D3DXMatrixTranslation(&W, 10.0f, 7.5f, (float)z);
		D3DXMatrixInverse(&WIT, 0, &W);
		D3DXMatrixTranspose(&WIT, &WIT);

		HR(mFX->SetMatrix(mhWVP, &(W*mView*mProj)));
		HR(mFX->SetMatrix(mhWorld, &W));
		HR(mFX->SetMatrix(mhWorldInvTrans, &WIT));
		HR(mFX->CommitChanges());
		HR(mSphere->DrawSubset(0));
	}
}