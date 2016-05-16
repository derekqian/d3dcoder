#ifndef TERRAIN_H
#define TERRAIN_H

#include "d3dUtil.h"
#include <string>
#include <vector>

class Terrain
{
public:
	struct InitInfo
	{
		std::wstring HeightmapFilename;
		std::wstring LayerMapFilename0;
		std::wstring LayerMapFilename1;
		std::wstring LayerMapFilename2;
		std::wstring LayerMapFilename3;
		std::wstring LayerMapFilename4;
		std::wstring BlendMapFilename;
		float HeightScale;
		float HeightOffset;
		UINT NumRows;
		UINT NumCols;
		float CellSpacing;
	};

public:
	Terrain();
	~Terrain();

	float width()const;
	float depth()const;
	float getHeight(float x, float y)const;

	void init(ID3D10Device* device, const InitInfo& initInfo);

	void setDirectionToSun(const D3DXVECTOR3& v);

	void draw(const D3DXMATRIX& world);

private:
	void loadHeightmap();
	void smooth();
	bool inBounds(UINT i, UINT j);
	float average(UINT i, UINT j);
	void buildVB();
	void buildIB();

private:
	InitInfo mInfo;

	UINT mNumVertices;
	UINT mNumFaces;

	std::vector<float> mHeightmap;

	ID3D10Device* md3dDevice;
	ID3D10Buffer* mVB;
	ID3D10Buffer* mIB;

	ID3D10ShaderResourceView* mLayer0;
	ID3D10ShaderResourceView* mLayer1;
	ID3D10ShaderResourceView* mLayer2;
	ID3D10ShaderResourceView* mLayer3;
	ID3D10ShaderResourceView* mLayer4;
	ID3D10ShaderResourceView* mBlendMap;

	ID3D10EffectTechnique* mTech;
	ID3D10EffectMatrixVariable* mfxWVPVar;
	ID3D10EffectMatrixVariable* mfxWorldVar;
	ID3D10EffectVectorVariable* mfxDirToSunVar;
	ID3D10EffectShaderResourceVariable* mfxLayer0Var;
	ID3D10EffectShaderResourceVariable* mfxLayer1Var;
	ID3D10EffectShaderResourceVariable* mfxLayer2Var;
	ID3D10EffectShaderResourceVariable* mfxLayer3Var;
	ID3D10EffectShaderResourceVariable* mfxLayer4Var;
	ID3D10EffectShaderResourceVariable* mfxBlendMapVar;
};

#endif // TERRAIN_H