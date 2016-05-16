//////////////////////////////////////////////////////////////////////////////////////////////////
// 
// File: pickSample.cpp
// 
// Author: Frank Luna (C) All Rights Reserved
//
// System: AMD Athlon 1800+ XP, 512 DDR, Geforce 3, Windows XP, MSVC++ 7.0 
//
// Desc: Demonstrates picking.
//          
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "d3dUtility.h"

//
// Globals
//

IDirect3DDevice9* Device = 0; 

const int Width  = 640;
const int Height = 480;

ID3DXMesh* Teapot = 0;
ID3DXMesh* Sphere = 0;

D3DXMATRIX World;
d3d::BoundingSphere BSphere;

//
// Functions
//
d3d::Ray CalcPickingRay(int x, int y)
{
	float px = 0.0f;
	float py = 0.0f;

	D3DVIEWPORT9 vp;
	Device->GetViewport(&vp);

	D3DXMATRIX proj;
	Device->GetTransform(D3DTS_PROJECTION, &proj);

	px = ((( 2.0f*x) / vp.Width)  - 1.0f) / proj(0, 0);
	py = (((-2.0f*y) / vp.Height) + 1.0f) / proj(1, 1);

	d3d::Ray ray;
	ray._origin    = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	ray._direction = D3DXVECTOR3(px, py, 1.0f);

	return ray;
}

void TransformRay(d3d::Ray* ray, D3DXMATRIX* T)
{
	// transform the ray's origin, w = 1.
	D3DXVec3TransformCoord(
		&ray->_origin,
		&ray->_origin,
		T);

	// transform the ray's direction, w = 0.
	D3DXVec3TransformNormal(
		&ray->_direction,
		&ray->_direction,
		T);

	// normalize the direction
	D3DXVec3Normalize(&ray->_direction, &ray->_direction);
}

bool RaySphereIntTest(d3d::Ray* ray, d3d::BoundingSphere* sphere)
{
	D3DXVECTOR3 v = ray->_origin - sphere->_center;

	float b = 2.0f * D3DXVec3Dot(&ray->_direction, &v);
	float c = D3DXVec3Dot(&v, &v) - (sphere->_radius * sphere->_radius);

	// find the discriminant
	float discriminant = (b * b) - (4.0f * c);

	// test for imaginary number
	if( discriminant < 0.0f )
		return false;

	discriminant = sqrtf(discriminant);

	float s0 = (-b + discriminant) / 2.0f;
	float s1 = (-b - discriminant) / 2.0f;

	// if a solution is >= 0, then we intersected the sphere
	if( s0 >= 0.0f || s1 >= 0.0f )
		return true;
	
	return false;
}

//
// Framework functions
//
bool Setup()
{
	//
	// Create the teapot.
	//

	D3DXCreateTeapot(Device, &Teapot, 0);

	//
	// Compute the bounding sphere.
	//

	BYTE* v = 0;
	Teapot->LockVertexBuffer(0, (void**)&v);

	D3DXComputeBoundingSphere(
		(D3DXVECTOR3*)v,
		Teapot->GetNumVertices(),
		D3DXGetFVFVertexSize(Teapot->GetFVF()),
		&BSphere._center,
		&BSphere._radius);

	Teapot->UnlockVertexBuffer();

	//
	// Build a sphere mesh that describes the teapot's bounding sphere.
	//

	D3DXCreateSphere(Device, BSphere._radius, 20, 20, &Sphere, 0);

	//
	// Set light.
	//

	D3DXVECTOR3 dir(0.707f, -0.0f, 0.707f);
	D3DXCOLOR col(1.0f, 1.0f, 1.0f, 1.0f);
	D3DLIGHT9 light = d3d::InitDirectionalLight(&dir, &col);

	Device->SetLight(0, &light);
	Device->LightEnable(0, true);
	Device->SetRenderState(D3DRS_NORMALIZENORMALS, true);
	Device->SetRenderState(D3DRS_SPECULARENABLE, false);	

	//
	// Set view matrix.
	//

	D3DXVECTOR3 pos(0.0f, 0.0f, -10.0f);
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);

	D3DXMATRIX V;
	D3DXMatrixLookAtLH(&V, &pos, &target, &up);
	Device->SetTransform(D3DTS_VIEW, &V);

	//
	// Set projection matrix.
	//

	D3DXMATRIX proj;
	D3DXMatrixPerspectiveFovLH(
			&proj,
			D3DX_PI * 0.25f, // 45 - degree
			(float)Width / (float)Height,
			1.0f,
			1000.0f);
	Device->SetTransform(D3DTS_PROJECTION, &proj);

	return true;
}

