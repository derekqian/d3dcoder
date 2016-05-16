//=============================================================================
// GunDemo.cpp by Frank Luna (C) 2005 All Rights Reserved.
//
// Demonstrates a gun particle system.
//
// Controls: Use mouse to look and 'W', 'S', 'A', and 'D' keys to move.
//           Spacebar to fire.
//=============================================================================

#include "d3dApp.h"
#include "DirectInput.h"
#include <crtdbg.h>
#include "GfxStats.h"
#include <list>
#include <vector>
#include <ctime>
#include "Terrain.h"
#include "Camera.h"
#include "PSystem.h"

class Gun : public PSystem
{
public:
	Gun(const std::string& fxName, 
		const std::string& techName, 
		const std::string& texName, 
		const D3DXVECTOR3& accel, 
		const AABB& box,
		int maxNumParticles,
		float timePerParticle)
		: PSystem(fxName, techName, texName, accel, box, maxNumParticles,
		timePerParticle)
	{
	}

	void initParticle(Particle& out)
	{
		// Generate at camera.
		out.initialPos = gCamera->pos();

		// Set down a bit so it looks like player is carrying the gun.
		out.initialPos.y -= 3.0f;

		// Fire in camera's look direction.
		float speed = 500.0f;
		out.initialVelocity = speed*gCamera->look();
 
		out.initialTime      = mTime;
		out.lifeTime        = 4.0f;
		out.initialColor    = WHITE;
		out.initialSize     = GetRandomFloat(80.0f, 90.0f);
		out.mass            = 1.0f;
	}
};

class GunDemo : public D3DApp
{
public:
	GunDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP);
	~GunDemo();

	bool checkDeviceCaps();
	void onLostDevice();
	void onResetDevice();
	void updateScene(float dt);
	void drawScene();

private:
	GfxStats* mGfxStats;
	Terrain*  mTerrain;
	PSystem*  mPSys;
};


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
	#if defined(DEBUG) | defined(_DEBUG)
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	#endif

	srand(time(0));

	// Construct camera before application, since the application uses the camera.
	Camera camera;
	gCamera = &camera;

	GunDemo app(hInstance, "Gun Demo", D3DDEVTYPE_HAL, D3DCREATE_HARDWARE_VERTEXPROCESSING);
	gd3dApp = &app;

	DirectInput di(DISCL_NONEXCLUSIVE|DISCL_FOREGROUND, DISCL_NONEXCLUSIVE|DISCL_FOREGROUND);
	gDInput = &di;

    return gd3dApp->run();
}

GunDemo::GunDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP)
: D3DApp(hInstance, winCaption, devType, requestedVP)
{
	if(!checkDeviceCaps())
	{
		MessageBox(0, "checkDeviceCaps() Failed", 0, 0);
		PostQuitMessage(0);
	}

	InitAllVertexDeclarations();

	mGfxStats = new GfxStats();
	
	// World space units are meters.  
	mTerrain = new Terrain(257, 257, 2.0f, 2.0f, 
		"heightmap1_257.raw",  
		"mud.dds",
		"stone.dds",
		"snow.dds",
		"blend_hm1.dds",
		0.4f, 0.0f);

	D3DXVECTOR3 toSun(1.0f, 1.0f, 1.0f);
	D3DXVec3Normalize(&toSun, &toSun);
	mTerrain->setDirToSunW(toSun);

	// Initialize camera.
	gCamera->pos() = D3DXVECTOR3(55.0f, 50.0f, 25.0f);
	gCamera->setSpeed(40.0f);

	// Initialize the particle system.
	D3DXMATRIX psysWorld;
	D3DXMatrixIdentity(&psysWorld);

	// Gun bullets spread over a long distance and there is not
	// a high bullet count, so not really worth trying to cull.
	AABB psysBox; 
	psysBox.maxPt = D3DXVECTOR3(INFINITY, INFINITY, INFINITY);
	psysBox.minPt = D3DXVECTOR3(-INFINITY, -INFINITY, -INFINITY);

	// Accelerate due to gravity.  However, since the bullets travel at 
	// such a high velocity, the effect of gravity of not really observed.
	mPSys = new Gun("gun.fx", "GunTech", "bolt.dds", 
		D3DXVECTOR3(0, -9.8f, 0.0f), psysBox, 100, -1.0f);
	mPSys->setWorldMtx(psysWorld);              

	mGfxStats->addVertices(mTerrain->getNumVertices());
	mGfxStats->addTriangles(mTerrain->getNumTriangles());

	onResetDevice();
}

GunDemo::~GunDemo()
{
	delete mGfxStats;
	delete mTerrain;
	delete mPSys;

	DestroyAllVertexDeclarations();
}

bool GunDemo::checkDeviceCaps()
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

void GunDemo::onLostDevice()
{
	mGfxStats->onLostDevice();
	mTerrain->onLostDevice();
	mPSys->onLostDevice();
}

void GunDemo::onResetDevice()
{
	mGfxStats->onResetDevice();
	mTerrain->onResetDevice();
	mPSys->onResetDevice();


	// The aspect ratio depends on the backbuffer dimensions, which can 
	// possibly change after a reset.  So rebuild the projection matrix.
	float w = (float)md3dPP.BackBufferWidth;
	float h = (float)md3dPP.BackBufferHeight;
	gCamera->setLens(D3DX_PI * 0.25f, w/h, 0.01f, 5000.0f);
}

void GunDemo::updateScene(float dt)
{
	mGfxStats->update(dt);

	gDInput->poll();

	gCamera->update(dt, 0, 0);

	mPSys->update(dt);

	// Can only fire once every tenth of a second.
	static float delay = 0.0f;
	if( gDInput->keyDown(DIK_SPACE) && delay <= 0.0f)
	{
		delay = 0.1f;
		mPSys->addParticle();
	}
	delay -= dt;
}

void GunDemo::drawScene()
{
	// Clear the backbuffer and depth buffer.
	HR(gd3dDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff666666, 1.0f, 0));

	HR(gd3dDevice->BeginScene());

	mTerrain->draw();
	mPSys->draw();

	mGfxStats->display();

	HR(gd3dDevice->EndScene());

	// Present the backbuffer.
	HR(gd3dDevice->Present(0, 0, 0, 0));
}
