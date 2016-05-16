#include "Mesh.h"
#include "TextureMgr.h"
#include "Camera.h"
#include "Effects.h"
#include "InputLayouts.h"
#include <fstream>
#include <istream>

using namespace std;
 
wistream& operator>>(wistream& is, D3DXVECTOR3& v)
{
	is >> v.x >> v.y >> v.z;
	return is;
}

wistream& operator>>(wistream& is, D3DXVECTOR2& v)
{
	is >> v.x >> v.y;
	return is;
}

Mesh::Mesh()
: md3dDevice(0), mMeshData(0), mNumSubsets(0)
{
}

Mesh::~Mesh()
{
	ReleaseCOM(mMeshData);
}

ID3DX10Mesh* Mesh::d3dxMesh()
{
	return mMeshData;
}

void Mesh::getAABB(D3DXVECTOR3& vMin, D3DXVECTOR3& vMax)
{
	ID3DX10MeshBuffer* vb = 0;
	HR(mMeshData->GetVertexBuffer(0, &vb));
	
	MeshVertex* vertices = 0;
	SIZE_T size;
	HR(vb->Map((void**)&vertices, &size));

	vMin = D3DXVECTOR3(+INFINITY, +INFINITY, +INFINITY);
	vMax = D3DXVECTOR3(-INFINITY, -INFINITY, -INFINITY);

	for(UINT i = 0; i < mMeshData->GetVertexCount(); ++i)
	{
		D3DXVec3Minimize(&vMin, &vMin, &vertices[i].pos);
		D3DXVec3Maximize(&vMax, &vMax, &vertices[i].pos);
	}
	
	HR(vb->Unmap());

	ReleaseCOM(vb);
}

void Mesh::init(ID3D10Device* device, std::wstring filename)
{
	md3dDevice = device;

	getFXHandles();

	wifstream fin(filename.c_str());
 
	UINT numVertices  = 0;
	UINT numTriangles = 0;
	wstring ignore;

	if( fin )
	{
		fin >> ignore; // file header text
		fin >> ignore; // #Subsets
		fin >> mNumSubsets;
		fin >> ignore; // #Vertices
		fin >> numVertices;
		fin >> ignore; // #Triangles
		fin >> numTriangles;

		D3D10_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0},
			{"TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D10_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 36, D3D10_INPUT_PER_VERTEX_DATA, 0},
		};
		HR(D3DX10CreateMesh(device, vertexDesc, 4, 
			vertexDesc[0].SemanticName, numVertices, 
			numTriangles, D3DX10_MESH_32_BIT, &mMeshData));

		fin >> ignore; // subsets header text
		for(UINT i = 0; i < mNumSubsets; ++i)
		{
			std::wstring diffuseMapFilename;
			std::wstring specMapFilename;
			std::wstring normalMapFilename;

			fin >> diffuseMapFilename;
			fin >> specMapFilename;
			fin >> normalMapFilename;
			
			D3DXVECTOR3 reflectivity;
			fin >> ignore; 
			fin >> reflectivity;

			mReflectMtrls.push_back(reflectivity);

			mDiffuseTextures.push_back(GetTextureMgr().createTex(diffuseMapFilename));
			mSpecTextures.push_back(GetTextureMgr().createTex(specMapFilename));
			mNormalTextures.push_back(GetTextureMgr().createTex(normalMapFilename));
		}

		MeshVertex* verts = new MeshVertex[numVertices];
		fin >> ignore; // vertices header text
		for(UINT i = 0; i < numVertices; ++i)
		{
			fin >> ignore; // Position:
			fin >> verts[i].pos;

			fin >> ignore; // Tangent:
			fin >> verts[i].tangent;

			fin >> ignore; // Normal:
			fin >> verts[i].normal;

			fin >> ignore; // Tex-Coords:
			fin >> verts[i].texC;
		}
		HR(mMeshData->SetVertexData(0, verts));

		delete[] verts;

		DWORD* indices = new DWORD[numTriangles*3];
		UINT* attributeBuffer = new UINT[numTriangles];
		fin >> ignore; // triangles header text
		for(UINT i = 0; i < numTriangles; ++i)
		{
			fin >> indices[i*3+0];
			fin >> indices[i*3+1];
			fin >> indices[i*3+2];
			fin >> attributeBuffer[i];
		}
		HR(mMeshData->SetIndexData(indices, numTriangles*3));
		HR(mMeshData->SetAttributeData(attributeBuffer));

		delete[] indices;
		delete[] attributeBuffer;

		HR(mMeshData->GenerateAdjacencyAndPointReps(0.001f));
		HR(mMeshData->Optimize(D3DX10_MESHOPT_ATTR_SORT|D3DX10_MESHOPT_VERTEX_CACHE,0,0));
		HR(mMeshData->CommitToDevice());
	}
}

