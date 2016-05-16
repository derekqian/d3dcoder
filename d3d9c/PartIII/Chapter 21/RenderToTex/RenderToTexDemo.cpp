//=============================================================================
// RenderToTexDemo.cpp by Frank Luna (C) 2005 All Rights Reserved.
//
// Demonstrates how to implement a radar by rendering into a texture
// every frame.
//
// Controls: Use mouse to look and 'W', 'S', 'A', and 'D' keys to move.
//=============================================================================

#include "d3dApp.h"
#include "DirectInput.h"
#include <crtdbg.h>
#include "GfxStats.h"
#include <list>
#include "Terrain.h"
#include "Camera.h"
#include "DrawableTex2D.h"
#include "Vertex.h"
#include "Sky.h"

class RenderToTexDemo : public D3DApp
{
public:
	RenderToTexDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP);
	~RenderToTexDemo();

	bool checkDeviceCaps();
	void onLostDevice();
	void onResetDevice();
	void updateScene(float dt);
	void drawScene();

	void buildFX();

private:
	GfxStats* mGfxStats;
	Terrain*  mTerrain;
	Sky* mSky;

	// Two camera's for this demo.
	Camera mFirstPersonCamera;
	Camera mBirdsEyeCamera;

	// The texture we draw into.
	DrawableTex2D* mRadarMap;
	bool mAutoGenMips;

	IDirect3DVertexBuffer9* mRadarVB;

	// General light/texture FX
	ID3DXEffect* mRadarFX;
	D3DXHANDLE   mhTech;
	D3DXHANDLE   mhTex;
};


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
	#if defined(DEBUG) | defined(_DEBUG)
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	#endif

	RenderToTexDemo app(hInstance, "RenderToTex Demo", D3DDEVTYPE_HAL, D3DCREATE_HARDWARE_VERTEXPROCESSING);
	gd3dApp = &app;

	DirectInput di(DISCL_NONEXCLUSIVE|DISCL_FOREGROUND, DISCL_NONEXCLUSIVE|DISCL_FOREGROUND);
	gDInput = &di;

	return gd3dApp->run();
}

