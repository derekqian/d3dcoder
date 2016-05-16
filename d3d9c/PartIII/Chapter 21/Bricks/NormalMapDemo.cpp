//=============================================================================
// NormalMapDemo.cpp by Frank Luna (C) 2005 All Rights Reserved.
//
// Demonstrates how to do per-pixel lighting using normal maps
//   instead of interpolated vertex normals.
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

class NormalMapDemo : public D3DApp
{
public:
	NormalMapDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP);
	~NormalMapDemo();

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
	D3DXMATRIX mSceneWorldInv;
	std::vector<Mtrl> mSceneMtrls;
	std::vector<IDirect3DTexture9*> mSceneTextures;

	// Hack for this particular scene--usually you'd want to come up
	// with a more general method of loading normal maps such that
	// the ith normal map corresponds with the ith mesh subset.
	// For example, you might call each color texture name_color and 
	// its corresponding normal map name_nmap.  Then when you load the
	// name_color texture you also load the corresponding normal map.
	// If a texture doesn't have a normal map, you could use a default one
	// like we use the default white texture.
	IDirect3DTexture9* mSceneNormalMaps[2];

	IDirect3DTexture9* mWhiteTex;

	ID3DXEffect* mFX;
	D3DXHANDLE   mhTech;
	D3DXHANDLE   mhWVP;
	D3DXHANDLE   mhWorldInv;
	D3DXHANDLE   mhEyePosW;
	D3DXHANDLE   mhTex;
	D3DXHANDLE   mhMtrl;
	D3DXHANDLE   mhLight;
	D3DXHANDLE   mhNormalMap;

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

	NormalMapDemo app(hInstance, "Normal Map Demo", D3DDEVTYPE_HAL, D3DCREATE_HARDWARE_VERTEXPROCESSING);
	gd3dApp = &app;

	DirectInput di(DISCL_NONEXCLUSIVE|DISCL_FOREGROUND, DISCL_NONEXCLUSIVE|DISCL_FOREGROUND);
	gDInput = &di;

    return gd3dApp->run();
}

NormalMapDemo::NormalMapDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP)
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
 
	mLight.ambient = D3DXCOLOR(0.4f, 0.4f, 0.4f, 1.0f);
	mLight.diffuse = D3DXCOLOR(0.8f, 0.8f, 0.8f, 1.0f);
	mLight.spec    = D3DXCOLOR(0.6f, 0.6f, 0.6f, 1.0f);

	ID3DXMesh* tempMesh = 0;
	LoadXFile("BasicColumnScene.x", &tempMesh, mSceneMtrls, mSceneTextures);

	// Get the vertex declaration for the NMapVertex.
	D3DVERTEXELEMENT9 elems[MAX_FVF_DECL_SIZE];
	UINT numElems = 0;
	HR(NMapVertex::Decl->GetDeclaration(elems, &numElems));

	// Clone the mesh to the NMapVertex format.
	ID3DXMesh* clonedTempMesh = 0;
	HR(tempMesh->CloneMesh(D3DXMESH_MANAGED, elems, gd3dDevice, &clonedTempMesh));

	// Now use D3DXComputeTangentFrameEx to build the TNB-basis for each vertex
	// in the mesh.  
	
	HR(D3DXComputeTangentFrameEx(
	  clonedTempMesh, // Input mesh
	  D3DDECLUSAGE_TEXCOORD, 0, // Vertex element of input tex-coords.  
      D3DDECLUSAGE_BINORMAL, 0, // Vertex element to output binormal.
	  D3DDECLUSAGE_TANGENT, 0,  // Vertex element to output tangent.
      D3DDECLUSAGE_NORMAL, 0,   // Vertex element to output normal.
      0, // Options
      0, // Adjacency
	  0.01f, 0.25f, 0.01f, // Thresholds for handling errors
	  &mSceneMesh, // Output mesh
	  0));         // Vertex Remapping

	// Done with temps.
	ReleaseCOM(tempMesh);
	ReleaseCOM(clonedTempMesh);

	D3DXMatrixIdentity(&mSceneWorld);
	D3DXMatrixIdentity(&mSceneWorldInv);

	HR(D3DXCreateTextureFromFile(gd3dDevice, "floor_nmap.bmp", &mSceneNormalMaps[0]));
	HR(D3DXCreateTextureFromFile(gd3dDevice, "bricks_nmap.bmp", &mSceneNormalMaps[1]));

	HR(D3DXCreateTextureFromFile(gd3dDevice, "whitetex.dds", &mWhiteTex));

	// Initialize camera.
	gCamera->pos().y = 3.0f;
	gCamera->pos().z = -10.0f;
	gCamera->setSpeed(10.0f);

	mGfxStats->addVertices(mSceneMesh->GetNumVertices());
	mGfxStats->addTriangles(mSceneMesh->GetNumFaces());

	mGfxStats->addVertices(mSky->getNumVertices());
	mGfxStats->addTriangles(mSky->getNumTriangles());

	buildFX();

	onResetDevice();
}

