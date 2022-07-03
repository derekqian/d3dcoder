//=============================================================================
// AsteroidsDemo.cpp by Frank Luna (C) 2005 All Rights Reserved.
//
// Demonstrates picking with a simple asteroids type game.
//
// Controls: Use mouse to look and 'W', 'S', 'A', and 'D' keys to move.
//           Left mouse button to pick asteroids.
//=============================================================================

#include "d3dApp.h"
#include "DirectInput.h"
#include <crtdbg.h>
#include "GfxStats.h"
#include <list>
#include <vector> 
#pragma comment(lib, "legacy_stdio_definitions.lib")
#include <ctime>
#include "Terrain.h"
#include "Camera.h"
#include "PSystem.h"

// Create a simple firework particle system which goes off
// whenever the user picks an asteroid.
class FireWork : public PSystem
{
public:
	FireWork(const std::string& fxName, 
		const std::string& techName, 
		const std::string& texName, 
		const D3DXVECTOR3& accel, 
		const AABB& box,
		int maxNumParticles,
		float timePerParticle)
		: PSystem(fxName, techName, texName, accel, box, 
		maxNumParticles, timePerParticle)
	{
		for(int i = 0; i < mMaxNumParticles; ++i)
		{
			initParticle(mParticles[i]);
		}
	}

	void initParticle(Particle& out)
	{
		out.initialTime = 0.0f;
		out.initialSize  = GetRandomFloat(12.0f, 15.0f);
		out.lifeTime = 10.0f;

		// Generate Random Direction
		D3DXVECTOR3 d;
		GetRandomVec(d);

		// Compute velocity.
		float speed = GetRandomFloat(100.0f, 150.0f);
		out.initialVelocity = speed*d;

		out.initialColor = WHITE;
		out.mass = GetRandomFloat(2.0f, 4.0f);

		float r = GetRandomFloat(0.0f, 2.0f);
		out.initialPos = r*d;
	}
};
 
// In order to not duplicate firework systems, we create
// a firework "instance" structure, which stores the position of
// the instance in world space and its relative time since being
// created.  In this way, we can draw several fireworks with only 
// one actual system by drawing the system in different world space
// positions and at different times.
struct FireWorkInstance
{
	float time;
	D3DXMATRIX toWorld;
};

// A simple asteroid structure to maintain the rotation, position, 
// and velocity of an asteroid.  
struct Asteroid
{
	D3DXVECTOR3 axis;
	float theta;
	D3DXVECTOR3 pos;
	D3DXVECTOR3 vel;
};

class AsteroidsDemo : public D3DApp
{
public:
	AsteroidsDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP);
	~AsteroidsDemo();

	bool checkDeviceCaps();
	void onLostDevice();
	void onResetDevice();
	void updateScene(float dt);
	void drawScene();

	void initAsteroids();
	void buildFX();
	void getWorldPickingRay(D3DXVECTOR3& originW, D3DXVECTOR3& dirW);

private:
	GfxStats* mGfxStats;

	// We only need one firework system, as we just draw the same system
	// several times per frame in different positions and at different
	// relative times to simulate multiple systems.
	PSystem* mFireWork;

	// A list of firework *instances*
	std::list<FireWorkInstance> mFireWorkInstances;

	// A list of asteroids.
	static const int NUM_ASTEROIDS = 300;
	std::list<Asteroid> mAsteroids;

	// We only need one actual mesh, as we just draw the same mesh several
	// times per frame in different positions to simulate multiple asteroids.
	ID3DXMesh* mAsteroidMesh;
	std::vector<Mtrl> mAsteroidMtrls;
	std::vector<IDirect3DTexture9*> mAsteroidTextures;
	AABB mAsteroidBox;

	// General light/texture FX
	ID3DXEffect* mFX;
	D3DXHANDLE   mhTech;
	D3DXHANDLE   mhWVP;
	D3DXHANDLE   mhWorldInvTrans;
	D3DXHANDLE   mhEyePos;
	D3DXHANDLE   mhWorld;
	D3DXHANDLE   mhTex;
	D3DXHANDLE   mhMtrl;
	D3DXHANDLE   mhLight;

	DirLight mLight;

	// Default texture if no texture present for subset.
	IDirect3DTexture9* mWhiteTex;
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

	AsteroidsDemo app(hInstance, "Asteroids Demo", D3DDEVTYPE_HAL, D3DCREATE_HARDWARE_VERTEXPROCESSING);
	gd3dApp = &app;

	DirectInput di(DISCL_NONEXCLUSIVE|DISCL_FOREGROUND, DISCL_NONEXCLUSIVE|DISCL_FOREGROUND);
	gDInput = &di;

    return gd3dApp->run();
}

