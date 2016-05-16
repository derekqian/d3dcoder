#ifndef EFFECTS_H
#define EFFECTS_H

#include "d3dUtil.h"

namespace fx
{
	extern ID3D10Effect* SkyFX;
	extern ID3D10Effect* CubeMapFX;
 
	void InitAll(ID3D10Device* device);
	void DestroyAll();
};

#endif // EFFECTS_H