NormalMapDemo::~NormalMapDemo()
{
	delete mGfxStats;
	delete mSky;
 
	ReleaseCOM(mFX);

	ReleaseCOM(mSceneMesh);
	for(UINT i = 0; i < mSceneTextures.size(); ++i)
		ReleaseCOM(mSceneTextures[i]);

	ReleaseCOM(mWhiteTex);
	ReleaseCOM(mSceneNormalMaps[0]);
	ReleaseCOM(mSceneNormalMaps[1]);

	DestroyAllVertexDeclarations();
}

bool NormalMapDemo::checkDeviceCaps()
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

void NormalMapDemo::onLostDevice()
{
	mGfxStats->onLostDevice();
	mSky->onLostDevice();
	HR(mFX->OnLostDevice());
}

void NormalMapDemo::onResetDevice()
{
	mGfxStats->onResetDevice();
	mSky->onResetDevice();
	HR(mFX->OnResetDevice());

	// The aspect ratio depends on the backbuffer dimensions, which can 
	// possibly change after a reset.  So rebuild the projection matrix.
	float w = (float)md3dPP.BackBufferWidth;
	float h = (float)md3dPP.BackBufferHeight;
	gCamera->setLens(D3DX_PI * 0.25f, w/h, 1.0f, 5000.0f);
}

void NormalMapDemo::updateScene(float dt)
{
	mGfxStats->update(dt);

	gDInput->poll();

	gCamera->update(dt, 0, 0);


	// Animate light by spinning it around.

	static float time = 0.0f;
	time += dt;

	mLight.dirW.x =  5.0f*cosf(time);
	mLight.dirW.y =  -1.0f;
	mLight.dirW.z =  5.0f*sinf(time);
	
	D3DXVec3Normalize(&mLight.dirW, &mLight.dirW);
}

void NormalMapDemo::drawScene()
{
	HR(gd3dDevice->BeginScene());

	mSky->draw();

	HR(mFX->SetValue(mhLight, &mLight, sizeof(DirLight)));
	HR(mFX->SetMatrix(mhWVP, &(mSceneWorld*gCamera->viewProj())));
	HR(mFX->SetValue(mhEyePosW, &gCamera->pos(), sizeof(D3DXVECTOR3)));
	
	UINT numPasses = 0;
	HR(mFX->Begin(&numPasses, 0));
	HR(mFX->BeginPass(0));

	for(UINT j = 0; j < mSceneMtrls.size(); ++j)
	{
		HR(mFX->SetValue(mhMtrl, &mSceneMtrls[j], sizeof(Mtrl)));
	
		// If there is a texture, then use.
		if(mSceneTextures[j] != 0)
		{
			HR(mFX->SetTexture(mhTex, mSceneTextures[j]));
		}

		// But if not, then set a pure white texture.  When the texture color
		// is multiplied by the color from lighting, it is like multiplying by
		// 1 and won't change the color from lighting.
		else
		{
			HR(mFX->SetTexture(mhTex, mWhiteTex));
		}
	
		HR(mFX->SetTexture(mhNormalMap, mSceneNormalMaps[j]));

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

void NormalMapDemo::buildFX()
{
	// Create the FX from a .fx file.
	ID3DXBuffer* errors = 0;
	HR(D3DXCreateEffectFromFile(gd3dDevice, "NormalMap.fx", 
		0, 0, D3DXSHADER_DEBUG, 0, &mFX, &errors));
	if( errors )
		MessageBox(0, (char*)errors->GetBufferPointer(), 0, 0);

	// Obtain handles.
	mhTech       = mFX->GetTechniqueByName("NormalMapTech");
	mhWVP        = mFX->GetParameterByName(0, "gWVP");
	mhWorldInv   = mFX->GetParameterByName(0, "gWorldInv");
	mhMtrl       = mFX->GetParameterByName(0, "gMtrl");
	mhLight      = mFX->GetParameterByName(0, "gLight");
	mhEyePosW    = mFX->GetParameterByName(0, "gEyePosW");
	mhTex        = mFX->GetParameterByName(0, "gTex");
	mhNormalMap  = mFX->GetParameterByName(0, "gNormalMap");

	// Set parameters that do not vary:

	// World is the identity, so inverse is also identity.
	HR(mFX->SetMatrix(mhWorldInv, &mSceneWorldInv));
	HR(mFX->SetTechnique(mhTech));
}