AsteroidsDemo::AsteroidsDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP)
: D3DApp(hInstance, winCaption, devType, requestedVP)
{
	if(!checkDeviceCaps())
	{
		MessageBox(0, "checkDeviceCaps() Failed", 0, 0);
		PostQuitMessage(0);
	}

	InitAllVertexDeclarations();

	mGfxStats = new GfxStats();
	
	// Load the asteroid mesh and compute its bounding box in local space.
	LoadXFile("asteroid.x", &mAsteroidMesh, mAsteroidMtrls, mAsteroidTextures);
	VertexPNT* v = 0;
	HR(mAsteroidMesh->LockVertexBuffer(0, (void**)&v));
	HR(D3DXComputeBoundingBox(&v->pos, mAsteroidMesh->GetNumVertices(),
		mAsteroidMesh->GetNumBytesPerVertex(), 
		&mAsteroidBox.minPt, &mAsteroidBox.maxPt));
	HR(mAsteroidMesh->UnlockVertexBuffer());

	// Initialize camera.
	gCamera->pos() = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	gCamera->setSpeed(40.0f);

	// Initialize the particle system.
	D3DXMATRIX psysWorld;
	D3DXMatrixIdentity(&psysWorld);

	AABB psysBox; 
	psysBox.maxPt = D3DXVECTOR3(INFINITY, INFINITY, INFINITY);
	psysBox.minPt = D3DXVECTOR3(-INFINITY, -INFINITY, -INFINITY);
	mFireWork = new FireWork("fireworks.fx", "FireWorksTech", "bolt.dds", 
		D3DXVECTOR3(0.0f, -9.8f, 0.0f), psysBox, 500, -1.0f);
	mFireWork->setWorldMtx(psysWorld);

	// Call update once to put all particles in the "alive" list.
	mFireWork->update(0.0f);

	// Load the default texture.
	HR(D3DXCreateTextureFromFile(gd3dDevice, "whitetex.dds", &mWhiteTex));

	// Init a light.
	mLight.dirW    = D3DXVECTOR3(0.707f, 0.0f, 0.707f);
	D3DXVec3Normalize(&mLight.dirW, &mLight.dirW);
	mLight.ambient = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	mLight.diffuse = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	mLight.spec    = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	
	buildFX();
	initAsteroids();

	onResetDevice();
}

AsteroidsDemo::~AsteroidsDemo()
{
	delete mGfxStats;
	ReleaseCOM(mWhiteTex);
	ReleaseCOM(mFX);
	delete mFireWork;

	ReleaseCOM(mAsteroidMesh);
	for(UINT i = 0; i < mAsteroidTextures.size(); ++i)
		ReleaseCOM(mAsteroidTextures[i]);
	
	DestroyAllVertexDeclarations();
}

bool AsteroidsDemo::checkDeviceCaps()
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

void AsteroidsDemo::onLostDevice()
{
	mGfxStats->onLostDevice();
	HR(mFX->OnLostDevice());
	mFireWork->onLostDevice();
}

void AsteroidsDemo::onResetDevice()
{
	mGfxStats->onResetDevice();
	HR(mFX->OnResetDevice());
	mFireWork->onResetDevice();

	// The aspect ratio depends on the backbuffer dimensions, which can 
	// possibly change after a reset.  So rebuild the projection matrix.
	float w = (float)md3dPP.BackBufferWidth;
	float h = (float)md3dPP.BackBufferHeight;
	gCamera->setLens(D3DX_PI * 0.25f, w/h, 0.01f, 5000.0f);
}

