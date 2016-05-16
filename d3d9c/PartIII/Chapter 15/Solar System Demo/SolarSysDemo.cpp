//=============================================================================
// SolarSysDemo.cpp by Frank Luna (C) 2005 All Rights Reserved.
//
// Another demonstrates of how to animate mesh hierarchies.
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

// We classify the objects in our scene as one of three types.
enum SolarType
{
	SUN,
	PLANET,
	MOON
};

struct SolarObject
{
	void set(SolarType type, D3DXVECTOR3 p, float yRot, int parentIndex, float s, IDirect3DTexture9* t)
	{
		typeID = type;
		pos    = p;
		yAngle = yRot;
		parent = parentIndex;
		size   = s;
		tex    = t;
	}

	// Note: The root's "parent" frame is the world space.

	SolarType typeID;
	D3DXVECTOR3 pos;  // Relative to parent frame.
	float yAngle;     // Relative to parent frame.
	int parent;       // Index to parent frame (-1 if root, i.e., no parent)
	float size;       // Relative to world frame.
	IDirect3DTexture9* tex;
	D3DXMATRIX toParentXForm;
	D3DXMATRIX toWorldXForm;
};

class SolarSysDemo : public D3DApp
{
public:
	SolarSysDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP);
	~SolarSysDemo();

	bool checkDeviceCaps();
	void onLostDevice();
	void onResetDevice();
	void updateScene(float dt);
	void drawScene();

	// Helper methods
	void buildFX();
	void buildViewMtx();
	void buildProjMtx();

	void buildObjectWorldTransforms();

	void genSphericalTexCoords();

private:
	GfxStats* mGfxStats;
	
	// We only need one sphere mesh.  To draw several solar objects we just 
	// draw the same mesh several times, but with a different transformation
	// applied so that it is drawn in a different place.
	ID3DXMesh* mSphere;
 
	static const int NUM_OBJECTS = 10;
	SolarObject mObject[NUM_OBJECTS];
	
	IDirect3DTexture9* mSunTex;
	IDirect3DTexture9* mPlanet1Tex;
	IDirect3DTexture9* mPlanet2Tex;
	IDirect3DTexture9* mPlanet3Tex;
	IDirect3DTexture9* mMoonTex;

	// Use a white material--the color will come from the texture.
	Mtrl mWhiteMtrl;

	DirLight mLight;

	ID3DXEffect* mFX;
	D3DXHANDLE   mhTech;
	D3DXHANDLE   mhWVP;
	D3DXHANDLE   mhWorldInvTrans;
	D3DXHANDLE   mhEyePos;
	D3DXHANDLE   mhWorld;
	D3DXHANDLE   mhTex;
	D3DXHANDLE   mhMtrl;
	D3DXHANDLE   mhLight;

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

	SolarSysDemo app(hInstance, "Solar System Demo", D3DDEVTYPE_HAL, D3DCREATE_HARDWARE_VERTEXPROCESSING);
	gd3dApp = &app;

	DirectInput di(DISCL_NONEXCLUSIVE|DISCL_FOREGROUND, DISCL_NONEXCLUSIVE|DISCL_FOREGROUND);
	gDInput = &di;

    return gd3dApp->run();
}

