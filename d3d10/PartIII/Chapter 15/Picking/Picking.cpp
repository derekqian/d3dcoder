//=============================================================================
// Picking.cpp by Frank Luna (C) 2008 All Rights Reserved.
//
// Demonstrates picking.  In this demo, the user can only pick triangles
// on the tree mesh.  However, it is simple to extend picking to all mesh
// objects by looping through them.
//
// Controls:
//		'A'/'D'/'W'/'S' - Move
//      Hold down left mouse button and move mouse to rotate.
//      Click the right mouse button to pick a triangle.
//
//=============================================================================

#include "d3dApp.h"
#include "Light.h"
#include "Camera.h"
#include "Effects.h"
#include "InputLayouts.h"
#include "TextureMgr.h"
#include "Sky.h"
#include "DrawableTex2D.h"
#include "Mesh.h"
 
class PickingApp : public D3DApp
{
public:
	PickingApp(HINSTANCE hInstance);
	~PickingApp();

	void initApp();
	void onResize();
	void updateScene(float dt);
	void drawScene(); 

	LRESULT msgProc(UINT msg, WPARAM wParam, LPARAM lParam);
 
	void pick(int sx, int sy);
private:

	ID3D10Buffer* mPickedTriVB;

	UINT mPickedTriangle;

	POINT mOldMousePos;

	Mesh mBaseMesh;
	Mesh mTreeMesh;
	Mesh mPillar06Mesh;
	Sky mSky;

	DrawableTex2D mShadowMap;

	Light mParallelLight;

	D3DXMATRIX mBaseWorld;
	D3DXMATRIX mTreeWorld;
	D3DXMATRIX mPillar06World1;
	D3DXMATRIX mPillar06World2;
	D3DXMATRIX mLightView;
	D3DXMATRIX mLightVolume;

	ID3D10RasterizerState* mNoCullRS;

	ID3D10ShaderResourceView* mEnvMapRV;

	ID3D10EffectTechnique* mBuildShadowMapTech;
	ID3D10EffectMatrixVariable* mfxBuildShadowMapLightWVPVar;
	ID3D10EffectShaderResourceVariable* mfxBuildShadowMapDiffuseMapVar;

	ID3D10EffectTechnique* mPickedTech;
	ID3D10EffectMatrixVariable* mfxPickedWVPVar;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif


	PickingApp theApp(hInstance);
	
	theApp.initApp();

	return theApp.run();
}

PickingApp::PickingApp(HINSTANCE hInstance)
: D3DApp(hInstance), mEnvMapRV(0), mNoCullRS(0)
{
	mPickedTriangle = -1;

	D3DXMatrixIdentity(&mBaseWorld);
	D3DXMatrixTranslation(&mTreeWorld, 0.0f, 1.38f, 0.0f);

	D3DXMATRIX T, R;
	D3DXMatrixTranslation(&T, -2.0f, 1.3f, -1.0f);
	D3DXMatrixRotationY(&R, 2.0f);
	mPillar06World1 = R*T;

	D3DXMatrixTranslation(&T, 4.0f, 1.3f, 1.0f);
	D3DXMatrixRotationY(&R, 1.0f);
	mPillar06World2 = R*T;
}

PickingApp::~PickingApp()
{
	if( md3dDevice )
		md3dDevice->ClearState();

	fx::DestroyAll();
	InputLayout::DestroyAll();

	ReleaseCOM(mPickedTriVB);
}

