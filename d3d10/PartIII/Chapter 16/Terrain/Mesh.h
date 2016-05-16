#ifndef MESH_H
#define MESH_H

#include "d3dUtil.h"
#include "Light.h"
#include <vector>
#include <string>
 
struct MeshVertex
{
	D3DXVECTOR3 pos;
	D3DXVECTOR3 tangent;
	D3DXVECTOR3 normal;
	D3DXVECTOR2 texC;
};

class Mesh
{
public:
	Mesh();
	~Mesh();

	ID3DX10Mesh* d3dxMesh();

	void getAABB(D3DXVECTOR3& vMin, D3DXVECTOR3& vMax);
 
	void init(ID3D10Device* device, std::wstring filename);

	void setLight(const Light& light);
	void setEyePos(const D3DXVECTOR3& eyePos);
	void setCubeMap(ID3D10ShaderResourceView* cubeMap);
	void setShadowMap(ID3D10ShaderResourceView* shadowMap);
	void enableCubeMap(bool enable);
	void unbindShadowMap();

	void draw(const D3DXMATRIX& world, const D3DXMATRIX& lightViewProj);
	void drawToShadowMap(
		ID3D10EffectShaderResourceVariable* diffuseMapVar,
		ID3D10EffectPass* pass);

private:
	Mesh(const Mesh& rhs);
	Mesh& operator=(const Mesh& rhs);
 
	void getFXHandles();

private:

	ID3D10Device* md3dDevice;
	ID3DX10Mesh* mMeshData;

	ID3D10EffectTechnique* mTech;
	ID3D10EffectVariable* mfxLightVar;
	ID3D10EffectVariable* mfxEyePosVar;
	ID3D10EffectMatrixVariable* mfxLightWVPVar;
	ID3D10EffectMatrixVariable* mfxWVPVar;
	ID3D10EffectMatrixVariable* mfxWorldVar;
	ID3D10EffectVectorVariable* mfxReflectMtrlVar;
	ID3D10EffectScalarVariable* mfxCubeMapEnabledVar;
	ID3D10EffectShaderResourceVariable* mfxDiffuseMapVar;
	ID3D10EffectShaderResourceVariable* mfxSpecMapVar;
	ID3D10EffectShaderResourceVariable* mfxNormalMapVar;
	ID3D10EffectShaderResourceVariable* mfxShadowMapVar;
	ID3D10EffectShaderResourceVariable* mfxCubeMapVar;

	UINT mNumSubsets;
	std::vector<D3DXVECTOR3> mReflectMtrls;
	std::vector<ID3D10ShaderResourceView*> mDiffuseTextures;
	std::vector<ID3D10ShaderResourceView*> mSpecTextures;
	std::vector<ID3D10ShaderResourceView*> mNormalTextures;
};

#endif // MESH