//=============================================================================
// PSystem.h by Frank Luna (C) 2004 All Rights Reserved.
//=============================================================================

#ifndef PSYSTEM_H
#define PSYSTEM_H

#include "d3dUtil.h"
#include "Vertex.h"
#include <vector> 
#pragma comment(lib, "legacy_stdio_definitions.lib")

//===============================================================
class PSystem
{
public:
	PSystem(
		const std::string& fxName, 
		const std::string& techName, 
		const std::string& texName, 
		const D3DXVECTOR3& accel, 
		const AABB& box,
		int maxNumParticles,
		float timePerParticle);

	virtual ~PSystem();

	// Access Methods
	float getTime();
	void  setTime(float t);
	const AABB& getAABB()const;

	void setWorldMtx(const D3DXMATRIX& world);
	void addParticle();

	virtual void onLostDevice();
	virtual void onResetDevice();

	virtual void initParticle(Particle& out) = 0;
	virtual void update(float dt);
	virtual void draw();

protected:
	// In practice, some sort of ID3DXEffect and IDirect3DTexture9 manager should
	// be used so that you do not duplicate effects/textures by having several
	// instances of a particle system.
	ID3DXEffect* mFX;
	D3DXHANDLE mhTech;
	D3DXHANDLE mhWVP;
	D3DXHANDLE mhEyePosL;
	D3DXHANDLE mhTex;
	D3DXHANDLE mhTime;
	D3DXHANDLE mhAccel;
	D3DXHANDLE mhViewportHeight;

	IDirect3DTexture9* mTex;
	IDirect3DVertexBuffer9* mVB;
	D3DXMATRIX mWorld;
	D3DXMATRIX mInvWorld;
	float mTime;
	D3DXVECTOR3 mAccel;
	AABB mBox;
	int mMaxNumParticles;
	float mTimePerParticle;

	std::vector<Particle> mParticles;
	std::vector<Particle*> mAliveParticles;
	std::vector<Particle*> mDeadParticles; 
};

#endif // P_SYSTEM