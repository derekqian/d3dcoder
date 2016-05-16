//////////////////////////////////////////////////////////////////////////////////////////////////
// 
// File: fps.h
// 
// Author: Frank Luna (C) All Rights Reserved
//
// System: AMD Athlon 1800+ XP, 512 DDR, Geforce 3, Windows XP, MSVC++ 7.0 
//
// Desc: Wraps the code to compute and display the frames rendered per second.
//          
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __fpsH__
#define __fpsH__

#include "d3dfont.h"

class FPSCounter
{
public:
	FPSCounter(IDirect3DDevice9* device);
	~FPSCounter();

	bool render(D3DCOLOR color, float timeDelta);
private:
	IDirect3DDevice9* _device;
	
	CD3DFont* _font;
	DWORD     _frameCnt;
	float     _timeElapsed;
	float     _fps;
	char      _fpsString[9];
	
};
#endif // __fpsH__