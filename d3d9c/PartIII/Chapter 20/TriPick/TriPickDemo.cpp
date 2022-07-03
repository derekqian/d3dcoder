//=============================================================================
// TriPickDemo.cpp by Frank Luna (C) 2005 All Rights Reserved.
//
// Demonstrates triangle picking with D3DXIntersect.
//
// Controls: Use mouse to look and 'W', 'S', 'A', and 'D' keys to move.
//           Left mouse button to pick triangle.
//=============================================================================

#include "d3dApp.h"
#include "DirectInput.h"
#include <crtdbg.h>
#include "GfxStats.h"
#include <list>
#include <vector> 
#pragma comment(lib, "legacy_stdio_definitions.lib")
#include <ctime>
#include "Camera.h"
#include "Vertex.h"


class TriPickDemo : public D3DApp
{
public:
	TriPickDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP);
	~TriPickDemo();

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

	// Car mesh.
	ID3DXMesh* mMesh;
	std::vector<Mtrl> mMeshMtrls;
	std::vector<IDirect3DTexture9*> mMeshTextures;
	
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

	TriPickDemo app(hInstance, "Tri-Pick Demo", D3DDEVTYPE_HAL, D3DCREATE_HARDWARE_VERTEXPROCESSING);
	gd3dApp = &app;

	DirectInput di(DISCL_NONEXCLUSIVE|DISCL_FOREGROUND, DISCL_NONEXCLUSIVE|DISCL_FOREGROUND);
	gDInput = &di;

    return gd3dApp->run();
}

TriPickDemo::TriPickDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP)
: D3DApp(hInstance, winCaption, devType, requestedVP)
{
	if(!checkDeviceCaps())
	{
		MessageBox(0, "checkDeviceCaps() Failed", 0, 0);
		PostQuitMessage(0);
	}

	InitAllVertexDeclarations();

	mGfxStats = new GfxStats();
	
	// Load the mesh data.
	LoadXFile("car.x", &mMesh, mMeshMtrls, mMeshTextures);

	// Initialize camera.
	gCamera->pos() = D3DXVECTOR3(-0.0f, 2.0f, -15.0f);
	gCamera->setSpeed(40.0f);

	// Load the default texture.
	HR(D3DXCreateTextureFromFile(gd3dDevice, "whitetex.dds", &mWhiteTex));

	// Init a light.
	mLight.dirW    = D3DXVECTOR3(0.707f, 0.0f, 0.707f);
	D3DXVec3Normalize(&mLight.dirW, &mLight.dirW);
	mLight.ambient = D3DXCOLOR(0.5f, 0.5f, 0.5f, 1.0f);
	mLight.diffuse = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	mLight.spec    = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	
	buildFX();

	mGfxStats->addTriangles(mMesh->GetNumFaces());
	mGfxStats->addVertices(mMesh->GetNumVertices());

	onResetDevice();
}

TriPickDemo::~TriPickDemo()
{
	delete mGfxStats;
	ReleaseCOM(mWhiteTex);
	ReleaseCOM(mFX);
	
	ReleaseCOM(mMesh);
	for(UINT i = 0; i < mMeshTextures.size(); ++i)
		ReleaseCOM(mMeshTextures[i]);
	
	DestroyAllVertexDeclarations();
}

bool TriPickDemo::checkDeviceCaps()
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

void TriPickDemo::onLostDevice()
{
	mGfxStats->onLostDevice();
	HR(mFX->OnLostDevice());
}

void TriPickDemo::onResetDevice()
{
	mGfxStats->onResetDevice();
	HR(mFX->OnResetDevice());

	// The aspect ratio depends on the backbuffer dimensions, which can 
	// possibly change after a reset.  So rebuild the projection matrix.
	float w = (float)md3dPP.BackBufferWidth;
	float h = (float)md3dPP.BackBufferHeight;
	gCamera->setLens(D3DX_PI * 0.25f, w/h, 0.01f, 5000.0f);
}

void TriPickDemo::updateScene(float dt)
{
	mGfxStats->update(dt);

	gDInput->poll();

	gCamera->update(dt, 0, 0);
}

