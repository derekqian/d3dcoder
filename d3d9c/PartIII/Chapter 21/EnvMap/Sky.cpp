//=============================================================================
// Sky.cpp by Frank Luna (C) 2004 All Rights Reserved.
//=============================================================================

#include "Sky.h"
#include "Camera.h"
#include "d3dUtil.h"

Sky::Sky(const std::string& envmapFilename, float skyRadius)
: mRadius(skyRadius)
{
	HR(D3DXCreateSphere(gd3dDevice, skyRadius, 30, 30, &mSphere, 0));
	HR(D3DXCreateCubeTextureFromFile(gd3dDevice, envmapFilename.c_str(), &mEnvMap));

	ID3DXBuffer* errors = 0;
	HR(D3DXCreateEffectFromFile(gd3dDevice, "sky.fx", 0, 0, 0, 
		0, &mFX, &errors));
	if( errors )
		MessageBox(0, (char*)errors->GetBufferPointer(), 0, 0);

	mhTech   = mFX->GetTechniqueByName("SkyTech");
	mhWVP    = mFX->GetParameterByName(0, "gWVP");
	mhEnvMap = mFX->GetParameterByName(0, "gEnvMap");

	// Set effect parameters that do not vary.
	HR(mFX->SetTechnique(mhTech));
	HR(mFX->SetTexture(mhEnvMap, mEnvMap));
}

Sky::~Sky()
{
	ReleaseCOM(mSphere);
	ReleaseCOM(mEnvMap);
	ReleaseCOM(mFX);
}

DWORD Sky::getNumTriangles()
{
	return mSphere->GetNumFaces();
}

DWORD Sky::getNumVertices()
{
	return mSphere->GetNumVertices();
}

IDirect3DCubeTexture9* Sky::getEnvMap()
{
	return mEnvMap;
}

float Sky::getRadius()
{
	return mRadius;
}

void Sky::onLostDevice()
{
	HR(mFX->OnLostDevice());
}

void Sky::onResetDevice()
{
	HR(mFX->OnResetDevice());
}

void Sky::draw()
{
	// Sky always centered about camera's position.
	D3DXMATRIX W;
	D3DXVECTOR3 p = gCamera->pos();
	D3DXMatrixTranslation(&W, p.x, p.y, p.z);
	HR(mFX->SetMatrix(mhWVP, &(W*gCamera->viewProj())));
	
	UINT numPasses = 0;
	HR(mFX->Begin(&numPasses, 0));
	HR(mFX->BeginPass(0));
	HR(mSphere->DrawSubset(0));
	HR(mFX->EndPass());
	HR(mFX->End());
}