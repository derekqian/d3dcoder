//=============================================================================
// ProjTexDemo.cpp by Frank Luna (C) 2005 All Rights Reserved.
//
// Demonstrates projective texturing.
//
// Controls: Use mouse to look and 'W', 'S', 'A', and 'D' keys to move.
//=============================================================================

#include "d3dApp.h"
#include "DirectInput.h"
#include <crtdbg.h>
#include "GfxStats.h"
#include <list>
#include "Camera.h"
#include "Sky.h"
#include "Vertex.h"

struct SpotLight
{
	D3DXCOLOR ambient;
	D3DXCOLOR diffuse;
	D3DXCOLOR spec;
	D3DXVECTOR3 posW;
	D3DXVECTOR3 dirW;  
	float  spotPower;
};

class ProjTexDemo : public D3DApp
{
public:
	ProjTexDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP);
	~ProjTexDemo();

	bool checkDeviceCaps();
	void onLostDevice();
	void onResetDevice();
	void updateScene(float dt);
	void drawScene();
	
	void buildFX();
private:
	GfxStats* mGfxStats;
	 
	Sky* mSky;
	ID3DXMesh* mSceneMesh;
	D3DXMATRIX mSceneWorld;
	std::vector<Mtrl> mSceneMtrls;
	std::vector<IDirect3DTexture9*> mSceneTextures;

	IDirect3DTexture9* mWhiteTex;
	IDirect3DTexture9* mSkullTex;

	D3DXMATRIX mLightWVP;

	ID3DXEffect* mFX;
	D3DXHANDLE   mhTech;
	D3DXHANDLE   mhWVP;
	D3DXHANDLE   mhLightWVP;
	D3DXHANDLE   mhWorldInvTrans;
	D3DXHANDLE   mhEyePosW;
	D3DXHANDLE   mhWorld;
	D3DXHANDLE   mhTex;
	D3DXHANDLE   mhMtrl;
	D3DXHANDLE   mhLight;
 
	SpotLight mLight;
};


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
	#if defined(DEBUG) | defined(_DEBUG)
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	#endif

	// Construct camera before application, since the application uses the camera.
	Camera camera;
	gCamera = &camera;

	ProjTexDemo app(hInstance, "Projective Tex Demo", D3DDEVTYPE_HAL, D3DCREATE_HARDWARE_VERTEXPROCESSING);
	gd3dApp = &app;

	DirectInput di(DISCL_NONEXCLUSIVE|DISCL_FOREGROUND, DISCL_NONEXCLUSIVE|DISCL_FOREGROUND);
	gDInput = &di;

    return gd3dApp->run();
}

