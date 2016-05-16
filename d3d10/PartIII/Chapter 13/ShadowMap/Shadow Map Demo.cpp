//=============================================================================
// Shadow Map Demo.cpp by Frank Luna (C) 2008 All Rights Reserved.
//
// Demonstrates shadow mapping.
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
#include "DrawableTex2D.h"

struct QuadVertex
{
	D3DXVECTOR3 pos;
	D3DXVECTOR2 texC;
};

class ShadowMapApp : public D3DApp
{
public:
	ShadowMapApp(HINSTANCE hInstance);
	~ShadowMapApp();

	void initApp();
	void onResize();
	void updateScene(float dt);
	void drawScene(); 

	LRESULT msgProc(UINT msg, WPARAM wParam, LPARAM lParam);

	void drawSceneToShadowMap();
	void buildNDCQuad();
private:

	POINT mOldMousePos;

	Quad mFloor;
	Box mBase;
	Sphere mBall;
	Cylinder mColumn;
	Sky mSky;

	// for drawing shadow map on screen
	ID3D10Buffer* mNDCQuadVB;

	DrawableTex2D mShadowMap;

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

	D3DXMATRIX mLightView;
	D3DXMATRIX mLightVolume;

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
	ID3D10EffectMatrixVariable* mfxLightWVPVar;
	ID3D10EffectMatrixVariable* mfxWVPVar;
	ID3D10EffectMatrixVariable* mfxWorldVar;
	ID3D10EffectMatrixVariable* mfxTexMtxVar;
	ID3D10EffectVectorVariable* mfxReflectMtrlVar;
	ID3D10EffectScalarVariable* mfxCubeMapEnabledVar;
	ID3D10EffectShaderResourceVariable* mfxDiffuseMapVar;
	ID3D10EffectShaderResourceVariable* mfxSpecMapVar;
	ID3D10EffectShaderResourceVariable* mfxNormalMapVar;
	ID3D10EffectShaderResourceVariable* mfxShadowMapVar;
	ID3D10EffectShaderResourceVariable* mfxCubeMapVar;


	ID3D10EffectTechnique* mBuildShadowMapTech;
	ID3D10EffectMatrixVariable* mfxBuildShadowMapLightWVPVar;
	ID3D10EffectShaderResourceVariable* mfxBuildShadowMapDiffuseMapVar;

	ID3D10EffectTechnique* mDrawShadowMapTech;
	ID3D10EffectShaderResourceVariable* mfxDrawShadowMapTexVar;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif


	ShadowMapApp theApp(hInstance);
	
	theApp.initApp();

	return theApp.run();
}

ShadowMapApp::ShadowMapApp(HINSTANCE hInstance)
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

ShadowMapApp::~ShadowMapApp()
{
	if( md3dDevice )
		md3dDevice->ClearState();

	ReleaseCOM(mNDCQuadVB);

	fx::DestroyAll();
	InputLayout::DestroyAll();
}

void ShadowMapApp::initApp()
{
	D3DApp::initApp();

	fx::InitAll(md3dDevice);
	InputLayout::InitAll(md3dDevice);
	GetTextureMgr().init(md3dDevice);


	mTech                = fx::ShadowFX->GetTechniqueByName("ShadowTech");
	mfxLightVar          = fx::ShadowFX->GetVariableByName("gLight");
	mfxEyePosVar         = fx::ShadowFX->GetVariableByName("gEyePosW");
	mfxLightWVPVar       = fx::ShadowFX->GetVariableByName("gLightWVP")->AsMatrix();
	mfxWVPVar            = fx::ShadowFX->GetVariableByName("gWVP")->AsMatrix();
	mfxWorldVar          = fx::ShadowFX->GetVariableByName("gWorld")->AsMatrix();
	mfxTexMtxVar         = fx::ShadowFX->GetVariableByName("gTexMtx")->AsMatrix();
	mfxReflectMtrlVar    = fx::ShadowFX->GetVariableByName("gReflectMtrl")->AsVector();
	mfxCubeMapEnabledVar = fx::ShadowFX->GetVariableByName("gCubeMapEnabled")->AsScalar();
	mfxDiffuseMapVar     = fx::ShadowFX->GetVariableByName("gDiffuseMap")->AsShaderResource();
	mfxSpecMapVar        = fx::ShadowFX->GetVariableByName("gSpecMap")->AsShaderResource();
	mfxNormalMapVar      = fx::ShadowFX->GetVariableByName("gNormalMap")->AsShaderResource();
	mfxShadowMapVar      = fx::ShadowFX->GetVariableByName("gShadowMap")->AsShaderResource();
	mfxCubeMapVar        = fx::ShadowFX->GetVariableByName("gCubeMap")->AsShaderResource();

	mBuildShadowMapTech            = fx::BuildShadowMapFX->GetTechniqueByName("BuildShadowMapTech");
	mfxBuildShadowMapLightWVPVar   = fx::BuildShadowMapFX->GetVariableByName("gLightWVP")->AsMatrix();
	mfxBuildShadowMapDiffuseMapVar = fx::BuildShadowMapFX->GetVariableByName("gDiffuseMap")->AsShaderResource();


	mDrawShadowMapTech     = fx::DrawShadowMapFX->GetTechniqueByName("DrawShadowMapTech");
	mfxDrawShadowMapTexVar = fx::DrawShadowMapFX->GetVariableByName("gShadowMap")->AsShaderResource();

	buildNDCQuad();

	mShadowMap.init(md3dDevice, 1024, 1024, false, DXGI_FORMAT_UNKNOWN);
 
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
	mParallelLight.ambient  = D3DXCOLOR(0.3f, 0.3f, 0.3f, 1.0f);
	mParallelLight.diffuse  = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	mParallelLight.specular = D3DXCOLOR(0.5f, 0.5f, 0.5f, 1.0f);

 
	D3DXMatrixOrthoLH(&mLightVolume, 30.0f, 30.0f, 1.0f, 100.0f);
}

