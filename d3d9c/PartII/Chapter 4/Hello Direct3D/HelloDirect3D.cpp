//=============================================================================
// HelloDirect3D.cpp by Frank Luna (C) 2005 All Rights Reserved.
//
// Demonstrates Direct3D Initialization and text output using the 
// framework code.
//=============================================================================

#include "d3dApp.h"
#include <tchar.h>
#include <crtdbg.h>

class HelloD3DApp : public D3DApp
{
public:
	HelloD3DApp(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP);
	~HelloD3DApp();

	bool checkDeviceCaps();
	void onLostDevice();
	void onResetDevice();
	void updateScene(float dt);
	void drawScene();

private:

	ID3DXFont* mFont;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
	#if defined(DEBUG) | defined(_DEBUG)
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	#endif


	HelloD3DApp app(hInstance, "Hello Direct3D", D3DDEVTYPE_HAL, D3DCREATE_HARDWARE_VERTEXPROCESSING);
	gd3dApp = &app;
	
	return gd3dApp->run();
}

HelloD3DApp::HelloD3DApp(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP)
: D3DApp(hInstance, winCaption, devType, requestedVP)
{
	srand(time_t(0));

	if(!checkDeviceCaps())
	{
		MessageBox(0, "checkDeviceCaps() Failed", 0, 0);
		PostQuitMessage(0);
	}

	D3DXFONT_DESC fontDesc;
	fontDesc.Height          = 80;
    fontDesc.Width           = 40;
    fontDesc.Weight          = FW_BOLD;
    fontDesc.MipLevels       = 0;
    fontDesc.Italic          = true;
    fontDesc.CharSet         = DEFAULT_CHARSET;
    fontDesc.OutputPrecision = OUT_DEFAULT_PRECIS;
    fontDesc.Quality         = DEFAULT_QUALITY;
    fontDesc.PitchAndFamily  = DEFAULT_PITCH | FF_DONTCARE;
    lstrcpy(fontDesc.FaceName, _T("Times New Roman"));

	HR(D3DXCreateFontIndirect(gd3dDevice, &fontDesc, &mFont));
}

HelloD3DApp::~HelloD3DApp()
{
	ReleaseCOM(mFont);
}

bool HelloD3DApp::checkDeviceCaps()
{
	// Nothing to check.
	return true;
}

void HelloD3DApp::onLostDevice()
{
	HR(mFont->OnLostDevice());
}

void HelloD3DApp::onResetDevice()
{
	HR(mFont->OnResetDevice());
}

void HelloD3DApp::updateScene(float dt)
{
}

void HelloD3DApp::drawScene()
{
	HR(gd3dDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(255, 255, 255), 1.0f, 0));

	RECT formatRect;
	GetClientRect(mhMainWnd, &formatRect);

	HR(gd3dDevice->BeginScene());

	mFont->DrawText(0, _T("Hello Direct3D"), -1, 
		&formatRect, DT_CENTER | DT_VCENTER, 
		D3DCOLOR_XRGB(rand() % 256, rand() % 256, rand() % 256));

	HR(gd3dDevice->EndScene());
	HR(gd3dDevice->Present(0, 0, 0, 0));
}