void Mesh::setLight(const Light& light)
{
	mfxLightVar->SetRawValue((void**)&light, 0, sizeof(Light));
}

void Mesh::setEyePos(const D3DXVECTOR3& eyePos)
{
	mfxEyePosVar->SetRawValue((void**)&eyePos, 0, sizeof(D3DXVECTOR3));
}

void Mesh::setCubeMap(ID3D10ShaderResourceView* cubeMap)
{
	mfxCubeMapVar->SetResource(cubeMap);
}

void Mesh::setShadowMap(ID3D10ShaderResourceView* shadowMap)
{
	mfxShadowMapVar->SetResource(shadowMap);
}

void Mesh::enableCubeMap(bool enable)
{
	mfxCubeMapEnabledVar->SetBool(enable);
}

void Mesh::unbindShadowMap()
{
	// Unbind shadow map from shader stage since we will be binding it
	// as a depth buffer when we rebuild the shadow map the next frame.

	mfxShadowMapVar->SetResource(0);

	D3D10_TECHNIQUE_DESC techDesc;
    mTech->GetDesc( &techDesc );

    for(UINT i = 0; i < techDesc.Passes; ++i)
    {
        ID3D10EffectPass* pass = mTech->GetPassByIndex(i);
		pass->Apply(0);
	}
}

void Mesh::draw(const D3DXMATRIX& world, const D3DXMATRIX& lightViewProj)
{
	D3DXMATRIX view = GetCamera().view();
	D3DXMATRIX proj = GetCamera().proj();

	D3DXMATRIX WVP = world*view*proj;

	D3DXMATRIX lightWVP = world*lightViewProj;

	mfxWVPVar->SetMatrix((float*)&WVP);
	mfxLightWVPVar->SetMatrix((float*)&lightWVP);
	mfxWorldVar->SetMatrix((float*)&world);

    D3D10_TECHNIQUE_DESC techDesc;
    mTech->GetDesc( &techDesc );

    for(UINT i = 0; i < techDesc.Passes; ++i)
    {
        ID3D10EffectPass* pass = mTech->GetPassByIndex(i);

		for(UINT subsetID = 0; subsetID < mNumSubsets; ++subsetID)
		{
			mfxReflectMtrlVar->SetRawValue((void*)&mReflectMtrls[subsetID], 0, sizeof(D3DXVECTOR3));
			mfxDiffuseMapVar->SetResource(mDiffuseTextures[subsetID]);
			mfxSpecMapVar->SetResource(mSpecTextures[subsetID]);
			mfxNormalMapVar->SetResource(mNormalTextures[subsetID]);

			mfxCubeMapEnabledVar->SetBool(false);

			pass->Apply(0);
			mMeshData->DrawSubset(subsetID);
		}
	}
}

void Mesh::drawToShadowMap(ID3D10EffectShaderResourceVariable* diffuseMapVar,
						   ID3D10EffectPass* pass)
{
	// We only need diffuse map for drawing into shadow map.
	for(UINT subsetID = 0; subsetID < mNumSubsets; ++subsetID)
	{
		diffuseMapVar->SetResource(mDiffuseTextures[subsetID]);
		pass->Apply(0);
		mMeshData->DrawSubset(subsetID);
	}
}

void Mesh::getFXHandles()
{
	mTech                = fx::MeshFX->GetTechniqueByName("MeshTech");
	mfxLightVar          = fx::MeshFX->GetVariableByName("gLight");
	mfxEyePosVar         = fx::MeshFX->GetVariableByName("gEyePosW");
	mfxLightWVPVar       = fx::MeshFX->GetVariableByName("gLightWVP")->AsMatrix();
	mfxWVPVar            = fx::MeshFX->GetVariableByName("gWVP")->AsMatrix();
	mfxWorldVar          = fx::MeshFX->GetVariableByName("gWorld")->AsMatrix();
	mfxReflectMtrlVar    = fx::MeshFX->GetVariableByName("gReflectMtrl")->AsVector();
	mfxCubeMapEnabledVar = fx::MeshFX->GetVariableByName("gCubeMapEnabled")->AsScalar();
	mfxDiffuseMapVar     = fx::MeshFX->GetVariableByName("gDiffuseMap")->AsShaderResource();
	mfxSpecMapVar        = fx::MeshFX->GetVariableByName("gSpecMap")->AsShaderResource();
	mfxNormalMapVar      = fx::MeshFX->GetVariableByName("gNormalMap")->AsShaderResource();
	mfxShadowMapVar      = fx::MeshFX->GetVariableByName("gShadowMap")->AsShaderResource();
	mfxCubeMapVar        = fx::MeshFX->GetVariableByName("gCubeMap")->AsShaderResource();

	mfxCubeMapEnabledVar->SetBool(false);
}