void ShadowMapApp::onResize()
{
	D3DApp::onResize();

	float aspect = (float)mClientWidth/mClientHeight;
	GetCamera().setLens(0.25f*PI, aspect, 0.5f, 1000.0f);
}

void ShadowMapApp::updateScene(float dt)
{
	D3DApp::updateScene(dt);

	// Update angles based on input to orbit camera around scene.
	if(GetAsyncKeyState('A') & 0x8000)	GetCamera().strafe(-1.3f*dt);
	if(GetAsyncKeyState('D') & 0x8000)	GetCamera().strafe(+1.3f*dt);
	if(GetAsyncKeyState('W') & 0x8000)	GetCamera().walk(+1.3f*dt);
	if(GetAsyncKeyState('S') & 0x8000)	GetCamera().walk(-1.3f*dt);
 
	GetCamera().rebuildView();

	// Animate light and keep shadow in sync.
	D3DXVECTOR3 lightPos;
	lightPos.x = 30.0f*cosf(0.25f*mTimer.getGameTime());
	lightPos.y = 20.0f;
	lightPos.z = 30.0f*sinf(0.25f*mTimer.getGameTime());

	D3DXMatrixLookAtLH(&mLightView, &lightPos,
		&D3DXVECTOR3(0.0f, 0.0f, 0.0f), &D3DXVECTOR3(0.0f, 1.0f, 0.0f));

	D3DXVECTOR3 lightDirection = -lightPos;
	D3DXVec3Normalize(&mParallelLight.dir, &lightDirection);
}

void ShadowMapApp::drawScene()
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

	//
	// Draw scene to the shadow map.
	//

	mShadowMap.begin();

	drawSceneToShadowMap();

	mShadowMap.end();

	// restore rendering to backbuffer
	resetOMTargetsAndViewport();

	//
	// Draw a quad with shadow map as texture to the screen so we can see
	// what the shadow map looks like.
	//

	UINT stride = sizeof(QuadVertex);
    UINT offset = 0;
    md3dDevice->IASetVertexBuffers(0, 1, &mNDCQuadVB, &stride, &offset);
	md3dDevice->IASetInputLayout(InputLayout::PosTex);
	mfxDrawShadowMapTexVar->SetResource(mShadowMap.depthMap());

	ID3D10EffectPass* pass = mDrawShadowMapTech->GetPassByIndex(0);
	pass->Apply(0);
	md3dDevice->Draw(6, 0);
	
	//
	// Draw the rest of the scene.
	//

	md3dDevice->IASetInputLayout(InputLayout::PosTangentNormalTex);

	// Set per frame constants.
	mfxEyePosVar->SetRawValue(&GetCamera().position(), 0, sizeof(D3DXVECTOR3));
	mfxLightVar->SetRawValue(&mParallelLight, 0, sizeof(Light));
	
	mfxCubeMapVar->SetResource(mEnvMapRV);
	mfxShadowMapVar->SetResource(mShadowMap.depthMap());
 
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
		mfxWVPVar->SetMatrix((float*)&(mFloorWorld*view*proj));
		mfxLightWVPVar->SetMatrix((float*)&(mFloorWorld*mLightView*mLightVolume));
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
		mfxWVPVar->SetMatrix((float*)&(mBaseWorld*view*proj));
		mfxLightWVPVar->SetMatrix((float*)&(mBaseWorld*mLightView*mLightVolume));
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
		mfxWVPVar->SetMatrix((float*)&(mCenterBallWorld*view*proj));
		mfxLightWVPVar->SetMatrix((float*)&(mCenterBallWorld*mLightView*mLightVolume));
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
			mfxWVPVar->SetMatrix((float*)&(mColumnWorld[i]*view*proj));
	    	mfxLightWVPVar->SetMatrix((float*)&(mColumnWorld[i]*mLightView*mLightVolume));
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
			mfxWVPVar->SetMatrix((float*)&(mBallWorld[i]*view*proj));
			mfxLightWVPVar->SetMatrix((float*)&(mBallWorld[i]*mLightView*mLightVolume));
			mfxWorldVar->SetMatrix((float*)&mBallWorld[i]);
			mfxReflectMtrlVar->SetFloatVector((float*)&mReflectMtrl[i]);
			pass->Apply(0);
			mBall.draw();
		}

		// Unbind shadow map from shader stage since we will be binding it
		// as a depth buffer when we rebuild the shadow map the next frame.
		mfxShadowMapVar->SetResource(0);
		pass->Apply(0);
    }

	// Draw sky last to save fill rate.
	mSky.draw();


	// We specify DT_NOCLIP, so we do not care about width/height of the rect.
	RECT R = {5, 5, 0, 0};
	md3dDevice->RSSetState(0);
	mFont->DrawText(0, mFrameStats.c_str(), -1, &R, DT_NOCLIP, WHITE);

	mSwapChain->Present(0, 0);
}

