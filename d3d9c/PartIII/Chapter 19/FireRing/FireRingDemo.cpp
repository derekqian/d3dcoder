//=============================================================================
// FireRingDemo.cpp by Frank Luna (C) 2005 All Rights Reserved.
//
// Demonstrates a ring of fire particle system.
//
// Controls: Use mouse to look and 'W', 'S', 'A', and 'D' keys to move.
//=============================================================================

#include "d3dApp.h"
#include "DirectInput.h"
#include <crtdbg.h>
#include "GfxStats.h"
#include <list>
#include <ctime>
#include "Terrain.h"
#include "Camera.h"
#include "PSystem.h"

class FireRing : public PSystem
{
public:
	FireRing(const std::string& fxName, 
		const std::string& techName, 
		const std::string& texName, 
		const D3DXVECTOR3& accel, 
		const AABB& box,
		int maxNumParticles,
		float timePerParticle)
		: PSystem(fxName, techName, texName, accel, box, 
		maxNumParticles, timePerParticle)
	{
	}

	void initParticle(Particle& out)
	{
		// Time particle is created relative to the global running
		// time of the particle system.
		out.initialTime = mTime;

		// Flare lives for 2-4 seconds.
		out.lifeTime   = GetRandomFloat(2.0f, 4.0f);

		// Initial size in pixels.
		out.initialSize  = GetRandomFloat(10.0f, 15.0f);

		// Give a very small initial velocity to give the flares
		// some randomness.
		GetRandomVec(out.initialVelocity);

		// Scalar value used in vertex shader as an amplitude factor.
		out.mass = GetRandomFloat(1.0f, 2.0f);

		// Start color at 50-100% intensity when born for variation.
		out.initialColor = GetRandomFloat(0.5f, 1.0f)*WHITE;

		// Generate random particle on the ring in polar coordinates:
		// random radius and random angle.
		float r = GetRandomFloat(10.0f, 14.0f);
		float t = GetRandomFloat(0, 2.0f*D3DX_PI);

		// Convert to Cartesian coordinates.
		out.initialPos.x = r*cosf(t);
		out.initialPos.y = r*sinf(t);

		// Random depth value in [-1, 1] (depth of the ring)
		out.initialPos.z = GetRandomFloat(-1.0f, 1.0f);
	}
};

class FireRingDemo : public D3DApp
{
public:
	FireRingDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP);
	~FireRingDemo();

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

	FireRingDemo app(hInstance, "Fire Ring Demo", D3DDEVTYPE_HAL, D3DCREATE_HARDWARE_VERTEXPROCESSING);
	gd3dApp = &app;

	DirectInput di(DISCL_NONEXCLUSIVE|DISCL_FOREGROUND, DISCL_NONEXCLUSIVE|DISCL_FOREGROUND);
	gDInput = &di;

    return gd3dApp->run();
}

FireRingDemo::FireRingDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP)
: D3DApp(hInstance, winCaption, devType, requestedVP)
{
	if(!checkDeviceCaps())
	{
		MessageBox(0, "checkDeviceCaps() Failed", 0, 0);
		PostQuitMessage(0);
	}

	InitAllVertexDeclarations();

	mGfxStats = new GfxStats();
	
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
	D3DXMatrixTranslation(&psysWorld, 55.0f, 45.0f, 55.0f);
	AABB psysBox;
	psysBox.minPt = D3DXVECTOR3(-15.0f, -15.0f, -15.0f);
	psysBox.maxPt = D3DXVECTOR3( 15.0f,  15.0f,  15.0f);
	mPSys = new FireRing("firering.fx", "FireRingTech", "torch.dds", 
		D3DXVECTOR3(0.0f, 0.9f, 0.0f), psysBox, 1500, 0.0025f);
	mPSys->setWorldMtx(psysWorld);
	
	mGfxStats->addVertices(mTerrain->getNumVertices());
	mGfxStats->addTriangles(mTerrain->getNumTriangles());

	onResetDevice();
}

FireRingDemo::~FireRingDemo()
{
	delete mGfxStats;
	delete mTerrain;
	delete mPSys;

	DestroyAllVertexDeclarations();
}

bool FireRingDemo::checkDeviceCaps()
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

void FireRingDemo::onLostDevice()
{
	mGfxStats->onLostDevice();
	mTerrain->onLostDevice();
	mPSys->onLostDevice();
}

void FireRingDemo::onResetDevice()
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

void FireRingDemo::updateScene(float dt)
{
	mGfxStats->update(dt);

	gDInput->poll();

	gCamera->update(dt, 0, 0);

	mPSys->update(dt);
}

void FireRingDemo::drawScene()
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
