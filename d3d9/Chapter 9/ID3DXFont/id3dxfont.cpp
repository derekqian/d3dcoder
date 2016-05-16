//////////////////////////////////////////////////////////////////////////////////////////////////
// 
// File: id3dxfont.cpp
// 
// Author: Frank Luna (C) All Rights Reserved
//
// System: AMD Athlon 1800+ XP, 512 DDR, Geforce 3, Windows XP, MSVC++ 7.0 
//
// Desc: Shows how to calculate the number of frames rendered per second and
//       demonstrates how to render text with the ID3DXFont interface.
//          
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "d3dUtility.h"

//
// Globals
//

IDirect3DDevice9* Device = 0; 

const int Width  = 640;
const int Height = 480;

ID3DXFont* Font   = 0;
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
	// Initialize a LOGFONT structure that describes the font
	// we want to create.
	//

	LOGFONT lf;
	ZeroMemory(&lf, sizeof(LOGFONT));

	lf.lfHeight         = 25;    // in logical units
	lf.lfWidth          = 12;    // in logical units
	lf.lfEscapement     = 0;        
	lf.lfOrientation    = 0;     
	lf.lfWeight         = 500;   // boldness, range 0(light) - 1000(bold)
	lf.lfItalic         = false;   
	lf.lfUnderline      = false;    
	lf.lfStrikeOut      = false;    
	lf.lfCharSet        = DEFAULT_CHARSET;
	lf.lfOutPrecision   = 0;              
	lf.lfClipPrecision  = 0;          
	lf.lfQuality        = 0;           
	lf.lfPitchAndFamily = 0;           
	strcpy(lf.lfFaceName, "Times New Roman"); // font style

	//
	// Create an ID3DXFont based on 'lf'.
	//

	if(FAILED(D3DXCreateFontIndirect(Device, &lf, &Font)))
	{
		::MessageBox(0, "D3DXCreateFontIndirect() - FAILED", 0, 0);
		::PostQuitMessage(0);
	}

	return true;
}

void Cleanup()
{
	d3d::Release<ID3DXFont*>(Font);
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

		RECT rect = {0, 0, Width, Height};
		Font->DrawText(
			FPSString, 
			-1, // size of string or -1 indicates null terminating string
			&rect,            // rectangle text is to be formatted to in windows coords
			DT_TOP | DT_LEFT, // draw in the top left corner of the viewport
			0xff000000);      // black text

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