void PickingApp::initApp()
{
	D3DApp::initApp();

	fx::InitAll(md3dDevice);
	InputLayout::InitAll(md3dDevice);
	GetTextureMgr().init(md3dDevice);

	D3D10_RASTERIZER_DESC rsDesc;
	ZeroMemory(&rsDesc, sizeof(D3D10_RASTERIZER_DESC));
    rsDesc.FillMode = D3D10_FILL_SOLID;
    rsDesc.CullMode = D3D10_CULL_NONE;
    rsDesc.FrontCounterClockwise = false;

	HR(md3dDevice->CreateRasterizerState(&rsDesc, &mNoCullRS));

	// Create the VB to store the picked triangle.  We make this a dynamic VB
	// since the picked triangle may change frequently.

	D3D10_BUFFER_DESC vbd;
    vbd.Usage = D3D10_USAGE_DYNAMIC;
    vbd.ByteWidth = sizeof(D3DXVECTOR3) * 3;
    vbd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
    vbd.MiscFlags = 0;
    HR(md3dDevice->CreateBuffer(&vbd, 0, &mPickedTriVB));

 
	mBuildShadowMapTech            = fx::BuildShadowMapFX->GetTechniqueByName("BuildShadowMapTech");
	mfxBuildShadowMapLightWVPVar   = fx::BuildShadowMapFX->GetVariableByName("gLightWVP")->AsMatrix();
	mfxBuildShadowMapDiffuseMapVar = fx::BuildShadowMapFX->GetVariableByName("gDiffuseMap")->AsShaderResource();

	mPickedTech     = fx::PickedFX->GetTechniqueByName("PickedTech");
	mfxPickedWVPVar = fx::PickedFX->GetVariableByName("gWVP")->AsMatrix();

	mShadowMap.init(md3dDevice, 1024, 1024, false, DXGI_FORMAT_UNKNOWN);
 
	mClearColor = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);

	GetCamera().position() = D3DXVECTOR3(0.0f, 1.8f, -10.0f);
 
	mEnvMapRV = GetTextureMgr().createCubeTex(L"grassenvmap1024.dds");
	mSky.init(md3dDevice, mEnvMapRV, 5000.0f);

	mBaseMesh.init(md3dDevice, L"templeBaseTree.m3d");
	mBaseMesh.setCubeMap(mEnvMapRV);

	mTreeMesh.init(md3dDevice, L"tree.m3d");
	mTreeMesh.setCubeMap(mEnvMapRV);

	mPillar06Mesh.init(md3dDevice, L"Pillar06.m3d");
	mPillar06Mesh.setCubeMap(mEnvMapRV);
 
	// direction updated at runtime
	mParallelLight.ambient  = D3DXCOLOR(0.3f, 0.3f, 0.3f, 1.0f);
	mParallelLight.diffuse  = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	mParallelLight.specular = D3DXCOLOR(0.5f, 0.4843f, 0.3f, 1.0f);

 
	D3DXMatrixOrthoLH(&mLightVolume, 20.0f, 20.0f, 1.0f, 100.0f);
}

void PickingApp::onResize()
{
	D3DApp::onResize();

	float aspect = (float)mClientWidth/mClientHeight;
	GetCamera().setLens(0.25f*PI, aspect, 0.5f, 1000.0f);
}

void PickingApp::updateScene(float dt)
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
	lightPos.x = 30.0f*cosf(0.1f*mTimer.getGameTime());
	lightPos.y = 20.0f;
	lightPos.z = 30.0f*sinf(0.1f*mTimer.getGameTime());

	D3DXMatrixLookAtLH(&mLightView, &lightPos,
		&D3DXVECTOR3(0.0f, 0.0f, 0.0f), &D3DXVECTOR3(0.0f, 1.0f, 0.0f));

	D3DXVECTOR3 lightDirection = -lightPos;
	D3DXVec3Normalize(&mParallelLight.dir, &lightDirection);
}

