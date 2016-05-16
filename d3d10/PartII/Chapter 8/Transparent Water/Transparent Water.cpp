//=============================================================================
// Transparent Water.cpp by Frank Luna (C) 2008 All Rights Reserved.
//
// Demonstrates alpha blending by making the water transparent.
//
// Controls:
//		'A'/'D'/'W'/'S' - Rotate 
//              'Z'/'X' - Zoom
//
//=============================================================================

#include "d3dApp.h"
#include "PeaksAndValleys.h"
#include "Waves.h"
#include "Light.h"
#include "Box.h"

class TransparentWaterApp : public D3DApp
{
public:
	TransparentWaterApp(HINSTANCE hInstance);
	~TransparentWaterApp();

	void initApp();
	void onResize();
	void updateScene(float dt);
	void drawScene(); 

private:
	void buildFX();
	void buildVertexLayouts();

private:
 
	Box mBox;
	PeaksAndValleys mLand;
	Waves mWaves;

	Light mParallelLight;

	// translate water tex-coords.
	D3DXVECTOR2 mWaterTexOffset;

	ID3D10BlendState* mTransparentBS;
 
	ID3D10Effect* mFX;
	ID3D10EffectTechnique* mTech;
	ID3D10ShaderResourceView* mGrassMapRV;
	ID3D10ShaderResourceView* mBoxMapRV;
	ID3D10ShaderResourceView* mWaterMapRV;
	ID3D10ShaderResourceView* mDefaultSpecMapRV;

	ID3D10EffectMatrixVariable* mfxWVPVar;
	ID3D10EffectMatrixVariable* mfxWorldVar;
	ID3D10EffectVariable* mfxEyePosVar;
	ID3D10EffectVariable* mfxLightVar;
	ID3D10EffectShaderResourceVariable* mfxDiffuseMapVar;
	ID3D10EffectShaderResourceVariable* mfxSpecMapVar;
	ID3D10EffectMatrixVariable* mfxTexMtxVar;

	ID3D10InputLayout* mVertexLayout;
 
	D3DXMATRIX mLandWorld;
	D3DXMATRIX mCrateWorld;
	D3DXMATRIX mWavesWorld;

	D3DXMATRIX mView;
	D3DXMATRIX mProj;
	D3DXMATRIX mWVP;

	D3DXVECTOR3 mEyePos;
	float mRadius;
	float mTheta;
	float mPhi;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif


	TransparentWaterApp theApp(hInstance);
	
	theApp.initApp();

	return theApp.run();
}

TransparentWaterApp::TransparentWaterApp(HINSTANCE hInstance)
: D3DApp(hInstance), mFX(0), mTech(0), mfxWVPVar(0), mfxWorldVar(0), mfxEyePosVar(0),
  mfxLightVar(0), mfxDiffuseMapVar(0), mfxSpecMapVar(0), mfxTexMtxVar(0), 
  mVertexLayout(0), mGrassMapRV(0), mBoxMapRV(0), mWaterMapRV(0), mDefaultSpecMapRV(0), 
  mEyePos(0.0f, 0.0f, 0.0f), mRadius(75.0f), mTheta(0.0f), mPhi(PI*0.4f)
{
	D3DXMatrixTranslation(&mCrateWorld, 8.0f, 0.0f, -15.0f);
	D3DXMatrixIdentity(&mLandWorld);
	D3DXMatrixIdentity(&mWavesWorld);
	D3DXMatrixIdentity(&mView);
	D3DXMatrixIdentity(&mProj);
	D3DXMatrixIdentity(&mWVP); 
}

TransparentWaterApp::~TransparentWaterApp()
{
	if( md3dDevice )
		md3dDevice->ClearState();

	ReleaseCOM(mFX);
	ReleaseCOM(mVertexLayout);
	ReleaseCOM(mGrassMapRV);
	ReleaseCOM(mBoxMapRV);
	ReleaseCOM(mWaterMapRV);
	ReleaseCOM(mDefaultSpecMapRV);

	ReleaseCOM(mTransparentBS);
}