void AsteroidsDemo::updateScene(float dt)
{
	mGfxStats->update(dt);

	mGfxStats->setTriCount(mAsteroidMesh->GetNumFaces()*mAsteroids.size());
	mGfxStats->setVertexCount(mAsteroidMesh->GetNumVertices()*mAsteroids.size());

	gDInput->poll();

	gCamera->update(dt, 0, 0);

	// Update the asteroids' orientation and position.
	std::list<Asteroid>::iterator asteroidIter = mAsteroids.begin();
	while( asteroidIter != mAsteroids.end() )
	{
		asteroidIter->theta += 4.0f*dt;
		asteroidIter->pos += asteroidIter->vel*dt;
		++asteroidIter;
	}


	// Update and delete dead firework systems.
	std::list<FireWorkInstance>::iterator fireworkIter = mFireWorkInstances.begin();
	while(fireworkIter != mFireWorkInstances.end())
	{
		fireworkIter->time += dt;

		// Kill system after 1 seconds.
		if(fireworkIter->time > 1.0f)
			fireworkIter = mFireWorkInstances.erase(fireworkIter);
		else
			++fireworkIter;
	}
}

void AsteroidsDemo::drawScene()
{
	// Clear the backbuffer and depth buffer.
	HR(gd3dDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff333333, 1.0f, 0));

	HR(gd3dDevice->BeginScene());

	HR(mFX->SetValue(mhEyePos, &gCamera->pos(), sizeof(D3DXVECTOR3)));
	HR(mFX->SetTechnique(mhTech));
	UINT numPasses = 0;
	HR(mFX->Begin(&numPasses, 0));
	HR(mFX->BeginPass(0));

	// Did we pick anything?
	D3DXVECTOR3 originW(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 dirW(0.0f, 0.0f, 0.0f);
	if( gDInput->mouseButtonDown(0) )
	{
		getWorldPickingRay(originW, dirW);
	}

	std::list<Asteroid>::iterator iter = mAsteroids.begin();
	while( iter != mAsteroids.end() )
	{
		// Build world matrix based on current rotation and position settings.
		D3DXMATRIX R, T;
		D3DXMatrixRotationAxis(&R, &iter->axis, iter->theta);
		D3DXMatrixTranslation(&T, iter->pos.x, iter->pos.y, iter->pos.z);

		D3DXMATRIX toWorld = R*T;

		// Transform AABB to world space.
		AABB box;
		mAsteroidBox.xform(toWorld, box);

		// Did we pick it?
		if(D3DXBoxBoundProbe(&box.minPt, &box.maxPt, &originW, &dirW))
		{
			// Create a firework instance.
			FireWorkInstance inst;
			inst.time = 0.0f;
			inst.toWorld = toWorld;
			mFireWorkInstances.push_back(inst);
	
			// Remove asteroid from list and move onto the next node.
			iter = mAsteroids.erase(iter);
			continue;
		}

		// Only draw if AABB is visible.
		if( gCamera->isVisible( box ) )
		{
			HR(mFX->SetMatrix(mhWVP, &(toWorld*gCamera->viewProj())));
			D3DXMATRIX worldInvTrans;
			D3DXMatrixInverse(&worldInvTrans, 0, &toWorld);
			D3DXMatrixTranspose(&worldInvTrans, &worldInvTrans);
			HR(mFX->SetMatrix(mhWorldInvTrans, &worldInvTrans));
			HR(mFX->SetMatrix(mhWorld, &toWorld));

			for(UINT j = 0; j < mAsteroidMtrls.size(); ++j)
			{
				HR(mFX->SetValue(mhMtrl, &mAsteroidMtrls[j], sizeof(Mtrl)));
			
				// If there is a texture, then use.
				if(mAsteroidTextures[j] != 0)
				{
					HR(mFX->SetTexture(mhTex, mAsteroidTextures[j]));
				}

				// But if not, then set a pure white texture.  When the texture color
				// is multiplied by the color from lighting, it is like multiplying by
				// 1 and won't change the color from lighting.
				else
				{
					HR(mFX->SetTexture(mhTex, mWhiteTex));
				}
			
				HR(mFX->CommitChanges());
				HR(mAsteroidMesh->DrawSubset(j));
			}
		}
		++iter;
	}
	 
	HR(mFX->EndPass());
	HR(mFX->End());

	// Draw fireworks.
	std::list<FireWorkInstance>::iterator psysIter = mFireWorkInstances.begin();
	while(psysIter != mFireWorkInstances.end())
	{
		mFireWork->setTime(psysIter->time);
		mFireWork->setWorldMtx(psysIter->toWorld);
		mFireWork->draw();
		++psysIter;
	}

	mGfxStats->display();

	HR(gd3dDevice->EndScene());

	// Present the backbuffer.
	HR(gd3dDevice->Present(0, 0, 0, 0));
}