SolarSysDemo::SolarSysDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP)
: D3DApp(hInstance, winCaption, devType, requestedVP)
{
	if(!checkDeviceCaps())
	{
		MessageBox(0, "checkDeviceCaps() Failed", 0, 0);
		PostQuitMessage(0);
	}

	InitAllVertexDeclarations();

	mGfxStats = new GfxStats();
	
	// Initialize Camera Settings
	mCameraRadius    = 25.0f;
	mCameraRotationY = 1.2f * D3DX_PI;
	mCameraHeight    = 10.0f;

	// Setup a directional light.
	mLight.dirW    = D3DXVECTOR3(0.0f, 1.0f, 2.0f);
	D3DXVec3Normalize(&mLight.dirW, &mLight.dirW);
	mLight.ambient = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	mLight.diffuse = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	mLight.spec    = D3DXCOLOR(0.6f, 0.6f, 0.6f, 1.0f);

	// Create a sphere to represent a solar object.
	HR(D3DXCreateSphere(gd3dDevice, 1.0f, 30, 30, &mSphere, 0));
	genSphericalTexCoords();
	D3DXMatrixIdentity(&mWorld);

	// Create the textures.
	HR(D3DXCreateTextureFromFile(gd3dDevice, "sun.dds", &mSunTex));
	HR(D3DXCreateTextureFromFile(gd3dDevice, "planet1.dds", &mPlanet1Tex));
	HR(D3DXCreateTextureFromFile(gd3dDevice, "planet2.dds", &mPlanet2Tex));
	HR(D3DXCreateTextureFromFile(gd3dDevice, "planet3.dds", &mPlanet3Tex));
	HR(D3DXCreateTextureFromFile(gd3dDevice, "moon.dds", &mMoonTex));

	// Initialize default white material.
	mWhiteMtrl.ambient = WHITE;
	mWhiteMtrl.diffuse = WHITE;
	mWhiteMtrl.spec    = WHITE * 0.5f;
	mWhiteMtrl.specPower = 48.0f;

	//==================================================
	// Specify how the solar object frames are related.

	D3DXVECTOR3 pos[NUM_OBJECTS] = 
	{
		D3DXVECTOR3(0.0f, 0.0f, 0.0f),
		D3DXVECTOR3(7.0f, 0.0f, 7.0f),
		D3DXVECTOR3(-9.0f, 0.0f, 0.0f),
		D3DXVECTOR3(7.0f, 0.0f, -6.0f),
		D3DXVECTOR3(5.0f, 0.0f, 0.0f),
		D3DXVECTOR3(-5.0f, 0.0f, 0.0f),
		D3DXVECTOR3(3.0f, 0.0f, 0.0f),
		D3DXVECTOR3(2.0f, 0.0f, -2.0f),
		D3DXVECTOR3(-2.0f, 0.0f, 0.0f),
		D3DXVECTOR3(0.0f, 0.0f, 2.0f)
	};

	mObject[0].set(SUN, pos[0], 0.0f, -1, 2.5f, mSunTex);  // Sun
	mObject[1].set(PLANET, pos[1], 0.0f, 0, 1.5f, mPlanet1Tex);// P1
	mObject[2].set(PLANET, pos[2], 0.0f, 0, 1.2f, mPlanet2Tex);// P2
	mObject[3].set(PLANET, pos[3], 0.0f, 0, 0.8f, mPlanet3Tex);// P3

	mObject[4].set(MOON, pos[4], 0.0f, 1, 0.5f, mMoonTex); // M1P1
	mObject[5].set(MOON, pos[5], 0.0f, 1, 0.5f, mMoonTex); // M2P1
	mObject[6].set(MOON, pos[6], 0.0f, 2, 0.4f, mMoonTex); // M1P2
	mObject[7].set(MOON, pos[7], 0.0f, 3, 0.3f, mMoonTex); // M1P3
	mObject[8].set(MOON, pos[8], 0.0f, 3, 0.3f, mMoonTex); // M2P3
	mObject[9].set(MOON, pos[9], 0.0f, 3, 0.3f, mMoonTex); // M3P3


	//==================================================
 
	mGfxStats->addVertices(mSphere->GetNumVertices() * NUM_OBJECTS);
	mGfxStats->addTriangles(mSphere->GetNumFaces() * NUM_OBJECTS);

	buildFX();

	onResetDevice();
}

SolarSysDemo::~SolarSysDemo()
{
	delete mGfxStats;
	
	ReleaseCOM(mFX);
	ReleaseCOM(mSphere);
	ReleaseCOM(mSunTex);
	ReleaseCOM(mPlanet1Tex);
	ReleaseCOM(mPlanet2Tex);
	ReleaseCOM(mPlanet3Tex);
	ReleaseCOM(mMoonTex);

	DestroyAllVertexDeclarations();
}

bool SolarSysDemo::checkDeviceCaps()
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

void SolarSysDemo::onLostDevice()
{
	mGfxStats->onLostDevice();
	HR(mFX->OnLostDevice());
}

void SolarSysDemo::onResetDevice()
{
	mGfxStats->onResetDevice();
	HR(mFX->OnResetDevice());

	// The aspect ratio depends on the backbuffer dimensions, which can 
	// possibly change after a reset.  So rebuild the projection matrix.
	buildProjMtx();
}

void SolarSysDemo::updateScene(float dt)
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
	if( mCameraRadius < 2.0f )
		mCameraRadius = 2.0f;

	// The camera position/orientation relative to world space can 
	// change every frame based on input, so we need to rebuild the
	// view matrix every frame with the latest changes.
	buildViewMtx();

	//================================================
	// Animate the solar objects with respect to time.

	for(int i = 0; i < NUM_OBJECTS; ++i)
	{
		switch(mObject[i].typeID)
		{
		case SUN:
			mObject[i].yAngle += 1.5f * dt;
			break;
		case PLANET:
			mObject[i].yAngle += 2.0f * dt;
			break;
		case MOON:
			mObject[i].yAngle += 2.5f * dt;
			break;
		}

		// If we rotate over 360 degrees, just roll back to 0.
		if(mObject[i].yAngle >= 2.0f*D3DX_PI)
			mObject[i].yAngle = 0.0f;
	}
}