void Cleanup()
{
	d3d::Release<ID3DXMesh*>(Teapot);
	d3d::Release<ID3DXMesh*>(Sphere);
}

bool Display(float timeDelta)
{
	if( Device )
	{
		//
		// Update: Update Teapot.
		//
			
		static float r     = 0.0f;
		static float v     = 1.0f;
		static float angle = 0.0f;

		D3DXMatrixTranslation(&World, cosf(angle) * r, sinf(angle) * r, 10.0f);

		// transfrom the bounding sphere to match the teapots position in the
		// world.
		BSphere._center = D3DXVECTOR3(cosf(angle)*r, sinf(angle)*r, 10.0f);

		r += v * timeDelta;

		if( r >= 8.0f )
			v = -v; // reverse direction

		if( r <= 0.0f )
			v = -v; // reverse direction

		angle += 1.0f * D3DX_PI * timeDelta;
		if( angle >= D3DX_PI * 2.0f )
			angle = 0.0f;

		//
		// Render
		//

		Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0);
		Device->BeginScene();

		// Render the teapot.
		Device->SetTransform(D3DTS_WORLD, &World);
		Device->SetMaterial(&d3d::YELLOW_MTRL);
		Teapot->DrawSubset(0);

		// Render the bounding sphere with alpha blending so we can see 
		// through it.
		Device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
		Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

		D3DMATERIAL9 blue = d3d::BLUE_MTRL;
		blue.Diffuse.a = 0.25f; // 25% opacity
		Device->SetMaterial(&blue);
		Sphere->DrawSubset(0);

		Device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

		Device->EndScene();
		Device->Present(0, 0, 0, 0);
	}
	return true;
}

//
// WndProc
//
LRESULT CALLBACK d3d::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch( msg )
	{
	case WM_DESTROY:
		::PostQuitMessage(0);
		break;
		
	case WM_KEYDOWN:
		if( wParam == VK_ESCAPE )
			::DestroyWindow(hwnd);

		break;
	case WM_LBUTTONDOWN:

		// compute the ray in view space given the clicked screen point
		d3d::Ray ray = CalcPickingRay(LOWORD(lParam), HIWORD(lParam));

		// transform the ray to world space
		D3DXMATRIX view;
		Device->GetTransform(D3DTS_VIEW, &view);

		D3DXMATRIX viewInverse;
		D3DXMatrixInverse(&viewInverse,	0, &view);

		TransformRay(&ray, &viewInverse);

		// test for a hit
		if( RaySphereIntTest(&ray, &BSphere) )
			::MessageBox(0, "Hit!", "HIT", 0);

		break;
	}
	return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

//
// WinMain
//
int WINAPI WinMain(HINSTANCE hinstance,
				   HINSTANCE prevInstance, 
				   PSTR cmdLine,
				   int showCmd)
{
	if(!d3d::InitD3D(hinstance,
		Width, Height, true, D3DDEVTYPE_HAL, &Device))
	{
		::MessageBox(0, "InitD3D() - FAILED", 0, 0);
		return 0;
	}
		
	if(!Setup())
	{
		::MessageBox(0, "Setup() - FAILED", 0, 0);
		return 0;
	}

	d3d::EnterMsgLoop( Display );

	Cleanup();

	Device->Release();

	return 0;
}
