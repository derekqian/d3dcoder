//=============================================================================
// SphereCylDemo.cpp by Frank Luna (C) 2005 All Rights Reserved.
//
// Demonstrates how to generate texture coordinates for sphere and
// cylinder.
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

class SphereCylDemo : public D3DApp
{
public:
	SphereCylDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP);
	~SphereCylDemo();

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

	void drawGrid();
	void drawCylinders();
	void drawSpheres();

	enum AXIS
	{
		X_AXIS,
		Y_AXIS,
		Z_AXIS
	};

	void genSphericalTexCoords();
	void genCylTexCoords(AXIS axis);

private:
	GfxStats* mGfxStats;

	DWORD mNumGridVertices;
	DWORD mNumGridTriangles;

	ID3DXMesh* mCylinder;
	ID3DXMesh* mSphere;

	IDirect3DVertexBuffer9* mVB;
	IDirect3DIndexBuffer9*  mIB;

	IDirect3DTexture9* mSphereTex;
	IDirect3DTexture9* mCylTex;
	IDirect3DTexture9* mGridTex;

	ID3DXEffect* mFX;
	D3DXHANDLE   mhTech;
	D3DXHANDLE   mhWVP;
	D3DXHANDLE   mhWorldInvTrans;
	D3DXHANDLE   mhAmbientLight;
	D3DXHANDLE   mhDiffuseLight;
	D3DXHANDLE   mhSpecLight;
	D3DXHANDLE   mhLightVecW;
	D3DXHANDLE   mhAmbientMtrl;
	D3DXHANDLE   mhDiffuseMtrl;
	D3DXHANDLE   mhSpecMtrl;
	D3DXHANDLE   mhSpecPower;
	D3DXHANDLE   mhEyePos;
	D3DXHANDLE   mhWorld;
	D3DXHANDLE   mhTex;

	D3DXCOLOR   mAmbientLight;
	D3DXCOLOR   mDiffuseLight;
	D3DXCOLOR   mSpecLight;
	D3DXVECTOR3 mLightVecW;

	Mtrl  mGridMtrl;
	Mtrl  mCylinderMtrl;
	Mtrl  mSphereMtrl;

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

	SphereCylDemo app(hInstance, "Sphere-Cyl-Tex Demo", D3DDEVTYPE_HAL, D3DCREATE_HARDWARE_VERTEXPROCESSING);
	gd3dApp = &app;

	DirectInput di(DISCL_NONEXCLUSIVE|DISCL_FOREGROUND, DISCL_NONEXCLUSIVE|DISCL_FOREGROUND);
	gDInput = &di;

    return gd3dApp->run();
}

SphereCylDemo::SphereCylDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP)
: D3DApp(hInstance, winCaption, devType, requestedVP)
{
	if(!checkDeviceCaps())
	{
		MessageBox(0, "checkDeviceCaps() Failed", 0, 0);
		PostQuitMessage(0);
	}

	InitAllVertexDeclarations();

	mGfxStats = new GfxStats();

	mCameraRadius    = 50.0f;
	mCameraRotationY = 1.2 * D3DX_PI;
	mCameraHeight    = 20.0f;

	mAmbientLight   = WHITE;
	mDiffuseLight   = WHITE;
	mSpecLight      = WHITE;
	mLightVecW      = D3DXVECTOR3(0.0, 0.0f, -1.0f);

	mGridMtrl     = Mtrl(WHITE*0.7f, WHITE, WHITE*0.5f, 16.0f);
	mCylinderMtrl = Mtrl(WHITE*0.4f, WHITE, WHITE*0.8f, 8.0f);
	mSphereMtrl   = Mtrl(WHITE*0.4f, WHITE, WHITE*0.8f, 8.0f);

	HR(D3DXCreateCylinder(gd3dDevice, 1.0f, 1.0f, 6.0f, 20, 20, &mCylinder, 0));
	HR(D3DXCreateSphere(gd3dDevice, 1.0f, 20, 20, &mSphere, 0));

	genSphericalTexCoords();
	genCylTexCoords(Z_AXIS);

	HR(D3DXCreateTextureFromFile(gd3dDevice, "marble.bmp", &mSphereTex));
	HR(D3DXCreateTextureFromFile(gd3dDevice, "stone2.dds", &mCylTex));
	HR(D3DXCreateTextureFromFile(gd3dDevice, "ground0.dds", &mGridTex));

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
}

SphereCylDemo::~SphereCylDemo()
{
	delete mGfxStats;
	ReleaseCOM(mVB);
	ReleaseCOM(mIB);
	ReleaseCOM(mFX);
	ReleaseCOM(mCylinder);
	ReleaseCOM(mSphere);
	ReleaseCOM(mSphereTex);
	ReleaseCOM(mCylTex);
	ReleaseCOM(mGridTex);

	DestroyAllVertexDeclarations();
}

