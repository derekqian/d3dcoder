//=============================================================================
// ShadowDemo.cpp by Frank Luna (C) 2005 All Rights Reserved.
//
// Prevents double blending with the stencil buffer.
//
// Controls: Use mouse to orbit and zoom; use the 'W' and 'S' keys to 
//           alter the height of the camera.
//           Use 'A' and 'D' keys to translate the teapot on the x-axis.
//=============================================================================

#include "d3dApp.h"
#include "DirectInput.h"
#include <crtdbg.h>
#include "GfxStats.h"
#include <list>
#include "Vertex.h"

class ShadowDemo : public D3DApp
{
public:
	ShadowDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP);
	~ShadowDemo();

	bool checkDeviceCaps();
	void onLostDevice();
	void onResetDevice();
	void updateScene(float dt);
	void drawScene();

	// Helper methods
	void buildRoomGeometry();
	void buildFX();
	void buildViewMtx();
	void buildProjMtx();

	void drawRoom();
	void drawMirror();
	void drawTeapot();
	void drawReflectedTeapot();
	void drawTeapotShadow();
	void genSphericalTexCoords();

private:
	GfxStats* mGfxStats;
	
	IDirect3DVertexBuffer9* mRoomVB;
	ID3DXMesh*              mTeapot;

	IDirect3DTexture9*     mFloorTex;
	IDirect3DTexture9*     mWallTex;
	IDirect3DTexture9*     mMirrorTex;
	IDirect3DTexture9*     mTeapotTex;

	ID3DXEffect* mFX;
	D3DXHANDLE   mhTech;
	D3DXHANDLE   mhWVP;
	D3DXHANDLE   mhWorldInvTrans;
	D3DXHANDLE   mhLightVecW;
	D3DXHANDLE   mhDiffuseMtrl;
	D3DXHANDLE   mhDiffuseLight;
	D3DXHANDLE   mhAmbientMtrl;
	D3DXHANDLE   mhAmbientLight;
	D3DXHANDLE   mhSpecularMtrl;
	D3DXHANDLE   mhSpecularLight;
	D3DXHANDLE   mhSpecularPower;
	D3DXHANDLE   mhEyePos;
	D3DXHANDLE   mhWorld;
	D3DXHANDLE   mhTex;

	Mtrl mWhiteMtrl;
	Mtrl mShadowMtrl;

	D3DXVECTOR3 mLightVecW;
	D3DXCOLOR   mAmbientLight;
	D3DXCOLOR   mDiffuseLight;
	D3DXCOLOR   mSpecularLight;

	float mCameraRotationY;
	float mCameraRadius;
	float mCameraHeight;

	D3DXMATRIX mRoomWorld;
	D3DXMATRIX mTeapotWorld;

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

	ShadowDemo app(hInstance, "Shadow Demo", D3DDEVTYPE_HAL, D3DCREATE_HARDWARE_VERTEXPROCESSING);
	gd3dApp = &app;

	DirectInput di(DISCL_NONEXCLUSIVE|DISCL_FOREGROUND, DISCL_NONEXCLUSIVE|DISCL_FOREGROUND);
	gDInput = &di;

    return gd3dApp->run();
}

ShadowDemo::ShadowDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP)
: D3DApp(hInstance, winCaption, devType, requestedVP)
{
	if(!checkDeviceCaps())
	{
		MessageBox(0, "checkDeviceCaps() Failed", 0, 0);
		PostQuitMessage(0);
	}

	InitAllVertexDeclarations();

	mGfxStats = new GfxStats();
	

	mCameraRadius    = 15.0f;
	mCameraRotationY = 1.4f * D3DX_PI;
	mCameraHeight    = 5.0f;

	mLightVecW     = D3DXVECTOR3(-0.577f, 0.577f, -0.577f);
	mDiffuseLight  = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	mAmbientLight  = D3DXCOLOR(0.6f, 0.6f, 0.6f, 1.0f);
	mSpecularLight = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);

	mWhiteMtrl.ambient   = WHITE;
	mWhiteMtrl.diffuse   = WHITE;
	mWhiteMtrl.spec      = WHITE * 0.8f;
	mWhiteMtrl.specPower = 16.0f;

	mShadowMtrl.ambient   = BLACK;
	mShadowMtrl.diffuse   = BLACK;
	mShadowMtrl.diffuse.a = 0.5f; // 50% transparent.
	mShadowMtrl.spec      = BLACK;
	mShadowMtrl.specPower = 1.0f;


	D3DXMatrixIdentity(&mRoomWorld);
	D3DXMatrixTranslation(&mTeapotWorld, 0.0f, 3.0f, -6.0f);

	HR(D3DXCreateTextureFromFile(gd3dDevice, "checkboard.dds", &mFloorTex));
	HR(D3DXCreateTextureFromFile(gd3dDevice, "brick2.dds", &mWallTex));
	HR(D3DXCreateTextureFromFile(gd3dDevice, "ice.dds", &mMirrorTex));
	HR(D3DXCreateTextureFromFile(gd3dDevice, "brick1.dds", &mTeapotTex));

	HR(D3DXCreateTeapot(gd3dDevice, &mTeapot, 0));
	// Generate texture coordinates for the teapot.
	genSphericalTexCoords();

	// Room geometry count.
	mGfxStats->addVertices(24);
	mGfxStats->addTriangles(8);

	// We draw the teapot 3 times--one regularly, one reflected and one shadowed.
	mGfxStats->addVertices(mTeapot->GetNumVertices() * 3);
	mGfxStats->addTriangles(mTeapot->GetNumFaces()   * 3);

	buildRoomGeometry();
	buildFX();

	onResetDevice();
}