void AsteroidsDemo::initAsteroids()
{
	Asteroid a;
	for(int i = 0; i < NUM_ASTEROIDS; ++i)
	{
		// Generate a random rotation axis.
		GetRandomVec(a.axis);

		// No rotation to start, but we will rotate as 
		// a function of time.
		a.theta = 0.0f;

		// Random position in world space.
		a.pos.x = GetRandomFloat(-500.0f, 500.0f);
		a.pos.y = GetRandomFloat(-500.0f, 500.0f);
		a.pos.z = GetRandomFloat(-500.0f, 500.0f);

		// Random velocity in world space.
		float speed = GetRandomFloat(10.0f, 20.0f);
		D3DXVECTOR3 dir;
		GetRandomVec(dir);
		a.vel = speed*dir;

		mAsteroids.push_back(a);
	}
}

void AsteroidsDemo::buildFX()
{
	// Create the FX from a .fx file.
	ID3DXBuffer* errors = 0;
	HR(D3DXCreateEffectFromFile(gd3dDevice, "PhongDirLtTex.fx", 
		0, 0, D3DXSHADER_DEBUG, 0, &mFX, &errors));
	if( errors )
		MessageBox(0, (char*)errors->GetBufferPointer(), 0, 0);

	// Obtain handles.
	mhTech            = mFX->GetTechniqueByName("PhongDirLtTexTech");
	mhWVP             = mFX->GetParameterByName(0, "gWVP");
	mhWorldInvTrans   = mFX->GetParameterByName(0, "gWorldInvTrans");
	mhMtrl            = mFX->GetParameterByName(0, "gMtrl");
	mhLight           = mFX->GetParameterByName(0, "gLight");
	mhEyePos          = mFX->GetParameterByName(0, "gEyePosW");
	mhWorld           = mFX->GetParameterByName(0, "gWorld");
	mhTex             = mFX->GetParameterByName(0, "gTex");

	HR(mFX->SetValue(mhLight, &mLight, sizeof(DirLight)));
}

void AsteroidsDemo::getWorldPickingRay(D3DXVECTOR3& originW, D3DXVECTOR3& dirW)
{
	// Get the screen point clicked.
	POINT s;
	GetCursorPos(&s);

	// Make it relative to the client area window.
	ScreenToClient(mhMainWnd, &s);

	// By the way we've been constructing things, the entire 
	// backbuffer is the viewport.

	float w = (float)md3dPP.BackBufferWidth;
	float h = (float)md3dPP.BackBufferHeight;

	D3DXMATRIX proj = gCamera->proj();

	float x = (2.0f*s.x/w - 1.0f) / proj(0,0);
	float y = (-2.0f*s.y/h + 1.0f) / proj(1,1);

	// Build picking ray in view space.
	D3DXVECTOR3 origin(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 dir(x, y, 1.0f);

	// So if the view matrix transforms coordinates from 
	// world space to view space, then the inverse of the
	// view matrix transforms coordinates from view space
	// to world space.
	D3DXMATRIX invView;
	D3DXMatrixInverse(&invView, 0, &gCamera->view());

	// Transform picking ray to world space.
	D3DXVec3TransformCoord(&originW, &origin, &invView);
	D3DXVec3TransformNormal(&dirW, &dir, &invView);
	D3DXVec3Normalize(&dirW, &dirW);
}