#include "InputLayouts.h"
#include "Effects.h"

ID3D10InputLayout* InputLayout::Pos                 = 0;
ID3D10InputLayout* InputLayout::PosTangentNormalTex = 0;

void InputLayout::InitAll(ID3D10Device* device)
{
	D3D10_PASS_DESC PassDesc;

	//
	// Position vertex
	//
 	D3D10_INPUT_ELEMENT_DESC skyVertexDesc[] =
	{
		{"POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D10_INPUT_PER_VERTEX_DATA, 0}
	};

	fx::SkyFX->GetTechniqueByName("SkyTech")->GetPassByIndex(0)->GetDesc(&PassDesc);
    HR(device->CreateInputLayout(skyVertexDesc, 1, PassDesc.pIAInputSignature,
		PassDesc.IAInputSignatureSize, &Pos));

	// 
	// Position-tangent-normal-texture vertex
	//
	D3D10_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0},
		{"TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D10_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 36, D3D10_INPUT_PER_VERTEX_DATA, 0},
	};

    fx::NormalMapFX->GetTechniqueByName("NormalMapTech")->GetPassByIndex(0)->GetDesc(&PassDesc);
    HR(device->CreateInputLayout(vertexDesc, 4, PassDesc.pIAInputSignature,
		PassDesc.IAInputSignatureSize, &PosTangentNormalTex));
}

void InputLayout::DestroyAll()
{
	ReleaseCOM(Pos);
	ReleaseCOM(PosTangentNormalTex);
}