ShadowDemo::~ShadowDemo()
{
	delete mGfxStats;
	ReleaseCOM(mRoomVB);
	ReleaseCOM(mTeapot);
	ReleaseCOM(mFloorTex);
	ReleaseCOM(mWallTex);
	ReleaseCOM(mMirrorTex);
	ReleaseCOM(mTeapotTex);
	ReleaseCOM(mFX);

	DestroyAllVertexDeclarations();
}

bool ShadowDemo::checkDeviceCaps()
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

void ShadowDemo::onLostDevice()
{
	mGfxStats->onLostDevice();
	HR(mFX->OnLostDevice());
}

void ShadowDemo::onResetDevice()
{
	mGfxStats->onResetDevice();
	HR(mFX->OnResetDevice());


	// The aspect ratio depends on the backbuffer dimensions, which can 
	// possibly change after a reset.  So rebuild the projection matrix.
	buildProjMtx();
}

void ShadowDemo::updateScene(float dt)
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
	if( mCameraRadius < 3.0f )
		mCameraRadius = 3.0f;

	// The camera position/orientation relative to world space can 
	// change every frame based on input, so we need to rebuild the
	// view matrix every frame with the latest changes.
	buildViewMtx();

	// Update teapot position by modifying the translation elements
	// in its world matrix.
	if( gDInput->keyDown(DIK_A) )	 
		mTeapotWorld(3,0)   -= 2.0f * dt;
	if( gDInput->keyDown(DIK_D) )	 
		mTeapotWorld(3,0)   += 2.0f * dt;
}


void ShadowDemo::drawScene()
{
	// Clear the backbuffer and depth buffer.
	HR(gd3dDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER |
		D3DCLEAR_STENCIL, 0xffeeeeee, 1.0f, 0));

	HR(gd3dDevice->BeginScene());

	HR(mFX->SetTechnique(mhTech));
	HR(mFX->SetValue(mhLightVecW, &mLightVecW, sizeof(D3DXVECTOR3)));
	HR(mFX->SetValue(mhDiffuseLight, &mDiffuseLight, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhAmbientLight, &mAmbientLight, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhSpecularLight, &mSpecularLight, sizeof(D3DXCOLOR)));

	// All objects use the same material (except the shadow).
	HR(mFX->SetValue(mhAmbientMtrl, &mWhiteMtrl.ambient, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhDiffuseMtrl, &mWhiteMtrl.diffuse, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhSpecularMtrl, &mWhiteMtrl.spec, sizeof(D3DXCOLOR)));
	HR(mFX->SetFloat(mhSpecularPower, mWhiteMtrl.specPower));

	drawRoom();
	drawMirror();
	drawTeapot();
	drawReflectedTeapot();

	HR(mFX->SetValue(mhAmbientMtrl, &mShadowMtrl.ambient, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhDiffuseMtrl, &mShadowMtrl.diffuse, sizeof(D3DXCOLOR)));
	HR(mFX->SetValue(mhSpecularMtrl, &mShadowMtrl.spec, sizeof(D3DXCOLOR)));
	HR(mFX->SetFloat(mhSpecularPower, mShadowMtrl.specPower));
	drawTeapotShadow();

	mGfxStats->display();

	HR(gd3dDevice->EndScene());

	// Present the backbuffer.
	HR(gd3dDevice->Present(0, 0, 0, 0));
}

