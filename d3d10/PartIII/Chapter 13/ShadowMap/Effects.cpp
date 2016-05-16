#include "Effects.h"

ID3D10Effect* fx::SkyFX            = 0;
ID3D10Effect* fx::ShadowFX         = 0;
ID3D10Effect* fx::BuildShadowMapFX = 0;
ID3D10Effect* fx::DrawShadowMapFX  = 0;

ID3D10Effect* CreateFX(ID3D10Device* device, std::wstring filename)
{
	DWORD shaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    shaderFlags |= D3D10_SHADER_DEBUG;
	shaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#endif
  
	ID3D10Blob* compilationErrors = 0;
	HRESULT hr = 0;
	ID3D10Effect* fx = 0;
	hr = D3DX10CreateEffectFromFile(filename.c_str(), 0, 0, 
		"fx_4_0", shaderFlags, 0, device, 0, 0, &fx, &compilationErrors, 0);
	if(FAILED(hr))
	{
		if( compilationErrors )
		{
			MessageBoxA(0, (char*)compilationErrors->GetBufferPointer(), 0, 0);
			ReleaseCOM(compilationErrors);
		}
		DXTrace(__FILE__, (DWORD)__LINE__, hr, filename.c_str(), true);
	}

	return fx;
}

void fx::InitAll(ID3D10Device* device)
{
	SkyFX            = CreateFX(device, L"sky.fx");
	ShadowFX         = CreateFX(device, L"shadow.fx");
	BuildShadowMapFX = CreateFX(device, L"buildshadowmap.fx");
	DrawShadowMapFX  = CreateFX(device, L"drawshadowmap.fx");
}

void fx::DestroyAll()
{
	ReleaseCOM(SkyFX);
	ReleaseCOM(ShadowFX);
	ReleaseCOM(BuildShadowMapFX);
	ReleaseCOM(DrawShadowMapFX);
}