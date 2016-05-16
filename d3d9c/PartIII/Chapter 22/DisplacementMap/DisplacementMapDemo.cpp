//=============================================================================
// DisplacementMapDemo.cpp by Frank Luna (C) 2005 All Rights Reserved.
//
// Demonstrates displacement mapping.
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
#include "WaterDmap.h"
#include "Vertex.h"

class DisplacementMapDemo : public D3DApp
{
public:
	DisplacementMapDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP);
	~DisplacementMapDemo();

	bool checkDeviceCaps();
	void onLostDevice();
	void onResetDevice();
	void updateScene(float dt);
	void drawScene();

private:
	GfxStats* mGfxStats;
	Sky* mSky;
	WaterDMap* mWater;
	DirLight mLight;
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

	DisplacementMapDemo app(hInstance, "Displacement Map Demo", D3DDEVTYPE_HAL, D3DCREATE_HARDWARE_VERTEXPROCESSING);
	gd3dApp = &app;

	DirectInput di(DISCL_NONEXCLUSIVE|DISCL_FOREGROUND, DISCL_NONEXCLUSIVE|DISCL_FOREGROUND);
	gDInput = &di;

    return gd3dApp->run();
}

DisplacementMapDemo::DisplacementMapDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP)
: D3DApp(hInstance, winCaption, devType, requestedVP)
{
	if(!checkDeviceCaps())
	{
		MessageBox(0, "checkDeviceCaps() Failed", 0, 0);
		PostQuitMessage(0);
	}

	InitAllVertexDeclarations();

	mLight.dirW = D3DXVECTOR3(0.0f, -1.0f, -3.0f);
	D3DXVec3Normalize(&mLight.dirW, &mLight.dirW);
	mLight.ambient = D3DXCOLOR(0.3f, 0.3f, 0.3f, 1.0f);
	mLight.diffuse = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	mLight.spec    = D3DXCOLOR(0.7f, 0.7f, 0.7f, 1.0f);

	mGfxStats = new GfxStats();
	mSky = new Sky("grassenvmap1024.dds", 10000.0f);

	D3DXMATRIX waterWorld;
	D3DXMatrixIdentity(&waterWorld);
	
	Mtrl waterMtrl;
	waterMtrl.ambient   = D3DXCOLOR(0.4f, 0.4f, 0.7f, 0.0f);
	waterMtrl.diffuse   = D3DXCOLOR(0.4f, 0.4f, 0.7f, 1.0f);
	waterMtrl.spec      = 0.8f*WHITE;
	waterMtrl.specPower = 128.0f;

	WaterDMap::InitInfo waterInitInfo;
	waterInitInfo.dirLight = mLight;
	waterInitInfo.mtrl     = waterMtrl;
	waterInitInfo.fxFilename = "waterdmap.fx";
	waterInitInfo.vertRows         = 128;
	waterInitInfo.vertCols         = 128;
	waterInitInfo.dx               = 0.25f;
	waterInitInfo.dz               = 0.25f;
	waterInitInfo.waveMapFilename0 = "wave0.dds";
	waterInitInfo.waveMapFilename1 = "wave1.dds";
	waterInitInfo.dmapFilename0    = "waterdmap0.dds";
	waterInitInfo.dmapFilename1    = "waterdmap1.dds";
	waterInitInfo.waveNMapVelocity0 = D3DXVECTOR2(0.05f, 0.07f);
	waterInitInfo.waveNMapVelocity1 = D3DXVECTOR2(-0.01f, 0.13f);
	waterInitInfo.waveDMapVelocity0 = D3DXVECTOR2(0.012f, 0.015f);
	waterInitInfo.waveDMapVelocity1 = D3DXVECTOR2(0.014f, 0.05f);
	waterInitInfo.scaleHeights      = D3DXVECTOR2(0.7f, 1.1f);
	waterInitInfo.texScale = 8.0f;
	waterInitInfo.toWorld = waterWorld;
 
	mWater = new WaterDMap(waterInitInfo);
	
	// Initialize camera.
	gCamera->pos().y = 1.0f;
	gCamera->pos().z = -15.0f;
	gCamera->setSpeed(5.0f);

	mGfxStats->addVertices(mWater->getNumVertices());
	mGfxStats->addTriangles(mWater->getNumTriangles());

	mGfxStats->addVertices(mSky->getNumVertices());
	mGfxStats->addTriangles(mSky->getNumTriangles());

	onResetDevice();
}

DisplacementMapDemo::~DisplacementMapDemo()
{
	delete mGfxStats;
	delete mSky;
	delete mWater;

	DestroyAllVertexDeclarations();
}

bool DisplacementMapDemo::checkDeviceCaps()
{
	D3DCAPS9 caps;
	HR(gd3dDevice->GetDeviceCaps(&caps));

	// Check for vertex shader version 3.0 support.
	if( caps.VertexShaderVersion < D3DVS_VERSION(3, 0) )
		return false;

	// Check for pixel shader version 3.0 support.
	if( caps.PixelShaderVersion < D3DPS_VERSION(3, 0) )
		return false;

	return true;
}

void DisplacementMapDemo::onLostDevice()
{
	mGfxStats->onLostDevice();
	mSky->onLostDevice();
	mWater->onLostDevice();
}

void DisplacementMapDemo::onResetDevice()
{
	mGfxStats->onResetDevice();
	mSky->onResetDevice();
	mWater->onResetDevice();

	// The aspect ratio depends on the backbuffer dimensions, which can 
	// possibly change after a reset.  So rebuild the projection matrix.
	float w = (float)md3dPP.BackBufferWidth;
	float h = (float)md3dPP.BackBufferHeight;
	gCamera->setLens(D3DX_PI * 0.25f, w/h, 1.0f, 1000.0f);
}

void DisplacementMapDemo::updateScene(float dt)
{
	mGfxStats->update(dt);

	gDInput->poll();

	// Prevent camera from getting too close to water
	if(gCamera->pos().y < 2.0f)
		gCamera->pos().y = 2.0f;

	gCamera->update(dt, 0, 0);
	

	mWater->update(dt);
}

void DisplacementMapDemo::drawScene()
{
	HR(gd3dDevice->BeginScene());

	mSky->draw();

	mWater->draw();

	mGfxStats->display();

	HR(gd3dDevice->EndScene());

	// Present the backbuffer.
	HR(gd3dDevice->Present(0, 0, 0, 0));
}

