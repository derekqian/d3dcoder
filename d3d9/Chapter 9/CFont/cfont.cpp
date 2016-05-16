//////////////////////////////////////////////////////////////////////////////////////////////////
// 
// File: cfont.cpp
// 
// Author: Frank Luna (C) All Rights Reserved
//
// System: AMD Athlon 1800+ XP, 512 DDR, Geforce 3, Windows XP, MSVC++ 7.0 
//
// Desc: Shows how to calculate the number of frames rendered per second and
//       demonstrates how to render text with the CD3DFont class.
//          
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "d3dUtility.h"
#include "d3dfont.h"

//
// Globals
//

IDirect3DDevice9* Device = 0; 

const int Width  = 640;
const int Height = 480;

CD3DFont* Font    = 0;
DWORD FrameCnt    = 0;
float TimeElapsed = 0;
float FPS         = 0;
char FPSString[9];

//
// Framework functions
//
bool Setup()
{
	//
	// Create a font object and initialize it.
	//

	Font = new CD3DFont("Times New Roman", 16, 0);
	Font->InitDeviceObjects( Device );
	Font->RestoreDeviceObjects();

	return true;
}

void Cleanup()
{
	if( Font )
	{
		Font->InvalidateDeviceObjects();
		Font->DeleteDeviceObjects();
		d3d::Delete<CD3DFont*>(Font);
	}
}

bool Display(float timeDelta)
{
	if( Device )
	{
		//
		// Update: Compute the frames per second.
		//

		FrameCnt++;

		TimeElapsed += timeDelta;

		if(TimeElapsed >= 1.0f)
		{
			FPS = (float)FrameCnt / TimeElapsed;

			
			sprintf(FPSString, "%f", FPS);
			FPSString[8] = '\0'; // mark end of string

			TimeElapsed = 0.0f;
			FrameCnt    = 0;
		}

		//
		// Render
		//

		Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0);
		Device->BeginScene();

		if( Font )
			Font->DrawText(20, 20, 0xff000000, FPSString);	

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

