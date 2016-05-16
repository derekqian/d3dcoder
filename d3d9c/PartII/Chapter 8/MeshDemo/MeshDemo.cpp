//=============================================================================
// MeshDemo.cpp by Frank Luna (C) 2005 All Rights Reserved.
//
// Demonstrates how create and render basic 3D solid shapes with the
// D3DXCreate* functions.
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
 

class MeshDemo : public D3DApp
{
public:
	MeshDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP);
	~MeshDemo();

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
	ID3DXEffect*            mFX;
	D3DXHANDLE              mhTech;
	D3DXHANDLE              mhWVP;

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

	MeshDemo app(hInstance, "Mesh Demo", D3DDEVTYPE_HAL, D3DCREATE_HARDWARE_VERTEXPROCESSING);
	gd3dApp = &app;

	DirectInput di(DISCL_NONEXCLUSIVE|DISCL_FOREGROUND, DISCL_NONEXCLUSIVE|DISCL_FOREGROUND);
	gDInput = &di;

    if(!gd3dApp->checkDeviceCaps())
		return 0;
	else
		return gd3dApp->run();
}

MeshDemo::MeshDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP)
: D3DApp(hInstance, winCaption, devType, requestedVP)
{
	if(!checkDeviceCaps())
	{
		MessageBox(0, "checkDeviceCaps() Failed", 0, 0);
		PostQuitMessage(0);
	}

	mGfxStats = new GfxStats();

	mCameraRadius    = 10.0f;
	mCameraRotationY = 1.2 * D3DX_PI;
	mCameraHeight    = 5.0f;

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

MeshDemo::~MeshDemo()
{
	delete mGfxStats;
	ReleaseCOM(mVB);
	ReleaseCOM(mIB);
	ReleaseCOM(mFX);
	ReleaseCOM(mCylinder);
	ReleaseCOM(mSphere);

	DestroyAllVertexDeclarations();
}

bool MeshDemo::checkDeviceCaps()
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

void MeshDemo::onLostDevice()
{
	mGfxStats->onLostDevice();
	HR(mFX->OnLostDevice());
}

void MeshDemo::onResetDevice()
{
	mGfxStats->onResetDevice();
	HR(mFX->OnResetDevice());


	// The aspect ratio depends on the backbuffer dimensions, which can 
	// possibly change after a reset.  So rebuild the projection matrix.
	buildProjMtx();
}

void MeshDemo::updateScene(float dt)
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

	// The camera position/orientation relative to world space can 
	// change every frame based on input, so we need to rebuild the
	// view matrix every frame with the latest changes.
	buildViewMtx();
}


void MeshDemo::drawScene()
{
	// Clear the backbuffer and depth buffer.
	HR(gd3dDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0));

	HR(gd3dDevice->BeginScene());

	// Let Direct3D know the vertex buffer, index buffer and vertex 
	// declaration we are using.
	HR(gd3dDevice->SetStreamSource(0, mVB, 0, sizeof(VertexPos)));
	HR(gd3dDevice->SetIndices(mIB));
	HR(gd3dDevice->SetVertexDeclaration(VertexPos::Decl));

	// Setup the rendering FX
	HR(mFX->SetTechnique(mhTech));
	

	// Begin passes.
	UINT numPasses = 0;
	HR(mFX->Begin(&numPasses, 0));
	for(UINT i = 0; i < numPasses; ++i)
	{
		HR(mFX->BeginPass(i));

		HR(mFX->SetMatrix(mhWVP, &(mView*mProj)));
		HR(mFX->CommitChanges());
		HR(gd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, mNumGridVertices, 0, mNumGridTriangles));
		
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
 
void MeshDemo::buildGeoBuffers()
{
	std::vector<D3DXVECTOR3> verts;
	std::vector<DWORD> indices;

	GenTriGrid(100, 100, 1.0f, 1.0f, 
		D3DXVECTOR3(0.0f, 0.0f, 0.0f), verts, indices);

	// Save vertex count and triangle count for DrawIndexedPrimitive arguments.
	mNumGridVertices  = 100*100;
	mNumGridTriangles = 99*99*2;

	// Obtain a pointer to a new vertex buffer.
	HR(gd3dDevice->CreateVertexBuffer(mNumGridVertices * sizeof(VertexPos), 
		D3DUSAGE_WRITEONLY,	0, D3DPOOL_MANAGED, &mVB, 0));

	// Now lock it to obtain a pointer to its internal data, and write the
	// grid's vertex data.
	VertexPos* v = 0;
	HR(mVB->Lock(0, 0, (void**)&v, 0));

	for(DWORD i = 0; i < mNumGridVertices; ++i)
		v[i] = verts[i];

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

void MeshDemo::buildFX()
{
	// Create the FX from a .fx file.
	ID3DXBuffer* errors = 0;
	HR(D3DXCreateEffectFromFile(gd3dDevice, "transform.fx", 
		0, 0, D3DXSHADER_DEBUG, 0, &mFX, &errors));
	if( errors )
		MessageBox(0, (char*)errors->GetBufferPointer(), 0, 0);

	// Obtain handles.
	mhTech = mFX->GetTechniqueByName("TransformTech");
	mhWVP  = mFX->GetParameterByName(0, "gWVP");
}

void MeshDemo::buildViewMtx()
{
	float x = mCameraRadius * cosf(mCameraRotationY);
	float z = mCameraRadius * sinf(mCameraRotationY);
	D3DXVECTOR3 pos(x, mCameraHeight, z);
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
	D3DXMatrixLookAtLH(&mView, &pos, &target, &up);
}

void MeshDemo::buildProjMtx()
{
	float w = (float)md3dPP.BackBufferWidth;
	float h = (float)md3dPP.BackBufferHeight;
	D3DXMatrixPerspectiveFovLH(&mProj, D3DX_PI * 0.25f, w/h, 1.0f, 5000.0f);
}

void MeshDemo::drawCylinders()
{
	D3DXMATRIX T, R;

	D3DXMatrixRotationX(&R, D3DX_PI*0.5f);

	for(int z = -30; z <= 30; z+= 10)
	{
		D3DXMatrixTranslation(&T, -10.0f, 3.0f, (float)z);
		HR(mFX->SetMatrix(mhWVP, &(R*T*mView*mProj)));
		HR(mFX->CommitChanges());
		HR(mCylinder->DrawSubset(0));

		D3DXMatrixTranslation(&T, 10.0f, 3.0f, (float)z);
		HR(mFX->SetMatrix(mhWVP, &(R*T*mView*mProj)));
		HR(mFX->CommitChanges());
		HR(mCylinder->DrawSubset(0));
	}
}

void MeshDemo::drawSpheres()
{
	D3DXMATRIX T;

	for(int z = -30; z <= 30; z+= 10)
	{
		D3DXMatrixTranslation(&T, -10.0f, 7.5f, (float)z);
		HR(mFX->SetMatrix(mhWVP, &(T*mView*mProj)));
		HR(mFX->CommitChanges());
		HR(mSphere->DrawSubset(0));

		D3DXMatrixTranslation(&T, 10.0f, 7.5f, (float)z);
		HR(mFX->SetMatrix(mhWVP, &(T*mView*mProj)));
		HR(mFX->CommitChanges());
		HR(mSphere->DrawSubset(0));
	}
}