void PickingApp::drawScene()
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

	D3D10_TECHNIQUE_DESC techDesc;
    mBuildShadowMapTech->GetDesc( &techDesc );

    for(UINT i = 0; i < techDesc.Passes; ++i)
    {
        ID3D10EffectPass* pass = mBuildShadowMapTech->GetPassByIndex(i);

		D3DXMATRIX WVP;

		// Don't back face cull for tree, since we want to see both sides of leaf triangles.
		md3dDevice->RSSetState(mNoCullRS);
		WVP = mTreeWorld*mLightView*mLightVolume;
		mfxBuildShadowMapLightWVPVar->SetMatrix((float*)&WVP);
		mTreeMesh.drawToShadowMap(mfxBuildShadowMapDiffuseMapVar, pass);
		md3dDevice->RSSetState(0); // restore default
		
		WVP = mPillar06World1*mLightView*mLightVolume;
		mfxBuildShadowMapLightWVPVar->SetMatrix((float*)&WVP);
		mPillar06Mesh.drawToShadowMap(mfxBuildShadowMapDiffuseMapVar, pass);

		WVP = mPillar06World2*mLightView*mLightVolume;
		mfxBuildShadowMapLightWVPVar->SetMatrix((float*)&WVP);
		mPillar06Mesh.drawToShadowMap(mfxBuildShadowMapDiffuseMapVar, pass);

		WVP = mBaseWorld*mLightView*mLightVolume;
		mfxBuildShadowMapLightWVPVar->SetMatrix((float*)&WVP);
		mBaseMesh.drawToShadowMap(mfxBuildShadowMapDiffuseMapVar, pass);
    }

	mShadowMap.end();

	// restore rendering to backbuffer
	resetOMTargetsAndViewport();

	 
	//
	// Draw the rest of the scene.
	//

	// Set per frame constants.
	mBaseMesh.setLight(mParallelLight);
	mBaseMesh.setEyePos(GetCamera().position());
	mBaseMesh.enableCubeMap(false);
	mBaseMesh.setShadowMap(mShadowMap.depthMap());

	// Don't back face cull for tree, since we want to see both sides of leaf triangles.
	md3dDevice->RSSetState(mNoCullRS);
	mTreeMesh.draw(mTreeWorld, mLightView*mLightVolume);
	md3dDevice->RSSetState(0); // restore default
 
	mPillar06Mesh.draw(mPillar06World1, mLightView*mLightVolume);
	mPillar06Mesh.draw(mPillar06World2, mLightView*mLightVolume);

	mBaseMesh.draw(mBaseWorld, mLightView*mLightVolume);

    
	mBaseMesh.unbindShadowMap();


	// Draw sky last to save fill rate.
	mSky.draw();


	if( mPickedTriangle != -1 )
	{
		ID3DX10Mesh* d3dxmesh = mTreeMesh.d3dxMesh();

		ID3DX10MeshBuffer* vb = 0;
		ID3DX10MeshBuffer* ib = 0;
		HR(d3dxmesh->GetVertexBuffer(0, &vb));
		HR(d3dxmesh->GetIndexBuffer(&ib));

		// Get the indices of the picked triangle.

		DWORD* indices = 0;
		SIZE_T size;
		HR(ib->Map((void**)&indices, &size));

		DWORD i0 = indices[mPickedTriangle*3+0];
		DWORD i1 = indices[mPickedTriangle*3+1];
		DWORD i2 = indices[mPickedTriangle*3+2];

		HR(ib->Unmap());

		// Get the vertices of the picked triangle and copy them into our VB.

		MeshVertex* vertices = 0;
		HR(vb->Map((void**)&vertices, &size));

		D3DXVECTOR3* triVerts = 0;
		HR(mPickedTriVB->Map(D3D10_MAP_WRITE_DISCARD, 0, (void**)&triVerts));

		triVerts[0] = vertices[i0].pos;
		triVerts[1] = vertices[i1].pos;
		triVerts[2] = vertices[i2].pos;

		mPickedTriVB->Unmap();
		HR(vb->Unmap());

		ReleaseCOM(vb);
		ReleaseCOM(ib);

		// Render the picked triangle specially to distinguish it
		// from the other triangles.

		md3dDevice->IASetInputLayout(InputLayout::Pos);
		md3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		UINT stride = sizeof(D3DXVECTOR3);
		UINT offset = 0;
		md3dDevice->IASetVertexBuffers(0, 1, &mPickedTriVB, &stride, &offset);

		D3D10_TECHNIQUE_DESC techDesc;
		mPickedTech->GetDesc( &techDesc );

		D3DXMATRIX W = mTreeWorld;
		D3DXMATRIX V = GetCamera().view();
		D3DXMATRIX P = GetCamera().proj();

		D3DXMATRIX WVP = W*V*P;

		mfxPickedWVPVar->SetMatrix((float*)&WVP);

		for(UINT i = 0; i < techDesc.Passes; ++i)
		{
			ID3D10EffectPass* pass = mPickedTech->GetPassByIndex(i);
			pass->Apply(0);

			md3dDevice->Draw(3, 0);
		}
	}


	// We specify DT_NOCLIP, so we do not care about width/height of the rect.
	RECT R = {5, 5, 0, 0};
	md3dDevice->RSSetState(0);
	mFont->DrawText(0, mFrameStats.c_str(), -1, &R, DT_NOCLIP, WHITE);

	mSwapChain->Present(0, 0);
}

LRESULT PickingApp::msgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	POINT mousePos;
	int dx = 0;
	int dy = 0;
	switch(msg)
	{
	case WM_RBUTTONDOWN:
		pick(LOWORD(lParam), HIWORD(lParam));
		return 0;

	case WM_RBUTTONUP:
		mPickedTriangle = -1;
		return 0;
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
 
void PickingApp::pick(int sx, int sy)
{
	D3DXMATRIX P = GetCamera().proj();

	// Compute picking ray in view space.
	float vx = (+2.0f*sx/mClientWidth  - 1.0f)/P(0,0);
	float vy = (-2.0f*sy/mClientHeight + 1.0f)/P(1,1);

	D3DXVECTOR3 rayOrigin(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 rayDir(vx, vy, 1.0f);

	// Tranform to world space.
	D3DXMATRIX V = GetCamera().view();

	D3DXMATRIX inverseV;
	D3DXMatrixInverse(&inverseV, 0, &V);

	D3DXVec3TransformCoord(&rayOrigin, &rayOrigin, &inverseV);
	D3DXVec3TransformNormal(&rayDir, &rayDir, &inverseV);

	// Transform to the mesh's local space.
	D3DXMATRIX inverseW;
	D3DXMatrixInverse(&inverseW, 0, &mTreeWorld);

	D3DXVec3TransformCoord(&rayOrigin, &rayOrigin, &inverseW);
	D3DXVec3TransformNormal(&rayDir, &rayDir, &inverseW);

	ID3DX10Mesh* d3dxmesh = mTreeMesh.d3dxMesh();

	UINT hitCount;
	float u,v,t;
	ID3D10Blob* allHits;
	d3dxmesh->Intersect(&rayOrigin, &rayDir, &hitCount, 
		&mPickedTriangle, &u, &v, &t, &allHits);

	ReleaseCOM(allHits);
}