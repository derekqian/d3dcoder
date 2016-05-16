//=============================================================================
// Normal Map Demo.cpp by Frank Luna (C) 2008 All Rights Reserved.
//
// Demonstrates normal mapping.
//
// Controls:
//		'A'/'D'/'W'/'S' - Move
//      Hold down left mouse button and move mouse to rotate.
//
//=============================================================================

#include "d3dApp.h"
#include "Light.h"
#include "Box.h"
#include "Quad.h"
#include "Cylinder.h"
#include "Sphere.h"
#include "Camera.h"
#include "Effects.h"
#include "InputLayouts.h"
#include "TextureMgr.h"
#include "Sky.h"


class NormalMapApp : public D3DApp
{
public:
	NormalMapApp(HINSTANCE hInstance);
	~NormalMapApp();

	void initApp();
	void onResize();
	void updateScene(float dt);
	void drawScene(); 

	LRESULT msgProc(UINT msg, WPARAM wParam, LPARAM lParam);

private:

	POINT mOldMousePos;

	Quad mFloor;
	Box mBase;
	Sphere mBall;
	Cylinder mColumn;
	Sky mSky;

	D3DXVECTOR4 mReflectNone;
	D3DXVECTOR4 mReflectAll;

	D3DXVECTOR4 mReflectMtrl[10];

	D3DXMATRIX mFloorWorld;
	D3DXMATRIX mBaseWorld;
	D3DXMATRIX mCenterBallWorld;
	D3DXMATRIX mColumnWorld[10];
	D3DXMATRIX mBallWorld[10];

	D3DXMATRIX mFloorTexMtx;
	D3DXMATRIX mIdentityTexMtx;


	Light mParallelLight;

	ID3D10ShaderResourceView* mFloorMapRV;
	ID3D10ShaderResourceView* mFloorNormalMapRV;
	ID3D10ShaderResourceView* mBaseMapRV;
	ID3D10ShaderResourceView* mBaseNormalMapRV;
	ID3D10ShaderResourceView* mBallMapRV;
	ID3D10ShaderResourceView* mColumnMapRV;
	ID3D10ShaderResourceView* mColumnNormalMapRV;
	ID3D10ShaderResourceView* mSpecMapRV;
	ID3D10ShaderResourceView* mDefaultSpecMapRV;
	ID3D10ShaderResourceView* mDefaultNormalMapRV;
	ID3D10ShaderResourceView* mEnvMapRV;
 
	ID3D10EffectTechnique* mTech;
	ID3D10EffectVariable* mfxLightVar;
	ID3D10EffectVariable* mfxEyePosVar;
	ID3D10EffectMatrixVariable* mfxWVPVar;
	ID3D10EffectMatrixVariable* mfxWorldVar;
	ID3D10EffectMatrixVariable* mfxTexMtxVar;
	ID3D10EffectVectorVariable* mfxReflectMtrlVar;
	ID3D10EffectScalarVariable* mfxCubeMapEnabledVar;
	ID3D10EffectShaderResourceVariable* mfxDiffuseMapVar;
	ID3D10EffectShaderResourceVariable* mfxSpecMapVar;
	ID3D10EffectShaderResourceVariable* mfxNormalMapVar;
	ID3D10EffectShaderResourceVariable* mfxCubeMapVar;
	
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif


	NormalMapApp theApp(hInstance);
	
	theApp.initApp();

	return theApp.run();
}

