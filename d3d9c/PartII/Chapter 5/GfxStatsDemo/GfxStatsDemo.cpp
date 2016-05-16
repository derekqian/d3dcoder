//=============================================================================
// GfxStatsDemo.cpp by Frank Luna (C) 2005 All Rights Reserved.
//
// Demonstrates using the performance timer, and the GfxStats class by
// displaying the frames per second and miliseconds per frame.
//=============================================================================

#include "d3dApp.h"
#include <tchar.h>
#include <crtdbg.h>
#include "GfxStats.h"

class GfxStatsDemo : public D3DApp
{
public:
	GfxStatsDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP);
	~GfxStatsDemo();

	bool checkDeviceCaps();
	void onLostDevice();
	void onResetDevice();
	void updateScene(float dt);
	void drawScene();

private:
	GfxStats* mGfxStats;
	
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
	#if defined(DEBUG) | defined(_DEBUG)
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	#endif


	GfxStatsDemo app(hInstance, "Gfx-Stats Demo", D3DDEVTYPE_HAL, D3DCREATE_HARDWARE_VERTEXPROCESSING);
	gd3dApp = &app;

	return gd3dApp->run();
}

GfxStatsDemo::GfxStatsDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP)
: D3DApp(hInstance, winCaption, devType, requestedVP)
{
	if(!checkDeviceCaps())
	{
		MessageBox(0, "checkDeviceCaps() Failed", 0, 0);
		PostQuitMessage(0);
	}

	mGfxStats = new GfxStats();
}

GfxStatsDemo::~GfxStatsDemo()
{
	delete mGfxStats;
}

bool GfxStatsDemo::checkDeviceCaps()
{
	// Nothing to check.
	return true;
}

void GfxStatsDemo::onLostDevice()
{
	mGfxStats->onLostDevice();
}

void GfxStatsDemo::onResetDevice()
{
	mGfxStats->onResetDevice();
}

void GfxStatsDemo::updateScene(float dt)
{
	mGfxStats->update(dt);
}

void GfxStatsDemo::drawScene()
{
	HR(gd3dDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0));
	HR(gd3dDevice->BeginScene());
	mGfxStats->display();
	HR(gd3dDevice->EndScene());
	HR(gd3dDevice->Present(0, 0, 0, 0));
}