bool SphereCylDemo::checkDeviceCaps()
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

void SphereCylDemo::onLostDevice()
{
	mGfxStats->onLostDevice();
	HR(mFX->OnLostDevice());
}

void SphereCylDemo::onResetDevice()
{
	mGfxStats->onResetDevice();
	HR(mFX->OnResetDevice());


	// The aspect ratio depends on the backbuffer dimensions, which can 
	// possibly change after a reset.  So rebuild the projection matrix.
	buildProjMtx();
}

void SphereCylDemo::updateScene(float dt)
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


void SphereCylDemo::drawScene()
{
	// Clear the backbuffer and depth buffer.
	HR(gd3dDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0));

	HR(gd3dDevice->BeginScene());

	// Setup the rendering FX
	HR(mFX->SetValue(mhAmbientLight, &mAmbientLight, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhDiffuseLight, &mDiffuseLight, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhSpecLight, &mSpecLight, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhLightVecW, &mLightVecW, sizeof(D3DXVECTOR3)));
 
	// Begin passes.
	UINT numPasses = 0;
	HR(mFX->Begin(&numPasses, 0));
	for(UINT i = 0; i < numPasses; ++i)
	{
		HR(mFX->BeginPass(i));

		drawGrid();
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
 
void SphereCylDemo::buildGeoBuffers()
{
	std::vector<D3DXVECTOR3> verts;
	std::vector<DWORD> indices;

	GenTriGrid(100, 100, 1.0f, 1.0f, 
		D3DXVECTOR3(0.0f, 0.0f, 0.0f), verts, indices);

	// Save vertex count and triangle count for DrawIndexedPrimitive arguments.
	mNumGridVertices  = 100*100;
	mNumGridTriangles = 99*99*2;

	// Obtain a pointer to a new vertex buffer.
	HR(gd3dDevice->CreateVertexBuffer(mNumGridVertices * sizeof(VertexPNT), 
		D3DUSAGE_WRITEONLY,	0, D3DPOOL_MANAGED, &mVB, 0));

	// Now lock it to obtain a pointer to its internal data, and write the
	// grid's vertex data.
	VertexPNT* v = 0;
	HR(mVB->Lock(0, 0, (void**)&v, 0));

	float texScale = 0.2f;
	for(int i = 0; i < 100; ++i)
	{
		for(int j = 0; j < 100; ++j)
		{
			DWORD index = i * 100 + j;
			v[index].pos    = verts[index];
			v[index].normal = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
			v[index].tex0 = D3DXVECTOR2((float)j, (float)i) * texScale;
		}
	}

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

void SphereCylDemo::buildFX()
{
	// Create the FX from a .fx file.
	ID3DXBuffer* errors = 0;
	HR(D3DXCreateEffectFromFile(gd3dDevice, "dirLightTex.fx", 
		0, 0, D3DXSHADER_DEBUG, 0, &mFX, &errors));
	if( errors )
		MessageBox(0, (char*)errors->GetBufferPointer(), 0, 0);

	// Obtain handles.
	mhTech           = mFX->GetTechniqueByName("DirLightTexTech");
	mhWVP            = mFX->GetParameterByName(0, "gWVP");
	mhWorldInvTrans  = mFX->GetParameterByName(0, "gWorldInvTrans");
	mhEyePos         = mFX->GetParameterByName(0, "gEyePosW");
	mhWorld          = mFX->GetParameterByName(0, "gWorld");
	mhAmbientLight   = mFX->GetParameterByName(0, "gAmbientLight");
	mhDiffuseLight   = mFX->GetParameterByName(0, "gDiffuseLight");
	mhSpecLight      = mFX->GetParameterByName(0, "gSpecularLight");
	mhLightVecW      = mFX->GetParameterByName(0, "gLightVecW");
	mhAmbientMtrl    = mFX->GetParameterByName(0, "gAmbientMtrl");
	mhDiffuseMtrl    = mFX->GetParameterByName(0, "gDiffuseMtrl");
	mhSpecMtrl       = mFX->GetParameterByName(0, "gSpecularMtrl");
	mhSpecPower      = mFX->GetParameterByName(0, "gSpecularPower");
	mhTex            = mFX->GetParameterByName(0, "gTex");
}

void SphereCylDemo::buildViewMtx()
{
	float x = mCameraRadius * cosf(mCameraRotationY);
	float z = mCameraRadius * sinf(mCameraRotationY);
	D3DXVECTOR3 pos(x, mCameraHeight, z);
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
	D3DXMatrixLookAtLH(&mView, &pos, &target, &up);

	HR(mFX->SetValue(mhEyePos, &pos, sizeof(D3DXVECTOR3)));
}

void SphereCylDemo::buildProjMtx()
{
	float w = (float)md3dPP.BackBufferWidth;
	float h = (float)md3dPP.BackBufferHeight;
	D3DXMatrixPerspectiveFovLH(&mProj, D3DX_PI * 0.25f, w/h, 1.0f, 5000.0f);
}

void SphereCylDemo::drawGrid()
{
	HR(gd3dDevice->SetStreamSource(0, mVB, 0, sizeof(VertexPNT)));
	HR(gd3dDevice->SetIndices(mIB));
	HR(gd3dDevice->SetVertexDeclaration(VertexPNT::Decl));

	D3DXMATRIX W, WIT;
	D3DXMatrixIdentity(&W);
	D3DXMatrixInverse(&WIT, 0, &W);
	D3DXMatrixTranspose(&WIT, &WIT);
	HR(mFX->SetMatrix(mhWorld, &W));
	HR(mFX->SetMatrix(mhWVP, &(W*mView*mProj)));
	HR(mFX->SetMatrix(mhWorldInvTrans, &WIT));

	HR(mFX->SetValue(mhAmbientMtrl, &mGridMtrl.ambient, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhDiffuseMtrl, &mGridMtrl.diffuse, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhSpecMtrl, &mGridMtrl.spec, sizeof(D3DXCOLOR)));
	HR(mFX->SetFloat(mhSpecPower, mGridMtrl.specPower));

	HR(mFX->SetTexture(mhTex, mGridTex));

	HR(mFX->CommitChanges());
	HR(gd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, mNumGridVertices, 0, mNumGridTriangles));
}

void SphereCylDemo::drawCylinders()
{
	HR(gd3dDevice->SetRenderState(D3DRS_WRAP0, D3DWRAP_U));

	D3DXMATRIX T, R, W, WIT;

	D3DXMatrixRotationX(&R, -D3DX_PI*0.5f);

	HR(mFX->SetValue(mhAmbientMtrl, &mCylinderMtrl.ambient, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhDiffuseMtrl, &mCylinderMtrl.diffuse, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhSpecMtrl, &mCylinderMtrl.spec, sizeof(D3DXCOLOR)));
	HR(mFX->SetFloat(mhSpecPower, mCylinderMtrl.specPower));

	HR(mFX->SetTexture(mhTex, mCylTex));
	for(int z = -30; z <= 30; z+= 10)
	{
		D3DXMatrixTranslation(&T, -10.0f, 3.0f, (float)z);
		W = R*T;
		D3DXMatrixInverse(&WIT, 0, &W);
		D3DXMatrixTranspose(&WIT, &WIT);

		HR(mFX->SetMatrix(mhWVP, &(W*mView*mProj)));
		HR(mFX->SetMatrix(mhWorld, &W));
		HR(mFX->SetMatrix(mhWorldInvTrans, &WIT));
		HR(mFX->CommitChanges());
		HR(mCylinder->DrawSubset(0));

		D3DXMatrixTranslation(&T, 10.0f, 3.0f, (float)z);
		W = R*T;
		D3DXMatrixInverse(&WIT, 0, &W);
		D3DXMatrixTranspose(&WIT, &WIT);

		HR(mFX->SetMatrix(mhWVP, &(W*mView*mProj)));
		HR(mFX->SetMatrix(mhWorld, &W));
		HR(mFX->SetMatrix(mhWorldInvTrans, &WIT));
		HR(mFX->CommitChanges());
		HR(mCylinder->DrawSubset(0));
	}
	// Disable.
	HR(gd3dDevice->SetRenderState(D3DRS_WRAP0, 0));
}

void SphereCylDemo::drawSpheres()
{
	HR(gd3dDevice->SetRenderState(D3DRS_WRAP0, D3DWRAP_U));

	D3DXMATRIX W, WIT;

	HR(mFX->SetValue(mhAmbientMtrl, &mSphereMtrl.ambient, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhDiffuseMtrl, &mSphereMtrl.diffuse, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhSpecMtrl, &mSphereMtrl.spec, sizeof(D3DXCOLOR)));
	HR(mFX->SetFloat(mhSpecPower, mSphereMtrl.specPower));

	HR(mFX->SetTexture(mhTex, mSphereTex));
	for(int z = -30; z <= 30; z+= 10)
	{
		D3DXMatrixTranslation(&W, -10.0f, 7.5f, (float)z);
		D3DXMatrixInverse(&WIT, 0, &W);
		D3DXMatrixTranspose(&WIT, &WIT);

		HR(mFX->SetMatrix(mhWVP, &(W*mView*mProj)));
		HR(mFX->SetMatrix(mhWorld, &W));
		HR(mFX->SetMatrix(mhWorldInvTrans, &WIT));
		HR(mFX->CommitChanges());
		HR(mSphere->DrawSubset(0));

		D3DXMatrixTranslation(&W, 10.0f, 7.5f, (float)z);
		D3DXMatrixInverse(&WIT, 0, &W);
		D3DXMatrixTranspose(&WIT, &WIT);

		HR(mFX->SetMatrix(mhWVP, &(W*mView*mProj)));
		HR(mFX->SetMatrix(mhWorld, &W));
		HR(mFX->SetMatrix(mhWorldInvTrans, &WIT));
		HR(mFX->CommitChanges());
		HR(mSphere->DrawSubset(0));
	}
	// Disable.
	HR(gd3dDevice->SetRenderState(D3DRS_WRAP0, 0));

}

void SphereCylDemo::genSphericalTexCoords()
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

void SphereCylDemo::genCylTexCoords(AXIS axis)
{
	// D3DXCreate* functions generate vertices with position 
	// and normal data.  But for texturing, we also need
	// tex-coords.  So clone the mesh to change the vertex
	// format to a format with tex-coords.

	D3DVERTEXELEMENT9 elements[64];
	UINT numElements = 0;
	VertexPNT::Decl->GetDeclaration(elements, &numElements);

	ID3DXMesh* temp = 0;
	HR(mCylinder->CloneMesh(D3DXMESH_SYSTEMMEM, 
		elements, gd3dDevice, &temp));

	ReleaseCOM(mCylinder);

	// Now generate texture coordinates for each vertex.
	VertexPNT* vertices = 0;
	HR(temp->LockVertexBuffer(0, (void**)&vertices));

	// We need to get the height of the cylinder we are projecting the
	// vertices onto.  That height depends on which axis the client has
	// specified that the cylinder lies on.  The height is determined by 
	// finding the height of the bounding cylinder on the specified axis.

	D3DXVECTOR3 maxPoint(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	D3DXVECTOR3 minPoint(FLT_MAX, FLT_MAX, FLT_MAX);
	
	for(UINT i = 0; i < temp->GetNumVertices(); ++i)
	{
		D3DXVec3Maximize(&maxPoint, &maxPoint, &vertices[i].pos);
		D3DXVec3Minimize(&minPoint, &minPoint, &vertices[i].pos);
	}

	float a = 0.0f;
	float b = 0.0f;
	float h = 0.0f;
	switch( axis )
	{
	case X_AXIS:
		a = minPoint.x;
		b = maxPoint.x;
		h = b-a;
		break;
	case Y_AXIS:
		a = minPoint.y;
		b = maxPoint.y;
		h = b-a;
		break;
	case Z_AXIS:
		a = minPoint.z;
		b = maxPoint.z;
		h = b-a;
		break;
	}


	// Iterate over each vertex and compute its texture coordinate.
	
	for(UINT i = 0; i < temp->GetNumVertices(); ++i)
	{
		// Get the coordinates along the axes orthogonal to the
		// axis the cylinder is aligned with.

		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		switch( axis )
		{
		case X_AXIS:
			x = vertices[i].pos.y;
			z = vertices[i].pos.z;
			y = vertices[i].pos.x;
			break;
		case Y_AXIS:
			x = vertices[i].pos.x;
			z = vertices[i].pos.z;
			y = vertices[i].pos.y;
			break;
		case Z_AXIS:
			x = vertices[i].pos.x;
			z = vertices[i].pos.y;
			y = vertices[i].pos.z;
			break;
		}

		// Convert to cylindrical coordinates.
		
		float theta = atan2f(z, x);
		float y2    = y - b; // Transform [a, b]-->[-h, 0]

		// Transform theta from [0, 2*pi] to [0, 1] range and
		// transform y2 from [-h, 0] to [0, 1].

		float u = theta / (2.0f*D3DX_PI);
		float v = y2 / -h; 
		
		// Save texture coordinates.
		
		vertices[i].tex0.x = u;
		vertices[i].tex0.y = v;
	}

	HR(temp->UnlockVertexBuffer());

	// Clone back to a hardware mesh.
	HR(temp->CloneMesh(D3DXMESH_MANAGED | D3DXMESH_WRITEONLY,
		elements, gd3dDevice, &mCylinder));

	ReleaseCOM(temp);
}