NormalMapApp::NormalMapApp(HINSTANCE hInstance)
: D3DApp(hInstance), mFloorMapRV(0), mFloorNormalMapRV(0), 
  mBaseMapRV(0), mBaseNormalMapRV(0), mBallMapRV(0), mColumnMapRV(0), 
  mColumnNormalMapRV(0), mSpecMapRV(0), mDefaultNormalMapRV(0), mEnvMapRV(0)
{
	
	D3DXMatrixIdentity(&mFloorWorld);
	D3DXMatrixTranslation(&mBaseWorld, 0.0f, 1.0f, 0.0f);

	D3DXMATRIX S, T;
	D3DXMatrixScaling(&S, 2.0f, 2.0f, 2.0f);
	D3DXMatrixTranslation(&T, 0.0f, 3.0f, 0.0f);
	mCenterBallWorld = S*T;
 
	for(int i = 0; i < 5; ++i)
	{
		D3DXMatrixTranslation(&mColumnWorld[i*2+0], -5.0f, 1.5f, -12.0f + i*6.0f);
		D3DXMatrixTranslation(&mColumnWorld[i*2+1], +5.0f, 1.5f, -12.0f + i*6.0f);

		D3DXMatrixTranslation(&mBallWorld[i*2+0], -5.0f, 3.5f, -12.0f + i*6.0f);
		D3DXMatrixTranslation(&mBallWorld[i*2+1], +5.0f, 3.5f, -12.0f + i*6.0f);
	}

	mReflectMtrl[0] = D3DXVECTOR4(0.0f, 0.0f, 1.0f, 1.0f);
	mReflectMtrl[1] = D3DXVECTOR4(0.0f, 0.0f, 1.0f, 1.0f);
	mReflectMtrl[2] = D3DXVECTOR4(0.0f, 1.0f, 0.0f, 1.0f);
	mReflectMtrl[3] = D3DXVECTOR4(0.0f, 1.0f, 0.0f, 1.0f);
	mReflectMtrl[4] = D3DXVECTOR4(1.0f, 0.0f, 0.0f, 1.0f);
	mReflectMtrl[5] = D3DXVECTOR4(1.0f, 0.0f, 0.0f, 1.0f);
	mReflectMtrl[6] = D3DXVECTOR4(0.0f, 1.0f, 0.0f, 1.0f);
	mReflectMtrl[7] = D3DXVECTOR4(0.0f, 1.0f, 0.0f, 1.0f);
	mReflectMtrl[8] = D3DXVECTOR4(0.0f, 0.0f, 1.0f, 1.0f);
	mReflectMtrl[9] = D3DXVECTOR4(0.0f, 0.0f, 1.0f, 1.0f);


	D3DXMatrixScaling(&mFloorTexMtx, 10.0f, 20.0f, 1.0f);
	D3DXMatrixIdentity(&mIdentityTexMtx);
 
	mReflectNone = D3DXVECTOR4(0.0f, 0.0f, 0.0f, 1.0f);
	mReflectAll  = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
}

NormalMapApp::~NormalMapApp()
{
	if( md3dDevice )
		md3dDevice->ClearState();

	fx::DestroyAll();
	InputLayout::DestroyAll();
}

void NormalMapApp::initApp()
{
	D3DApp::initApp();

	fx::InitAll(md3dDevice);
	InputLayout::InitAll(md3dDevice);
	GetTextureMgr().init(md3dDevice);


	mTech                = fx::NormalMapFX->GetTechniqueByName("NormalMapTech");
	mfxLightVar          = fx::NormalMapFX->GetVariableByName("gLight");
	mfxEyePosVar         = fx::NormalMapFX->GetVariableByName("gEyePosW");
	mfxWVPVar            = fx::NormalMapFX->GetVariableByName("gWVP")->AsMatrix();
	mfxWorldVar          = fx::NormalMapFX->GetVariableByName("gWorld")->AsMatrix();
	mfxTexMtxVar         = fx::NormalMapFX->GetVariableByName("gTexMtx")->AsMatrix();
	mfxReflectMtrlVar    = fx::NormalMapFX->GetVariableByName("gReflectMtrl")->AsVector();
	mfxCubeMapEnabledVar = fx::NormalMapFX->GetVariableByName("gCubeMapEnabled")->AsScalar();
	mfxDiffuseMapVar     = fx::NormalMapFX->GetVariableByName("gDiffuseMap")->AsShaderResource();
	mfxSpecMapVar        = fx::NormalMapFX->GetVariableByName("gSpecMap")->AsShaderResource();
	mfxNormalMapVar      = fx::NormalMapFX->GetVariableByName("gNormalMap")->AsShaderResource();
	mfxCubeMapVar        = fx::NormalMapFX->GetVariableByName("gCubeMap")->AsShaderResource();

 
	mClearColor = D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f);

	GetCamera().position() = D3DXVECTOR3(0.0f, 1.8f, -10.0f);

	mFloorMapRV         = GetTextureMgr().createTex(L"floor_diffuse.dds");
	mFloorNormalMapRV   = GetTextureMgr().createTex(L"floor_normal.dds");
	mBaseMapRV          = GetTextureMgr().createTex(L"stone_diffuse.dds");
	mBaseNormalMapRV    = GetTextureMgr().createTex(L"stone_normal.dds");
	mBallMapRV          = GetTextureMgr().createTex(L"blackdiffuse.dds");
	mColumnMapRV        = GetTextureMgr().createTex(L"bricks_diffuse.dds");
	mColumnNormalMapRV  = GetTextureMgr().createTex(L"bricks_normal.dds");
	mSpecMapRV          = GetTextureMgr().createTex(L"spec.dds");
	mDefaultSpecMapRV   = GetTextureMgr().createTex(L"defaultspec.dds");
	mDefaultNormalMapRV = GetTextureMgr().createTex(L"default_normal.dds");
	mEnvMapRV           = GetTextureMgr().createCubeTex(L"grassenvmap1024.dds");

	mFloor.init(md3dDevice, 41, 21, 1.0f);
	mBase.init(md3dDevice, 2.0f);
	mBall.init(md3dDevice, 0.5f, 30, 30);
	mColumn.init(md3dDevice, 0.25f, 1.0f, 3.0f, 30, 30);
	mSky.init(md3dDevice, mEnvMapRV, 5000.0f);

 
	// direction updated at runtime
	//mParallelLight.dir      = D3DXVECTOR3(0.57735f, -0.57735f, 0.57735f);
	mParallelLight.ambient  = D3DXCOLOR(0.3f, 0.3f, 0.3f, 1.0f);
	mParallelLight.diffuse  = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	mParallelLight.specular = D3DXCOLOR(0.5f, 0.5f, 0.5f, 1.0f);
}

