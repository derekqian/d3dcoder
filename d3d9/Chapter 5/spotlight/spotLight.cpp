//////////////////////////////////////////////////////////////////////////////////////////////////
// 
// File: spotLight.cpp
// 
// Author: Frank Luna (C) All Rights Reserved
//
// System: AMD Athlon 1800+ XP, 512 DDR, Geforce 3, Windows XP, MSVC++ 7.0 
//
// Desc: Demonstrates using a spotlight with D3DX objects.  You can move
//       the spotlight around the scene with the arrow keys.
//          
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "d3dUtility.h"

//
// Globals
//

IDirect3DDevice9*     Device = 0; 

const int Width  = 640;
const int Height = 480;

ID3DXMesh* Objects[4] = {0, 0, 0, 0};
D3DXMATRIX  Worlds[4];
D3DMATERIAL9 Mtrls[4];

D3DLIGHT9 Spot;

//
// Framework Functions
//
bool Setup()
{
	//
	// Create objects.
	//

	D3DXCreateTeapot(Device, &Objects[0], 0);
	D3DXCreateSphere(Device, 1.0f, 20, 20, &Objects[1], 0);
	D3DXCreateTorus(Device, 0.5f, 1.0f, 20, 20, &Objects[2], 0);
	D3DXCreateCylinder(Device, 0.5f, 0.5f, 2.0f, 20, 20, &Objects[3], 0);

	//
	// Build world matrices - position the objects in world space.
	//

	D3DXMatrixTranslation(&Worlds[0],  0.0f,  2.0f, 0.0f);
	D3DXMatrixTranslation(&Worlds[1],  0.0f, -2.0f, 0.0f);
	D3DXMatrixTranslation(&Worlds[2], -3.0f,  0.0f, 0.0f);
	D3DXMatrixTranslation(&Worlds[3],  3.0f,  0.0f, 0.0f);
	D3DXMATRIX Rx;
	D3DXMatrixRotationX(&Rx, D3DX_PI * 0.5f);
	Worlds[3] = Rx * Worlds[3];

	//
	// Setup the object's materials.
	//

	Mtrls[0] = d3d::RED_MTRL;
	Mtrls[1] = d3d::BLUE_MTRL;
	Mtrls[2] = d3d::GREEN_MTRL;
	Mtrls[3] = d3d::YELLOW_MTRL;

	for(int i = 0; i < 4; i++)
		Mtrls[i].Power = 20.0f;

	//
	// Setup a spot light
	//

	D3DXVECTOR3 pos(0.0f, 0.0f, -5.0f);
	D3DXVECTOR3 dir(0.0f, 0.0f,  1.0f);
	D3DXCOLOR   c = d3d::WHITE;
	Spot = d3d::InitSpotLight(&pos, &dir, &c);

	//
	// Set and Enable spotlight.
	//

	Device->SetLight(0, &Spot);
	Device->LightEnable(0, true);

	//
	// Set light related render states.
	//

	Device->SetRenderState(D3DRS_NORMALIZENORMALS, true);
	Device->SetRenderState(D3DRS_SPECULARENABLE, true);

	//
	// Position and aim the camera.
	//
	D3DXVECTOR3 position( 0.0f, 0.0f, -5.0f );
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
	D3DXMATRIX V;
	D3DXMatrixLookAtLH(&V, &position, &target, &up);
	Device->SetTransform(D3DTS_VIEW, &V);

	//
	// Set the projection matrix.
	//

	D3DXMATRIX proj;
	D3DXMatrixPerspectiveFovLH(
			&proj,
			D3DX_PI * 0.5f, // 90 - degree
			(float)Width / (float)Height,
			1.0f,
			1000.0f);
	Device->SetTransform(D3DTS_PROJECTION, &proj);

	return true;
}

void Cleanup()
{
	for(int i = 0; i < 4; i++)
		d3d::Release<ID3DXMesh*>(Objects[i]);
}

bool Display(float timeDelta)
{
	if( Device )
	{
		//
		// Move spot light around based on keyboard input
		//

		static float angle = (3.0f * D3DX_PI) / 2.0f;
		
		if( ::GetAsyncKeyState(VK_LEFT) & 0x8000f )
			Spot.Direction.x -= 0.5f * timeDelta;

		if( ::GetAsyncKeyState(VK_RIGHT) & 0x8000f )
			Spot.Direction.x += 0.5f * timeDelta;

		if( ::GetAsyncKeyState(VK_DOWN) & 0x8000f )
			Spot.Direction.y -= 0.5f * timeDelta;

		if( ::GetAsyncKeyState(VK_UP) & 0x8000f )
			Spot.Direction.y += 0.5f * timeDelta;

		// update the light 
		Device->SetLight(0, &Spot);
		Device->LightEnable(0, true);

		//
		// Draw the scene:
		//
		Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0);
		Device->BeginScene();

		for(int i = 0; i < 4; i++)
		{
			// set material and world matrix for ith object, then render
			// the ith object.
			Device->SetMaterial(&Mtrls[i]);
			Device->SetTransform(D3DTS_WORLD, &Worlds[i]);
			Objects[i]->DrawSubset(0);
		}

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