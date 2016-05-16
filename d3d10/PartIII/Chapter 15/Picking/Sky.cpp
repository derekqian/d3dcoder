#include "Sky.h"
#include "TextureMgr.h"
#include "Camera.h"
#include "Effects.h"
#include "InputLayouts.h"

struct SkyVertex
{
	D3DXVECTOR3 pos;
};
 
Sky::Sky()
: md3dDevice(0), mVB(0), mIB(0), mCubeMap(0)
{
	mNumIndices = 0;
}

Sky::~Sky()
{
	ReleaseCOM(mVB);
	ReleaseCOM(mIB);
}

void Sky::init(ID3D10Device* device, ID3D10ShaderResourceView* cubemap, float radius)
{
	md3dDevice = device;
	mCubeMap   = cubemap;

	mTech         = fx::SkyFX->GetTechniqueByName("SkyTech");
	mfxWVPVar     = fx::SkyFX->GetVariableByName("gWVP")->AsMatrix();
	mfxCubeMapVar = fx::SkyFX->GetVariableByName("gCubeMap")->AsShaderResource();


	std::vector<D3DXVECTOR3> vertices;
	std::vector<DWORD> indices;

	BuildGeoSphere(2, radius, vertices, indices);

	std::vector<SkyVertex> skyVerts(vertices.size());
	for(size_t i = 0; i < vertices.size(); ++i)
	{
		// Scale on y-axis to turn into an ellipsoid to make a flatter Sky surface
		skyVerts[i].pos = 0.5f*vertices[i];
	}

	D3D10_BUFFER_DESC vbd;
    vbd.Usage = D3D10_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(SkyVertex) * (UINT)skyVerts.size();
    vbd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D10_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = &skyVerts[0];
    HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mVB));

	mNumIndices = (UINT)indices.size();

	D3D10_BUFFER_DESC ibd;
    ibd.Usage = D3D10_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(DWORD) * mNumIndices;
    ibd.BindFlags = D3D10_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D10_SUBRESOURCE_DATA iinitData;
    iinitData.pSysMem = &indices[0];
    HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mIB));
}

void Sky::draw()
{
	D3DXVECTOR3 eyePos = GetCamera().position();

	// center Sky about eye in world space
	D3DXMATRIX W;
	D3DXMatrixTranslation(&W, eyePos.x, eyePos.y, eyePos.z);

	D3DXMATRIX V = GetCamera().view();
	D3DXMATRIX P = GetCamera().proj();

	D3DXMATRIX WVP = W*V*P;

	HR(mfxWVPVar->SetMatrix((float*)WVP));
	HR(mfxCubeMapVar->SetResource(mCubeMap));

	UINT stride = sizeof(SkyVertex);
    UINT offset = 0;
    md3dDevice->IASetVertexBuffers(0, 1, &mVB, &stride, &offset);
	md3dDevice->IASetIndexBuffer(mIB, DXGI_FORMAT_R32_UINT, 0);
	md3dDevice->IASetInputLayout(InputLayout::Pos);
	md3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	D3D10_TECHNIQUE_DESC techDesc;
    mTech->GetDesc( &techDesc );

    for(UINT p = 0; p < techDesc.Passes; ++p)
    {
        ID3D10EffectPass* pass = mTech->GetPassByIndex(p);

		pass->Apply(0);
		md3dDevice->DrawIndexed(mNumIndices, 0, 0);
	}
}
 