RenderToTexDemo::RenderToTexDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP)
: D3DApp(hInstance, winCaption, devType, requestedVP)
{
	mAutoGenMips = true;

	if(!checkDeviceCaps())
	{
		MessageBox(0, "checkDeviceCaps() Failed", 0, 0);
		PostQuitMessage(0);
	}

	gCamera = &mFirstPersonCamera;

	InitAllVertexDeclarations();

	mGfxStats = new GfxStats();
	mSky = new Sky("grassenvmap1024.dds", 10000.0f);

	// Viewport is entire texture.
	D3DVIEWPORT9 vp = {0, 0, 256, 256, 0.0f, 1.0f};
	mRadarMap = new DrawableTex2D(256, 256, 0, D3DFMT_X8R8G8B8, true, D3DFMT_D24X8, vp, mAutoGenMips);
	HR(gd3dDevice->CreateVertexBuffer(6*sizeof(VertexPT), D3DUSAGE_WRITEONLY,
		0, D3DPOOL_MANAGED, &mRadarVB, 0));

	// Radar quad takes up quadrant IV.  Note that we specify coordinate directly in
	// normalized device coordinates.  I.e., world, view, projection matrices are all
	// identity.
	VertexPT* v = 0;
	HR(mRadarVB->Lock(0, 0, (void**)&v, 0));
	v[0] = VertexPT(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[1] = VertexPT(1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[2] = VertexPT(0.0f, -1.0f, 0.0f, 0.0f, 1.0f);
	v[3] = VertexPT(0.0f, -1.0f, 0.0f, 0.0f, 1.0f);
	v[4] = VertexPT(1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[5] = VertexPT(1.0f, -1.0f, 0.0f, 1.0f, 1.0f);
	HR(mRadarVB->Unlock());

	mTerrain = new Terrain(513, 513, 4.0f, 4.0f, 
		"coastMountain513.raw",  
		"grass.dds",
		"dirt.dds",
		"rock.dds",
		"blend_hm17.dds",
		1.5f, 0.0f);

	D3DXVECTOR3 toSun(1.0f, 1.0f, 1.0f);
	D3DXVec3Normalize(&toSun, &toSun);
	mTerrain->setDirToSunW(toSun);

	// Initialize camera.
	gCamera->pos().y = 250.0f;
	gCamera->setSpeed(50.0f);

	mGfxStats->addVertices(mTerrain->getNumVertices());
	mGfxStats->addTriangles(mTerrain->getNumTriangles());

	buildFX();

	onResetDevice();
}

RenderToTexDemo::~RenderToTexDemo()
{
	delete mGfxStats;
	delete mTerrain;
	delete mSky;
	delete mRadarMap;
	ReleaseCOM(mRadarVB);
	ReleaseCOM(mRadarFX);

	DestroyAllVertexDeclarations();
}

bool RenderToTexDemo::checkDeviceCaps()
{
	D3DCAPS9 caps;
	HR(gd3dDevice->GetDeviceCaps(&caps));

	// Check for vertex shader version 2.0 support.
	if( caps.VertexShaderVersion < D3DVS_VERSION(2, 0) )
		return false;

	// Check for pixel shader version 2.0 support.
	if( caps.PixelShaderVersion < D3DPS_VERSION(2, 0) )
		return false;

	// Check render target support.  The adapter format can be either the display mode format
	// for windowed mode, or D3DFMT_X8R8G8B8 for fullscreen mode, so we need to test against
	// both.  We use D3DFMT_X8R8G8B8 as the render texture format and D3DFMT_D24X8 as the 
	// render texture depth format.
	D3DDISPLAYMODE mode;
	md3dObject->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &mode);

	// Windowed.
	if(FAILED(md3dObject->CheckDeviceFormat(D3DADAPTER_DEFAULT, mDevType, mode.Format, 
		D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, D3DFMT_X8R8G8B8)))
		return false;
	if(FAILED(md3dObject->CheckDepthStencilMatch(D3DADAPTER_DEFAULT, mDevType, mode.Format,
		D3DFMT_X8R8G8B8, D3DFMT_D24X8)))
		return false;

	// Fullscreen.
	if(FAILED(md3dObject->CheckDeviceFormat(D3DADAPTER_DEFAULT, mDevType, D3DFMT_X8R8G8B8, 
		D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, D3DFMT_X8R8G8B8)))
		return false;
	if(FAILED(md3dObject->CheckDepthStencilMatch(D3DADAPTER_DEFAULT, mDevType, D3DFMT_X8R8G8B8,
		D3DFMT_X8R8G8B8, D3DFMT_D24X8)))
		return false;
 
	if( caps.Caps2 & D3DCAPS2_CANAUTOGENMIPMAP )
	{
		HRESULT hr = D3D_OK;

		// Windowed.
		hr = md3dObject->CheckDeviceFormat(D3DADAPTER_DEFAULT, 
			D3DDEVTYPE_HAL, mode.Format, D3DUSAGE_AUTOGENMIPMAP,
			D3DRTYPE_TEXTURE, D3DFMT_X8R8G8B8);
		if(hr == D3DOK_NOAUTOGEN)
			mAutoGenMips = false;

		// Fullscreen.
		hr = md3dObject->CheckDeviceFormat(D3DADAPTER_DEFAULT, 
			D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DUSAGE_AUTOGENMIPMAP,
			D3DRTYPE_TEXTURE, D3DFMT_X8R8G8B8);
		if(hr == D3DOK_NOAUTOGEN)
			mAutoGenMips = false;

	}

	return true;
}

void RenderToTexDemo::onLostDevice()
{
	mGfxStats->onLostDevice();
	mTerrain->onLostDevice();
	mRadarMap->onLostDevice();
	mSky->onLostDevice();
	HR(mRadarFX->OnLostDevice());
}

void RenderToTexDemo::onResetDevice()
{
	mGfxStats->onResetDevice();
	mTerrain->onResetDevice();
	mRadarMap->onResetDevice();
	mSky->onResetDevice();
	HR(mRadarFX->OnResetDevice());

	// The aspect ratio depends on the backbuffer dimensions, which can 
	// possibly change after a reset.  So rebuild the projection matrix.
	float w = (float)md3dPP.BackBufferWidth;
	float h = (float)md3dPP.BackBufferHeight;
	mFirstPersonCamera.setLens(D3DX_PI * 0.25f, w/h, 1.0f, 2000.0f);
	mBirdsEyeCamera.setLens(D3DX_PI * 0.25f, w/h, 1.0f, 2000.0f);
}

void RenderToTexDemo::updateScene(float dt)
{
	mGfxStats->update(dt);

	gDInput->poll();

	gCamera->update(dt, mTerrain, 5.5f);
}

void RenderToTexDemo::drawScene()
{
	// Draw into radar map.
	D3DXVECTOR3 pos(gCamera->pos().x, gCamera->pos().y + 1000.0f, gCamera->pos().z);
	D3DXVECTOR3 up(0.0f, 0.0f, 1.0f);
	mBirdsEyeCamera.lookAt(pos, gCamera->pos(), up);

	mRadarMap->beginScene();
	gd3dDevice->Clear(0, 0, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xff000000, 1.0f, 0);
	gCamera = &mBirdsEyeCamera;
	mTerrain->draw();
	mRadarMap->endScene();
	gCamera = &mFirstPersonCamera;

	HR(gd3dDevice->BeginScene());
	mSky->draw();

	mTerrain->draw();

	HR(gd3dDevice->SetStreamSource(0, mRadarVB, 0, sizeof(VertexPT)));
	HR(gd3dDevice->SetVertexDeclaration (VertexPT::Decl));

	HR(mRadarFX->SetTexture(mhTex, mRadarMap->d3dTex()));
	UINT numPasses = 0;
	HR(mRadarFX->Begin(&numPasses, 0));
	HR(mRadarFX->BeginPass(0));
	HR(gd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2));
	HR(mRadarFX->EndPass());
	HR(mRadarFX->End());

	mGfxStats->display();

	HR(gd3dDevice->EndScene());

	// Present the backbuffer.
	HR(gd3dDevice->Present(0, 0, 0, 0));
}

void RenderToTexDemo::buildFX()
{
	// Create the generic Light & Tex FX from a .fx file.
	ID3DXBuffer* errors = 0;
	HR(D3DXCreateEffectFromFile(gd3dDevice, "Radar.fx", 
		0, 0, 0, 0, &mRadarFX, &errors));
	if( errors )
		MessageBox(0, (char*)errors->GetBufferPointer(), 0, 0);

	// Obtain handles.
	mhTech = mRadarFX->GetTechniqueByName("RadarTech");
	mhTex  = mRadarFX->GetParameterByName(0, "gTex");
}