void NormalMapApp::onResize()
{
	D3DApp::onResize();

	float aspect = (float)mClientWidth/mClientHeight;
	GetCamera().setLens(0.25f*PI, aspect, 0.5f, 1000.0f);
}

void NormalMapApp::updateScene(float dt)
{
	D3DApp::updateScene(dt);

	// Update angles based on input to orbit camera around scene.
	if(GetAsyncKeyState('A') & 0x8000)	GetCamera().strafe(-1.3f*dt);
	if(GetAsyncKeyState('D') & 0x8000)	GetCamera().strafe(+1.3f*dt);
	if(GetAsyncKeyState('W') & 0x8000)	GetCamera().walk(+1.3f*dt);
	if(GetAsyncKeyState('S') & 0x8000)	GetCamera().walk(-1.3f*dt);
 
	GetCamera().rebuildView();

	D3DXVECTOR3 lightDirection;
	lightDirection.x = cosf(0.5f*mTimer.getGameTime());
	lightDirection.y = -0.5f;
	lightDirection.z = sinf(0.5f*mTimer.getGameTime());
	D3DXVec3Normalize(&mParallelLight.dir, &lightDirection);
}

void NormalMapApp::drawScene()
{
	D3DApp::drawScene();
	
	
	// Restore default states, input layout and primitive topology 
	// because mFont->DrawText changes them.  Note that we can 
	// restore the default states by passing null.
	md3dDevice->OMSetDepthStencilState(0, 0);
	float blendFactor[] = {0.0f, 0.0f, 0.0f, 0.0f};
	md3dDevice->OMSetBlendState(0, blendFactor, 0xffffffff);
	
	md3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	md3dDevice->IASetInputLayout(InputLayout::PosTangentNormalTex);

	// Set per frame constants.
	mfxEyePosVar->SetRawValue(&GetCamera().position(), 0, sizeof(D3DXVECTOR3));
	mfxLightVar->SetRawValue(&mParallelLight, 0, sizeof(Light));
	
	mfxCubeMapVar->SetResource(mEnvMapRV);
	

	D3DXMATRIX view = GetCamera().view();
	D3DXMATRIX proj = GetCamera().proj();

    D3D10_TECHNIQUE_DESC techDesc;
    mTech->GetDesc( &techDesc );

    for(UINT i = 0; i < techDesc.Passes; ++i)
    {
        ID3D10EffectPass* pass = mTech->GetPassByIndex(i);

		//
		// draw floor
		//
		D3DXMATRIX floorWVP = mFloorWorld*view*proj;
		mfxWVPVar->SetMatrix((float*)&floorWVP);
		mfxWorldVar->SetMatrix((float*)&mFloorWorld);
		mfxTexMtxVar->SetMatrix((float*)&mFloorTexMtx);
		mfxDiffuseMapVar->SetResource(mFloorMapRV);
		mfxNormalMapVar->SetResource(mFloorNormalMapRV);
		mfxSpecMapVar->SetResource(mSpecMapRV);
		mfxCubeMapEnabledVar->SetBool(false);
		mfxReflectMtrlVar->SetFloatVector((float*)&mReflectNone);
		pass->Apply(0);
		mFloor.draw();
		//
		// draw base
		//
		D3DXMATRIX baseWVP = mBaseWorld*view*proj;
		mfxWVPVar->SetMatrix((float*)&baseWVP);
		mfxWorldVar->SetMatrix((float*)&mBaseWorld);
		mfxTexMtxVar->SetMatrix((float*)&mIdentityTexMtx);
		mfxDiffuseMapVar->SetResource(mBaseMapRV);
		mfxNormalMapVar->SetResource(mBaseNormalMapRV);
		mfxSpecMapVar->SetResource(mSpecMapRV);
		mfxCubeMapEnabledVar->SetBool(false);
		mfxReflectMtrlVar->SetFloatVector((float*)&mReflectNone);
		pass->Apply(0);
		mBase.draw();

		//
		// draw center ball
		//
		D3DXMATRIX centerBallWVP = mCenterBallWorld*view*proj;
		mfxWVPVar->SetMatrix((float*)&centerBallWVP);
		mfxWorldVar->SetMatrix((float*)&mCenterBallWorld);
		mfxTexMtxVar->SetMatrix((float*)&mIdentityTexMtx);
		mfxDiffuseMapVar->SetResource(mBallMapRV);
		mfxNormalMapVar->SetResource(mDefaultNormalMapRV);
		mfxSpecMapVar->SetResource(mDefaultSpecMapRV);
		mfxCubeMapEnabledVar->SetBool(true);
		mfxReflectMtrlVar->SetFloatVector((float*)&mReflectAll);
		pass->Apply(0);
		mBall.draw();

		//
		// draw columns
		//
		mfxTexMtxVar->SetMatrix((float*)&mIdentityTexMtx);
		mfxCubeMapEnabledVar->SetBool(false);
		mfxReflectMtrlVar->SetFloatVector((float*)&mReflectNone);
		mfxDiffuseMapVar->SetResource(mColumnMapRV);
		mfxNormalMapVar->SetResource(mColumnNormalMapRV);
		mfxSpecMapVar->SetResource(mSpecMapRV);
		for(int i = 0; i < 10; ++i)
		{
			D3DXMATRIX columnWVP = mColumnWorld[i]*view*proj;
			mfxWVPVar->SetMatrix((float*)&columnWVP);
			mfxWorldVar->SetMatrix((float*)&mColumnWorld[i]);
			pass->Apply(0);
			mColumn.draw();
		}	
		//
		// draw balls
		//
		mfxTexMtxVar->SetMatrix((float*)&mIdentityTexMtx);
		mfxCubeMapEnabledVar->SetBool(true);
		mfxDiffuseMapVar->SetResource(mBallMapRV);
		mfxNormalMapVar->SetResource(mDefaultNormalMapRV);
		mfxSpecMapVar->SetResource(mDefaultSpecMapRV);
		for(int i = 0; i < 10; ++i)
		{
			D3DXMATRIX ballWVP = mBallWorld[i]*view*proj;
			mfxWVPVar->SetMatrix((float*)&ballWVP);
			mfxWorldVar->SetMatrix((float*)&mBallWorld[i]);
			mfxReflectMtrlVar->SetFloatVector((float*)&mReflectMtrl[i]);
			pass->Apply(0);
			mBall.draw();
		}
    }

	// Draw sky last to save fill rate.
	mSky.draw();


	// We specify DT_NOCLIP, so we do not care about width/height of the rect.
	RECT R = {5, 5, 0, 0};
	md3dDevice->RSSetState(0);
	mFont->DrawText(0, mFrameStats.c_str(), -1, &R, DT_NOCLIP, WHITE);

	mSwapChain->Present(0, 0);
}

LRESULT NormalMapApp::msgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	POINT mousePos;
	int dx = 0;
	int dy = 0;
	switch(msg)
	{
	case WM_LBUTTONDOWN:
		if( wParam & MK_LBUTTON )
		{
			SetCapture(mhMainWnd);

			mOldMousePos.x = LOWORD(lParam);
			mOldMousePos.y = HIWORD(lParam);
		}
		return 0;

	case WM_LBUTTONUP:
		ReleaseCapture();
		return 0;

	case WM_MOUSEMOVE:
		if( wParam & MK_LBUTTON )
		{
			mousePos.x = (int)LOWORD(lParam); 
			mousePos.y = (int)HIWORD(lParam); 

			dx = mousePos.x - mOldMousePos.x;
			dy = mousePos.y - mOldMousePos.y;

			GetCamera().pitch( dy * 0.0087266f );
			GetCamera().rotateY( dx * 0.0087266f );
			
			mOldMousePos = mousePos;
		}
		return 0;
	}

	return D3DApp::msgProc(msg, wParam, lParam);
}
 
 