ProjTexDemo::ProjTexDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP)
: D3DApp(hInstance, winCaption, devType, requestedVP)
{
	if(!checkDeviceCaps())
	{
		MessageBox(0, "checkDeviceCaps() Failed", 0, 0);
		PostQuitMessage(0);
	}

	InitAllVertexDeclarations();

	mGfxStats = new GfxStats();
	mSky = new Sky("grassenvmap1024.dds", 10000.0f);
 
	LoadXFile("shapes.x", &mSceneMesh, mSceneMtrls, mSceneTextures);
	D3DXMatrixIdentity(&mSceneWorld);

	HR(D3DXCreateTextureFromFile(gd3dDevice, "whitetex.dds", &mWhiteTex));
	HR(D3DXCreateTextureFromFile(gd3dDevice, "skull.dds", &mSkullTex));

	// Build light projective texture matrix.
	D3DXMATRIX lightView;
	D3DXVECTOR3 lightPosW(60.0f, 90.0f, 0.0f);
	D3DXVECTOR3 lightTargetW(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 lightUpW(0.0f, 1.0f, 0.0f);
	D3DXMatrixLookAtLH(&lightView, &lightPosW, &lightTargetW, &lightUpW);
	
	D3DXMATRIX lightLens;
	float lightFOV = D3DX_PI*0.30f;
	D3DXMatrixPerspectiveFovLH(&lightLens, lightFOV, 1.0f, 1.0f, 200.0f);

	mLightWVP = mSceneWorld*lightView*lightLens;

	// Setup a spotlight corresponding to the projector.
	D3DXVECTOR3 lightDirW = lightTargetW - lightPosW;
	D3DXVec3Normalize(&lightDirW, &lightDirW);
	mLight.posW      = lightPosW;
	mLight.dirW      = lightDirW;
	mLight.ambient   = D3DXCOLOR(0.5f, 0.5f, 0.5f, 1.0f);
	mLight.diffuse   = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	mLight.spec      = D3DXCOLOR(0.8f, 0.8f, 0.8f, 1.0f);
	mLight.spotPower = 8.0f;
	
	// Initialize camera.
	gCamera->pos().y = 100.0f;
	gCamera->pos().z = -100.0f;
	gCamera->setSpeed(50.0f);

	mGfxStats->addVertices(mSceneMesh->GetNumVertices());
	mGfxStats->addTriangles(mSceneMesh->GetNumFaces());

	mGfxStats->addVertices(mSky->getNumVertices());
	mGfxStats->addTriangles(mSky->getNumTriangles());

	buildFX();

	onResetDevice();
}

ProjTexDemo::~ProjTexDemo()
{
	delete mGfxStats;
	delete mSky;
	ReleaseCOM(mFX);
	ReleaseCOM(mWhiteTex);
	ReleaseCOM(mSkullTex);

	ReleaseCOM(mSceneMesh);
	for(UINT i = 0; i < mSceneTextures.size(); ++i)
		ReleaseCOM(mSceneTextures[i]);

	DestroyAllVertexDeclarations();
}

bool ProjTexDemo::checkDeviceCaps()
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

void ProjTexDemo::onLostDevice()
{
	mGfxStats->onLostDevice();
	mSky->onLostDevice();
	HR(mFX->OnLostDevice());
}

void ProjTexDemo::onResetDevice()
{
	mGfxStats->onResetDevice();
	mSky->onResetDevice();
	HR(mFX->OnResetDevice());

	// The aspect ratio depends on the backbuffer dimensions, which can 
	// possibly change after a reset.  So rebuild the projection matrix.
	float w = (float)md3dPP.BackBufferWidth;
	float h = (float)md3dPP.BackBufferHeight;
	gCamera->setLens(D3DX_PI * 0.25f, w/h, 1.0f, 2000.0f);
}

void ProjTexDemo::updateScene(float dt)
{
	mGfxStats->update(dt);

	gDInput->poll();

	gCamera->update(dt, 0, 0);
}

void ProjTexDemo::drawScene()
{
	HR(gd3dDevice->BeginScene());

	// Draw sky first--this also replaces our gd3dDevice->Clear call.
	//mSky->draw();
	HR(gd3dDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0));
	
	// Draw the scene mesh.
	HR(mFX->SetTechnique(mhTech));
	HR(mFX->SetMatrix(mhWorldInvTrans, &mSceneWorld));
	HR(mFX->SetMatrix(mhWorld, &mSceneWorld));
	HR(mFX->SetValue(mhLight, &mLight, sizeof(SpotLight)));
	HR(mFX->SetMatrix(mhWVP, &(mSceneWorld*gCamera->viewProj())));
	HR(mFX->SetValue(mhEyePosW, &gCamera->pos(), sizeof(D3DXVECTOR3)));
	HR(mFX->SetTexture(mhTex, mSkullTex));
	HR(mFX->SetMatrix(mhLightWVP, &mLightWVP));

	UINT numPasses = 0;
	HR(mFX->Begin(&numPasses, 0));
	HR(mFX->BeginPass(0));

	for(UINT j = 0; j < mSceneMtrls.size(); ++j)
	{
		HR(mFX->SetValue(mhMtrl, &mSceneMtrls[j], sizeof(Mtrl)));

		HR(mFX->CommitChanges());
		HR(mSceneMesh->DrawSubset(j));
	}

	HR(mFX->EndPass());
	HR(mFX->End());
 
	mGfxStats->display();

	HR(gd3dDevice->EndScene());

	// Present the backbuffer.
	HR(gd3dDevice->Present(0, 0, 0, 0));
}

void ProjTexDemo::buildFX()
{
	 // Create the FX from a .fx file.
	ID3DXBuffer* errors = 0;
	HR(D3DXCreateEffectFromFile(gd3dDevice, "ProjTex.fx", 
		0, 0, D3DXSHADER_DEBUG|D3DXSHADER_SKIPOPTIMIZATION, 0, &mFX, &errors));
	if( errors )
		MessageBox(0, (char*)errors->GetBufferPointer(), 0, 0);

	// Obtain handles.
	mhTech            = mFX->GetTechniqueByName("ProjTexTech");
	mhWVP             = mFX->GetParameterByName(0, "gWVP");
	mhLightWVP        = mFX->GetParameterByName(0, "gLightWVP");
	mhWorldInvTrans   = mFX->GetParameterByName(0, "gWorldInvTrans");
	mhMtrl            = mFX->GetParameterByName(0, "gMtrl");
	mhLight           = mFX->GetParameterByName(0, "gLight");
	mhEyePosW         = mFX->GetParameterByName(0, "gEyePosW");
	mhWorld           = mFX->GetParameterByName(0, "gWorld");
	mhTex             = mFX->GetParameterByName(0, "gTex");
}