void TriPickDemo::drawScene()
{
	// Clear the backbuffer and depth buffer.
	HR(gd3dDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0));

	HR(gd3dDevice->BeginScene());

	HR(mFX->SetValue(mhEyePos, &gCamera->pos(), sizeof(D3DXVECTOR3)));
	HR(mFX->SetTechnique(mhTech));
	UINT numPasses = 0;
	HR(mFX->Begin(&numPasses, 0));
	HR(mFX->BeginPass(0));

	// Specify mesh directly in world space.
	D3DXMATRIX toWorld;
	D3DXMatrixIdentity(&toWorld);

	// Set FX parameters.
	HR(mFX->SetMatrix(mhWVP, &(toWorld*gCamera->viewProj())));
	D3DXMATRIX worldInvTrans;
	D3DXMatrixInverse(&worldInvTrans, 0, &toWorld);
	D3DXMatrixTranspose(&worldInvTrans, &worldInvTrans);
	HR(mFX->SetMatrix(mhWorldInvTrans, &worldInvTrans));
	HR(mFX->SetMatrix(mhWorld, &toWorld));

	// Draw the car in wireframe mode.
	HR(gd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME));
	for(UINT j = 0; j < mMeshMtrls.size(); ++j)
	{
		HR(mFX->SetValue(mhMtrl, &mMeshMtrls[j], sizeof(Mtrl)));
			
		// If there is a texture, then use.
		if(mMeshTextures[j] != 0)
		{
			HR(mFX->SetTexture(mhTex, mMeshTextures[j]));
		}

		// But if not, then set a pure white texture.  When the texture color
		// is multiplied by the color from lighting, it is like multiplying by
		// 1 and won't change the color from lighting.
		else
		{
			HR(mFX->SetTexture(mhTex, mWhiteTex));
		}
			
		HR(mFX->CommitChanges());
		HR(mMesh->DrawSubset(j));
	}

	// Switch back to solid mode.
	HR(gd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID));

	// Did we pick anything?
	D3DXVECTOR3 originW(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 dirW(0.0f, 0.0f, 0.0f);
	if( gDInput->mouseButtonDown(0) )
	{
		getWorldPickingRay(originW, dirW);

		BOOL hit = 0;
		DWORD faceIndex = -1;
		float u = 0.0f;
		float v = 0.0f;
		float dist = 0.0f;
		ID3DXBuffer* allhits = 0;
		DWORD numHits = 0;
		HR(D3DXIntersect(mMesh, &originW, &dirW, &hit,
			&faceIndex, &u, &v, &dist, &allhits, &numHits));
		ReleaseCOM(allhits);

		// We hit anything?
		if( hit )
		{
			// Yes, draw the picked triangle in solid mode.
			IDirect3DVertexBuffer9* vb = 0;
			IDirect3DIndexBuffer9* ib = 0;
			HR(mMesh->GetVertexBuffer(&vb));
			HR(mMesh->GetIndexBuffer(&ib));

			HR(gd3dDevice->SetIndices(ib));
			HR(gd3dDevice->SetVertexDeclaration(VertexPNT::Decl));
			HR(gd3dDevice->SetStreamSource(0, vb, 0, sizeof(VertexPNT)));

			// faceIndex identifies the picked triangle to draw.
			HR(gd3dDevice->DrawIndexedPrimitive(
				D3DPT_TRIANGLELIST, 0, 0, mMesh->GetNumVertices(), faceIndex*3, 1))
			
			ReleaseCOM(vb);
			ReleaseCOM(ib);
		}
	}
	 
	HR(mFX->EndPass());
	HR(mFX->End());

	mGfxStats->display();

	HR(gd3dDevice->EndScene());

	// Present the backbuffer.
	HR(gd3dDevice->Present(0, 0, 0, 0));
}

void TriPickDemo::buildFX()
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

void TriPickDemo::getWorldPickingRay(D3DXVECTOR3& originW, D3DXVECTOR3& dirW)
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