void TransparentWaterApp::initApp()
{
	D3DApp::initApp();

	mClearColor = D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f);

	mBox.init(md3dDevice, 5.0f);
	mLand.init(md3dDevice, 129, 129, 1.0f);

	// No wave damping.
	mWaves.init(md3dDevice, 257, 257, 0.5f, 0.03f, 3.25f, 0.0f);

	// Generate some waves at start up.
	for(int k = 0; k < 50; ++k)
	{ 
		DWORD i = 5 + rand() % 250;
		DWORD j = 5 + rand() % 250;

		float r = RandF(0.5f, 1.25f);

		mWaves.disturb(i, j, r);
	}

	
	buildFX();
	buildVertexLayouts();

	HR(D3DX10CreateShaderResourceViewFromFile(md3dDevice, 
		L"grass.dds", 0, 0, &mGrassMapRV, 0 ));

	HR(D3DX10CreateShaderResourceViewFromFile(md3dDevice, 
		L"WoodCrate01.dds", 0, 0, &mBoxMapRV, 0 ));

	HR(D3DX10CreateShaderResourceViewFromFile(md3dDevice, 
		L"water2a.dds", 0, 0, &mWaterMapRV, 0 ));

	HR(D3DX10CreateShaderResourceViewFromFile(md3dDevice, 
		L"defaultspec.dds", 0, 0, &mDefaultSpecMapRV, 0 ));

	mWaterTexOffset = D3DXVECTOR2(0.0f, 0.0f);

	mParallelLight.dir      = D3DXVECTOR3(0.57735f, -0.57735f, 0.57735f);
	mParallelLight.ambient  = D3DXCOLOR(0.2f, 0.2f, 0.2f, 1.0f);
	mParallelLight.diffuse  = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	mParallelLight.specular = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);


	D3D10_BLEND_DESC blendDesc = {0};
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.BlendEnable[0] = true;
	blendDesc.SrcBlend       = D3D10_BLEND_SRC_ALPHA;
	blendDesc.DestBlend      = D3D10_BLEND_INV_SRC_ALPHA;
	blendDesc.BlendOp        = D3D10_BLEND_OP_ADD;
	blendDesc.SrcBlendAlpha  = D3D10_BLEND_ONE;
	blendDesc.DestBlendAlpha = D3D10_BLEND_ZERO;
	blendDesc.BlendOpAlpha   = D3D10_BLEND_OP_ADD;
	blendDesc.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;

	HR(md3dDevice->CreateBlendState(&blendDesc, &mTransparentBS));
}

void TransparentWaterApp::onResize()
{
	D3DApp::onResize();

	float aspect = (float)mClientWidth/mClientHeight;
	D3DXMatrixPerspectiveFovLH(&mProj, 0.25f*PI, aspect, 1.0f, 1000.0f);
}

void TransparentWaterApp::updateScene(float dt)
{
	D3DApp::updateScene(dt);

	// Update angles based on input to orbit camera around scene.
	if(GetAsyncKeyState('A') & 0x8000)	mTheta -= 2.0f*dt;
	if(GetAsyncKeyState('D') & 0x8000)	mTheta += 2.0f*dt;
	if(GetAsyncKeyState('W') & 0x8000)	mPhi -= 2.0f*dt;
	if(GetAsyncKeyState('S') & 0x8000)	mPhi += 2.0f*dt;
	if(GetAsyncKeyState('Z') & 0x8000)	mRadius -= 15.0f*dt;
	if(GetAsyncKeyState('X') & 0x8000)	mRadius += 15.0f*dt;

	// Restrict the angle mPhi.
	if( mPhi < 0.1f )	mPhi = 0.1f;
	if( mPhi > PI-0.1f)	mPhi = PI-0.1f;

	// Convert Spherical to Cartesian coordinates: mPhi measured from +y
	// and mTheta measured counterclockwise from -z.
	mEyePos.x =  mRadius*sinf(mPhi)*sinf(mTheta);
	mEyePos.z = -mRadius*sinf(mPhi)*cosf(mTheta);
	mEyePos.y =  mRadius*cosf(mPhi);

	// Build the view matrix.
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
	D3DXMatrixLookAtLH(&mView, &mEyePos, &target, &up);


	// Animate water texture as a function of time.
	mWaterTexOffset.y += 0.1f*dt;
	mWaterTexOffset.x = 0.25f*sinf(4.0f*mWaterTexOffset.y); 


	mWaves.update(dt);
}