void SolarSysDemo::drawScene()
{
	// Clear the backbuffer and depth buffer.
	HR(gd3dDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff000000, 1.0f, 0));

	HR(gd3dDevice->BeginScene());

	HR(mFX->SetValue(mhLight, &mLight, sizeof(DirLight)));

	HR(mFX->SetTechnique(mhTech));
	UINT numPasses = 0;
	HR(mFX->Begin(&numPasses, 0));
	HR(mFX->BeginPass(0));

	// Wrap the texture coordinates that get assigned to TEXCOORD2 in the pixel shader.
	HR(gd3dDevice->SetRenderState(D3DRS_WRAP2, D3DWRAP_U));

	// Build the world transforms for each frame, then render them.
	buildObjectWorldTransforms();
	D3DXMATRIX S;
	for(int i = 0; i < NUM_OBJECTS; ++i)
	{
		float s = mObject[i].size;
		D3DXMatrixScaling(&S, s, s, s);

		// Prefix the frame matrix with a scaling transformation to
		// size it relative to the world.
		mWorld = S * mObject[i].toWorldXForm;
		HR(mFX->SetMatrix(mhWVP, &(mWorld*mView*mProj)));
		D3DXMATRIX worldInvTrans;
		D3DXMatrixInverse(&worldInvTrans, 0, &mWorld);
		D3DXMatrixTranspose(&worldInvTrans, &worldInvTrans);
		HR(mFX->SetMatrix(mhWorldInvTrans, &worldInvTrans));
		HR(mFX->SetMatrix(mhWorld, &mWorld));
		HR(mFX->SetValue(mhMtrl, &mWhiteMtrl, sizeof(Mtrl)));
		HR(mFX->SetTexture(mhTex, mObject[i].tex));	
		HR(mFX->CommitChanges());
		
		mSphere->DrawSubset(0);
	}
	HR(gd3dDevice->SetRenderState(D3DRS_WRAP2, 0));
	HR(mFX->EndPass());
	HR(mFX->End());
	
	mGfxStats->display();

	HR(gd3dDevice->EndScene());

	// Present the backbuffer.
	HR(gd3dDevice->Present(0, 0, 0, 0));
}

void SolarSysDemo::buildFX()
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
}

void SolarSysDemo::buildViewMtx()
{
	float x = mCameraRadius * cosf(mCameraRotationY);
	float z = mCameraRadius * sinf(mCameraRotationY);
	D3DXVECTOR3 pos(x, mCameraHeight, z);
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
	D3DXMatrixLookAtLH(&mView, &pos, &target, &up);

	HR(mFX->SetValue(mhEyePos, &pos, sizeof(D3DXVECTOR3)));
}

void SolarSysDemo::buildProjMtx()
{
	float w = (float)md3dPP.BackBufferWidth;
	float h = (float)md3dPP.BackBufferHeight;
	D3DXMatrixPerspectiveFovLH(&mProj, D3DX_PI * 0.25f, w/h, 1.0f, 5000.0f);
}

void SolarSysDemo::buildObjectWorldTransforms()
{
	// First, construct the transformation matrix that transforms
	// the ith bone into the coordinate system of its parent.

	D3DXMATRIX R, T;
	D3DXVECTOR3 p;
	for(int i = 0; i < NUM_OBJECTS; ++i) 
	{
		p = mObject[i].pos;
		D3DXMatrixRotationY(&R, mObject[i].yAngle);
		D3DXMatrixTranslation(&T, p.x, p.y, p.z);
		mObject[i].toParentXForm = R * T;
	}

	// For each object...
	for(int i = 0; i < NUM_OBJECTS; ++i)
	{
		// Initialize to identity matrix.
		D3DXMatrixIdentity(&mObject[i].toWorldXForm);

		// The ith object's world transform is given by its 
		// to-parent transform, followed by its parent's to-parent transform, 
		// followed by its grandparent's to-parent transform, and
		// so on, up to the root's to-parent transform.
		int k = i;
		while( k != -1 )
		{
			mObject[i].toWorldXForm *= mObject[k].toParentXForm;
			k = mObject[k].parent;
		}
	}
}

void SolarSysDemo::genSphericalTexCoords()
{
	// D3DXCreate* functions generate vertices with position 
	// and normal data.  But for texturing, we also need
	// tex-coords.  So clone the mesh to change the vertex
	// format to a format with tex-coords.

	D3DVERTEXELEMENT9 elements[64];
	UINT numElements = 0;
	VertexPNT::Decl->GetDeclaration(elements, &numElements);

	ID3DXMesh* temp = 0;
	HR(mSphere->CloneMesh(D3DXMESH_SYSTEMMEM, 
		elements, gd3dDevice, &temp));

	ReleaseCOM(mSphere);

	// Now generate texture coordinates for each vertex.
	VertexPNT* vertices = 0;
	HR(temp->LockVertexBuffer(0, (void**)&vertices));

	for(UINT i = 0; i < temp->GetNumVertices(); ++i)
	{
		// Convert to spherical coordinates.
		D3DXVECTOR3 p = vertices[i].pos;
	
 
		float theta = atan2f(p.z, p.x);
		float phi   = acosf(p.y / sqrtf(p.x*p.x+p.y*p.y+p.z*p.z));

		// Phi and theta give the texture coordinates, but are not in 
		// the range [0, 1], so scale them into that range.

		float u = theta / (2.0f*D3DX_PI);
		float v = phi   / D3DX_PI;
		
		// Save texture coordinates.
		
		vertices[i].tex0.x = u;
		vertices[i].tex0.y = v;
	}
	HR(temp->UnlockVertexBuffer());

	// Clone back to a hardware mesh.
	HR(temp->CloneMesh(D3DXMESH_MANAGED | D3DXMESH_WRITEONLY,
		elements, gd3dDevice, &mSphere));

	ReleaseCOM(temp);
}