LRESULT ShadowMapApp::msgProc(UINT msg, WPARAM wParam, LPARAM lParam)
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
 
void ShadowMapApp::drawSceneToShadowMap()
{
	D3D10_TECHNIQUE_DESC techDesc;
    mBuildShadowMapTech->GetDesc( &techDesc );

    for(UINT i = 0; i < techDesc.Passes; ++i)
    {
        ID3D10EffectPass* pass = mBuildShadowMapTech->GetPassByIndex(i);

		//
		// draw floor
		//
		D3DXMATRIX floorWVP = mFloorWorld*mLightView*mLightVolume;
		mfxBuildShadowMapLightWVPVar->SetMatrix((float*)&floorWVP);
		mfxBuildShadowMapDiffuseMapVar->SetResource(mFloorMapRV);
		pass->Apply(0);
		mFloor.draw();
		//
		// draw base
		//
		D3DXMATRIX baseWVP = mBaseWorld*mLightView*mLightVolume;
		mfxBuildShadowMapLightWVPVar->SetMatrix((float*)&baseWVP);
		mfxBuildShadowMapDiffuseMapVar->SetResource(mBaseMapRV);
		pass->Apply(0);
		mBase.draw();
		//
		// draw center ball
		//
		D3DXMATRIX centerBallWVP = mCenterBallWorld*mLightView*mLightVolume;
		mfxBuildShadowMapLightWVPVar->SetMatrix((float*)&centerBallWVP);
		mfxBuildShadowMapDiffuseMapVar->SetResource(mBallMapRV);
		pass->Apply(0);
		mBall.draw();
		//
		// draw columns
		//
		for(int i = 0; i < 10; ++i)
		{
			D3DXMATRIX columnWVP = mColumnWorld[i]*mLightView*mLightVolume;
			mfxBuildShadowMapLightWVPVar->SetMatrix((float*)&columnWVP);
			mfxBuildShadowMapDiffuseMapVar->SetResource(mColumnMapRV);
			pass->Apply(0);
			mColumn.draw();
		}	
		//
		// draw balls
		//
		for(int i = 0; i < 10; ++i)
		{
			D3DXMATRIX ballWVP = mBallWorld[i]*mLightView*mLightVolume;
			mfxBuildShadowMapLightWVPVar->SetMatrix((float*)&ballWVP);
			mfxBuildShadowMapDiffuseMapVar->SetResource(mBallMapRV);
			pass->Apply(0);
			mBall.draw();
		}
    }
}

void ShadowMapApp::buildNDCQuad()
{
	D3DXVECTOR3 pos[] = 
	{
		D3DXVECTOR3(0.0f, -1.0f, 0.0f),
		D3DXVECTOR3(0.0f,  0.0f, 0.0f),
		D3DXVECTOR3(1.0f,  0.0f, 0.0f),

		D3DXVECTOR3(0.0f, -1.0f, 0.0f),
		D3DXVECTOR3(1.0f,  0.0f, 0.0f),
		D3DXVECTOR3(1.0f, -1.0f, 0.0f)
	};

	D3DXVECTOR2 tex[] = 
	{
		D3DXVECTOR2(0.0f, 1.0f),
		D3DXVECTOR2(0.0f, 0.0f),
		D3DXVECTOR2(1.0f, 0.0f),

		D3DXVECTOR2(0.0f, 1.0f),
		D3DXVECTOR2(1.0f, 0.0f),
		D3DXVECTOR2(1.0f, 1.0f)
	};

	QuadVertex qv[6];

	for(int i = 0; i < 6; ++i)
	{
		qv[i].pos  = pos[i];
		qv[i].texC = tex[i];
	}

	D3D10_BUFFER_DESC vbd;
    vbd.Usage = D3D10_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(QuadVertex) * 6;
    vbd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D10_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = qv;
    HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mNDCQuadVB));
}