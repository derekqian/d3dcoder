#ifndef INPUTLAYOUTS_H
#define INPUTLAYOUTS_H

#include "d3dUtil.h"

namespace InputLayout
{
	extern ID3D10InputLayout* Pos;
	extern ID3D10InputLayout* PosTex;
	extern ID3D10InputLayout* PosTangentNormalTex;

	void InitAll(ID3D10Device* device);
	void DestroyAll();
};

#endif // INPUTLAYOUTS_H