void TransparentWaterApp::drawScene()
{
	D3DApp::drawScene();
	
	
	// Restore default states, input layout and primitive topology 
	// because mFont->DrawText changes them.  Note that we can 
	// restore the default states by passing null.
	md3dDevice->OMSetDepthStencilState(0, 0);
	float blendFactor[] = {0.0f, 0.0f, 0.0f, 0.0f};
	md3dDevice->OMSetBlendState(0, blendFactor, 0xffffffff);
    md3dDevice->IASetInputLayout(mVertexLayout);
    md3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set per frame constants.
	mfxEyePosVar->SetRawValue(&mEyePos, 0, sizeof(D3DXVECTOR3));
	mfxLightVar->SetRawValue(&mParallelLight, 0, sizeof(Light));

	// Scale texture coordinates by 5 units to map [0,1]-->[0,5]
	// so that the texture repeats five times in each direction.
	D3DXMATRIX S;
	D3DXMatrixScaling(&S, 5.0f, 5.0f, 1.0f);
	D3DXMATRIX landTexMtx = S;
	
	// Don't modify box tex-coords.
	D3DXMATRIX boxTexMtx;
	D3DXMatrixIdentity(&boxTexMtx);

	// Scale and translate the texture.
	D3DXMATRIX T;
	D3DXMatrixTranslation(&T, mWaterTexOffset.x, mWaterTexOffset.y, 0.0f);
	D3DXMATRIX waterTexMtx = S*T;


    D3D10_TECHNIQUE_DESC techDesc;
    mTech->GetDesc( &techDesc );

    for(UINT i = 0; i < techDesc.Passes; ++i)
    {
        ID3D10EffectPass* pass = mTech->GetPassByIndex(i);

		//
		// Set land specific constants.
		//
		mWVP = mLandWorld*mView*mProj;
		mfxWVPVar->SetMatrix((float*)&mWVP);
		mfxWorldVar->SetMatrix((float*)&mLandWorld);
		mfxTexMtxVar->SetMatrix((float*)&landTexMtx);
		mfxDiffuseMapVar->SetResource(mGrassMapRV);
		mfxSpecMapVar->SetResource(mDefaultSpecMapRV);
		pass->Apply(0);
		mLand.draw();
		//
		// Set box specific constants.
		//
		mWVP = mCrateWorld*mView*mProj;
		mfxWVPVar->SetMatrix((float*)&mWVP);
		mfxWorldVar->SetMatrix((float*)&mCrateWorld);
		mfxTexMtxVar->SetMatrix((float*)&boxTexMtx);
		mfxDiffuseMapVar->SetResource(mBoxMapRV);
		mfxSpecMapVar->SetResource(mDefaultSpecMapRV);
		pass->Apply(0);
		mBox.draw();
		//
		// Set water specific constants.
		//
		mWVP = mWavesWorld*mView*mProj;
		mfxWVPVar->SetMatrix((float*)&mWVP);
		mfxWorldVar->SetMatrix((float*)&mWavesWorld);
		mfxTexMtxVar->SetMatrix((float*)&waterTexMtx);
		mfxDiffuseMapVar->SetResource(mWaterMapRV);
		mfxSpecMapVar->SetResource(mDefaultSpecMapRV);
		pass->Apply(0);
		md3dDevice->OMSetBlendState(mTransparentBS, blendFactor, 0xffffffff);
		mWaves.draw(); 
    }

	// We specify DT_NOCLIP, so we do not care about width/height of the rect.
	RECT R = {5, 5, 0, 0};
	md3dDevice->RSSetState(0);
	mFont->DrawText(0, mFrameStats.c_str(), -1, &R, DT_NOCLIP, WHITE);

	mSwapChain->Present(0, 0);
}

void TransparentWaterApp::buildFX()
{
	DWORD shaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    shaderFlags |= D3D10_SHADER_DEBUG;
	shaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#endif
  
	ID3D10Blob* compilationErrors = 0;
	HRESULT hr = 0;
	hr = D3DX10CreateEffectFromFile(L"texAlpha.fx", 0, 0, 
		"fx_4_0", shaderFlags, 0, md3dDevice, 0, 0, &mFX, &compilationErrors, 0);
	if(FAILED(hr))
	{
		if( compilationErrors )
		{
			MessageBoxA(0, (char*)compilationErrors->GetBufferPointer(), 0, 0);
			ReleaseCOM(compilationErrors);
		}
		DXTrace(__FILE__, (DWORD)__LINE__, hr, L"D3DX10CreateEffectFromFile", true);
	} 

	mTech = mFX->GetTechniqueByName("TexAlphaTech");
	
	mfxWVPVar        = mFX->GetVariableByName("gWVP")->AsMatrix();
	mfxWorldVar      = mFX->GetVariableByName("gWorld")->AsMatrix();
	mfxEyePosVar     = mFX->GetVariableByName("gEyePosW");
	mfxLightVar      = mFX->GetVariableByName("gLight");
	mfxDiffuseMapVar = mFX->GetVariableByName("gDiffuseMap")->AsShaderResource();
	mfxSpecMapVar    = mFX->GetVariableByName("gSpecMap")->AsShaderResource();
	mfxTexMtxVar     = mFX->GetVariableByName("gTexMtx")->AsMatrix();
}

void TransparentWaterApp::buildVertexLayouts()
{
	// Create the vertex input layout.
	D3D10_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D10_INPUT_PER_VERTEX_DATA, 0},
	};

	// Create the input layout
    D3D10_PASS_DESC PassDesc;
    mTech->GetPassByIndex(0)->GetDesc(&PassDesc);
    HR(md3dDevice->CreateInputLayout(vertexDesc, 3, PassDesc.pIAInputSignature,
		PassDesc.IAInputSignatureSize, &mVertexLayout));
}
