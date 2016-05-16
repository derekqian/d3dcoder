//////////////////////////////////////////////////////////////////////////////////////////////////
// 
// File: cube.h
// 
// Author: Frank Luna (C) All Rights Reserved
//
// System: AMD Athlon 1800+ XP, 512 DDR, Geforce 3, Windows XP, MSVC++ 7.0 
//
// Desc: Provides an interface to create and render a cube.
//          
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __cubeH__
#define __cubeH__

#include <d3dx9.h>
#include <string>

class Cube
{
public:
	Cube(IDirect3DDevice9* device);
	~Cube();

	bool draw(D3DXMATRIX* world, D3DMATERIAL9* mtrl, IDirect3DTexture9* tex);
private:
	IDirect3DDevice9*       _device;
	IDirect3DVertexBuffer9* _vb;
	IDirect3DIndexBuffer9*  _ib;
};
#endif //__cubeH__