void ShadowDemo::buildRoomGeometry()
{
	// Create and specify geometry.  For this sample we draw a floor
	// and a wall with a mirror on it.  We put the floor, wall, and
	// mirror geometry in one vertex buffer.
	//
    //   |----|----|----|
    //   |Wall|Mirr|Wall|
	//   |    | or |    |
    //   /--------------/
    //  /   Floor      /
	// /--------------/

	// Create the vertex buffer.
	HR(gd3dDevice->CreateVertexBuffer(24 * sizeof(VertexPNT), D3DUSAGE_WRITEONLY,
		0, D3DPOOL_MANAGED,	&mRoomVB, 0));

	// Write box vertices to the vertex buffer.
	VertexPNT* v = 0;
	HR(mRoomVB->Lock(0, 0, (void**)&v, 0));

	// Floor: Observe we tile texture coordinates.
	v[0] = VertexPNT(-7.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f);
	v[1] = VertexPNT(-7.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	v[2] = VertexPNT( 7.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f);
	
	v[3] = VertexPNT(-7.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f);
	v[4] = VertexPNT( 7.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f);
	v[5] = VertexPNT( 7.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 4.0f, 4.0f);

	// Wall: Observe we tile texture coordinates, and that we
	// leave a gap in the middle for the mirror.
	v[6]  = VertexPNT(-7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
	v[7]  = VertexPNT(-7.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[8]  = VertexPNT(-2.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f);
	
	v[9]  = VertexPNT(-7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
	v[10] = VertexPNT(-2.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f);
	v[11] = VertexPNT(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 2.0f);

	v[12] = VertexPNT(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
	v[13] = VertexPNT(2.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[14] = VertexPNT(7.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f);
	
	v[15] = VertexPNT(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
	v[16] = VertexPNT(7.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f);
	v[17] = VertexPNT(7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 2.0f);

	// Mirror
	v[18] = VertexPNT(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[19] = VertexPNT(-2.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[20] = VertexPNT( 2.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	
	v[21] = VertexPNT(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[22] = VertexPNT( 2.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	v[23] = VertexPNT( 2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	HR(mRoomVB->Unlock());
}

void ShadowDemo::buildFX()
{
	// Create the FX from a .fx file.
	ID3DXBuffer* errors = 0;
	HR(D3DXCreateEffectFromFile(gd3dDevice, "DirLightTex.fx", 
		0, 0, D3DXSHADER_DEBUG, 0, &mFX, &errors));
	if( errors )
		MessageBox(0, (char*)errors->GetBufferPointer(), 0, 0);

	// Obtain handles.
	mhTech          = mFX->GetTechniqueByName("DirLightTexTech");
	mhWVP           = mFX->GetParameterByName(0, "gWVP");
	mhWorldInvTrans = mFX->GetParameterByName(0, "gWorldInvTrans");
	mhLightVecW     = mFX->GetParameterByName(0, "gLightVecW");
	mhDiffuseMtrl   = mFX->GetParameterByName(0, "gDiffuseMtrl");
	mhDiffuseLight  = mFX->GetParameterByName(0, "gDiffuseLight");
	mhAmbientMtrl   = mFX->GetParameterByName(0, "gAmbientMtrl");
	mhAmbientLight  = mFX->GetParameterByName(0, "gAmbientLight");
	mhSpecularMtrl  = mFX->GetParameterByName(0, "gSpecularMtrl");
	mhSpecularLight = mFX->GetParameterByName(0, "gSpecularLight");
	mhSpecularPower = mFX->GetParameterByName(0, "gSpecularPower");
	mhEyePos        = mFX->GetParameterByName(0, "gEyePosW");
	mhWorld         = mFX->GetParameterByName(0, "gWorld");
	mhTex           = mFX->GetParameterByName(0, "gTex");
}

void ShadowDemo::buildViewMtx()
{
	float x = mCameraRadius * cosf(mCameraRotationY);
	float z = mCameraRadius * sinf(mCameraRotationY);
	D3DXVECTOR3 pos(x, mCameraHeight, z);
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
	D3DXMatrixLookAtLH(&mView, &pos, &target, &up);

	HR(mFX->SetValue(mhEyePos, &pos, sizeof(D3DXVECTOR3)));
}

void ShadowDemo::buildProjMtx()
{
	float w = (float)md3dPP.BackBufferWidth;
	float h = (float)md3dPP.BackBufferHeight;
	D3DXMatrixPerspectiveFovLH(&mProj, D3DX_PI * 0.25f, w/h, 1.0f, 5000.0f);
}

void ShadowDemo::drawRoom()
{
	HR(mFX->SetMatrix(mhWVP, &(mRoomWorld*mView*mProj)));
	D3DXMATRIX worldInvTrans;
	D3DXMatrixInverse(&worldInvTrans, 0, &mRoomWorld);
	D3DXMatrixTranspose(&worldInvTrans, &worldInvTrans);
	HR(mFX->SetMatrix(mhWorldInvTrans, &worldInvTrans));
	HR(mFX->SetMatrix(mhWorld, &mRoomWorld));

	HR(gd3dDevice->SetVertexDeclaration(VertexPNT::Decl));
	HR(gd3dDevice->SetStreamSource(0, mRoomVB, 0, sizeof(VertexPNT)));

	// Begin passes.
	UINT numPasses = 0;
	HR(mFX->Begin(&numPasses, 0));
	for(UINT i = 0; i < numPasses; ++i)
	{
		HR(mFX->BeginPass(i));

		// draw the floor
		HR(mFX->SetTexture(mhTex, mFloorTex));
		HR(mFX->CommitChanges());
		HR(gd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2));

		// draw the walls
		HR(mFX->SetTexture(mhTex, mWallTex));
		HR(mFX->CommitChanges());
		HR(gd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 6, 4));

		HR(mFX->EndPass());
	}
	HR(mFX->End());
}

void ShadowDemo::drawMirror()
{
	HR(mFX->SetMatrix(mhWVP, &(mRoomWorld*mView*mProj)));
	D3DXMATRIX worldInvTrans;
	D3DXMatrixInverse(&worldInvTrans, 0, &mRoomWorld);
	D3DXMatrixTranspose(&worldInvTrans, &worldInvTrans);
	HR(mFX->SetMatrix(mhWorldInvTrans, &worldInvTrans));
	HR(mFX->SetMatrix(mhWorld, &mRoomWorld));
	HR(mFX->SetTexture(mhTex, mMirrorTex));

	HR(gd3dDevice->SetVertexDeclaration(VertexPNT::Decl));
	HR(gd3dDevice->SetStreamSource(0, mRoomVB, 0, sizeof(VertexPNT)));

	// Begin passes.
	UINT numPasses = 0;
	HR(mFX->Begin(&numPasses, 0));
	for(UINT i = 0; i < numPasses; ++i)
	{
		HR(mFX->BeginPass(i));

		// draw the mirror
		HR(gd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 18, 2));

		HR(mFX->EndPass());
	}
	HR(mFX->End());
}

void ShadowDemo::drawTeapot()
{
	// Cylindrically interpolate texture coordinates.
	HR(gd3dDevice->SetRenderState(D3DRS_WRAP0, D3DWRAPCOORD_0));

	HR(mFX->SetMatrix(mhWVP, &(mTeapotWorld*mView*mProj)));
	D3DXMATRIX worldInvTrans;
	D3DXMatrixInverse(&worldInvTrans, 0, &mTeapotWorld);
	D3DXMatrixTranspose(&worldInvTrans, &worldInvTrans);
	HR(mFX->SetMatrix(mhWorldInvTrans, &worldInvTrans));
	HR(mFX->SetMatrix(mhWorld, &mTeapotWorld));
	HR(mFX->SetTexture(mhTex, mTeapotTex));

	// Begin passes.
	UINT numPasses = 0;
	HR(mFX->Begin(&numPasses, 0));
	for(UINT i = 0; i < numPasses; ++i)
	{
		HR(mFX->BeginPass(i));
		HR(mTeapot->DrawSubset(0));
		HR(mFX->EndPass());
	}
	HR(mFX->End());

	// Disable wrap.
	HR(gd3dDevice->SetRenderState(D3DRS_WRAP0, 0));
}

void ShadowDemo::drawReflectedTeapot()
{
	HR(gd3dDevice->SetRenderState(D3DRS_STENCILENABLE,    true));
    HR(gd3dDevice->SetRenderState(D3DRS_STENCILFUNC,      D3DCMP_ALWAYS));
    HR(gd3dDevice->SetRenderState(D3DRS_STENCILREF,       0x1));
    HR(gd3dDevice->SetRenderState(D3DRS_STENCILMASK,      0xffffffff));
    HR(gd3dDevice->SetRenderState(D3DRS_STENCILWRITEMASK, 0xffffffff));
    HR(gd3dDevice->SetRenderState(D3DRS_STENCILZFAIL,     D3DSTENCILOP_KEEP));
    HR(gd3dDevice->SetRenderState(D3DRS_STENCILFAIL,      D3DSTENCILOP_KEEP));
    HR(gd3dDevice->SetRenderState(D3DRS_STENCILPASS,      D3DSTENCILOP_REPLACE));

	// disable writes to the depth and back buffers
    HR(gd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, false));
    HR(gd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true));
    HR(gd3dDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_ZERO));
    HR(gd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE));

	// Draw mirror to stencil only.
	drawMirror();

	// Re-enable depth writes
	HR(gd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, true ));

	// Only draw reflected teapot to the pixels where the mirror
	// was drawn to.
	HR(gd3dDevice->SetRenderState(D3DRS_STENCILFUNC,  D3DCMP_EQUAL));
    HR(gd3dDevice->SetRenderState(D3DRS_STENCILPASS,  D3DSTENCILOP_KEEP));

	// Build Reflection transformation.
	D3DXMATRIX R;
	D3DXPLANE plane(0.0f, 0.0f, 1.0f, 0.0f); // xy plane
	D3DXMatrixReflect(&R, &plane);

	// Save the original teapot world matrix.
	D3DXMATRIX oldTeapotWorld = mTeapotWorld;

	// Add reflection transform.
	mTeapotWorld = mTeapotWorld * R;

	// Reflect light vector also.
	D3DXVECTOR3 oldLightVecW = mLightVecW;
	D3DXVec3TransformNormal(&mLightVecW, &mLightVecW, &R);
	HR(mFX->SetValue(mhLightVecW, &mLightVecW, sizeof(D3DXVECTOR3)));

	// Disable depth buffer and render the reflected teapot.
	HR(gd3dDevice->SetRenderState(D3DRS_ZENABLE, false));
	HR(gd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false));

	// Finally, draw the reflected teapot
	HR(gd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW));
	drawTeapot();
	mTeapotWorld = oldTeapotWorld;
	mLightVecW   = oldLightVecW;
	
	// Restore render states.
	HR(gd3dDevice->SetRenderState(D3DRS_ZENABLE, true));
	HR(gd3dDevice->SetRenderState( D3DRS_STENCILENABLE, false));
	HR(gd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW));
}

void ShadowDemo::drawTeapotShadow()
{
	HR(gd3dDevice->SetRenderState(D3DRS_STENCILENABLE,    true));
    HR(gd3dDevice->SetRenderState(D3DRS_STENCILFUNC,      D3DCMP_EQUAL));
    HR(gd3dDevice->SetRenderState(D3DRS_STENCILREF,       0x0));
    HR(gd3dDevice->SetRenderState(D3DRS_STENCILMASK,      0xffffffff));
    HR(gd3dDevice->SetRenderState(D3DRS_STENCILWRITEMASK, 0xffffffff));
    HR(gd3dDevice->SetRenderState(D3DRS_STENCILZFAIL,     D3DSTENCILOP_KEEP));
    HR(gd3dDevice->SetRenderState(D3DRS_STENCILFAIL,      D3DSTENCILOP_KEEP));
    HR(gd3dDevice->SetRenderState(D3DRS_STENCILPASS,      D3DSTENCILOP_INCR)); 

	// Position shadow.
	D3DXVECTOR4 lightDirection(0.577f, -0.577f, 0.577f, 0.0f);
	D3DXPLANE groundPlane(0.0f, -1.0f, 0.0f, 0.0f);

	D3DXMATRIX S;
	D3DXMatrixShadow(&S, &lightDirection, &groundPlane);

	// Offset the shadow up slightly so that there is no
	// z-fighting with the shadow and ground.
	D3DXMATRIX eps;
	D3DXMatrixTranslation(&eps, 0.0f, 0.001f, 0.0f);

	// Save the original teapot world matrix.
	D3DXMATRIX oldTeapotWorld = mTeapotWorld;

	// Add shadow projection transform.
	mTeapotWorld = mTeapotWorld * S * eps;

	// Alpha blend the shadow.
	HR(gd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true));
	HR(gd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
	HR(gd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));

	drawTeapot();

	// Restore settings.
	mTeapotWorld = oldTeapotWorld;
	HR(gd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false));
	HR(gd3dDevice->SetRenderState(D3DRS_STENCILENABLE,    false));
}

void ShadowDemo::genSphericalTexCoords()
{
	// D3DXCreate* functions generate vertices with position 
	// and normal data.  But for texturing, we also need
	// tex-coords.  So clone the mesh to change the vertex
	// format to a format with tex-coords.

	D3DVERTEXELEMENT9 elements[64];
	UINT numElements = 0;
	VertexPNT::Decl->GetDeclaration(elements, &numElements);

	ID3DXMesh* temp = 0;
	HR(mTeapot->CloneMesh(D3DXMESH_SYSTEMMEM, 
		elements, gd3dDevice, &temp));

	ReleaseCOM(mTeapot);

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
		elements, gd3dDevice, &mTeapot));

	ReleaseCOM(temp);
}