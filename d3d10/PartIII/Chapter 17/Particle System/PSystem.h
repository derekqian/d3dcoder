#ifndef PARTICLESYS_H
#define PARTICLESYS_H

#include "d3dUtil.h"
#include <string>
#include <vector>

class PSystem
{
public:
	
	PSystem();
	~PSystem();

	// Time elapsed since the system was reset.
	float getAge()const;

	void setEyePos(const D3DXVECTOR3& eyePosW);
	void setEmitPos(const D3DXVECTOR3& emitPosW);
	void setEmitDir(const D3DXVECTOR3& emitDirW);

	void init(ID3D10Device* device, ID3D10Effect* FX, 
		ID3D10ShaderResourceView* texArrayRV, UINT maxParticles);

	void reset();
	void update(float dt, float gameTime);
	void draw();

private:
	void buildVB();

	PSystem(const PSystem& rhs);
	PSystem& operator=(const PSystem& rhs);
 
private:
 
	UINT mMaxParticles;
	bool mFirstRun;

	float mGameTime;
	float mTimeStep;
	float mAge;

	D3DXVECTOR4 mEyePosW;
	D3DXVECTOR4 mEmitPosW;
	D3DXVECTOR4 mEmitDirW;

	ID3D10Device* md3dDevice;
	ID3D10Buffer* mInitVB;	
	ID3D10Buffer* mDrawVB;
	ID3D10Buffer* mStreamOutVB;
 
	ID3D10ShaderResourceView* mTexArrayRV;
	ID3D10ShaderResourceView* mRandomTexRV;

	ID3D10EffectTechnique* mStreamOutTech;
	ID3D10EffectTechnique* mDrawTech;
	ID3D10EffectMatrixVariable* mfxViewProjVar;
	ID3D10EffectScalarVariable* mfxGameTimeVar;
	ID3D10EffectScalarVariable* mfxTimeStepVar;
	ID3D10EffectVectorVariable* mfxEyePosVar;
	ID3D10EffectVectorVariable* mfxEmitPosVar;
	ID3D10EffectVectorVariable* mfxEmitDirVar;
	ID3D10EffectShaderResourceVariable* mfxTexArrayVar;
	ID3D10EffectShaderResourceVariable* mfxRandomTexVar;
};

#endif // PARTICLESYS_H