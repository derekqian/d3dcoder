#include "PSystem.h"
#include "TextureMgr.h"
#include "InputLayouts.h"
#include "Effects.h"
#include "Camera.h"

namespace
{
	struct ParticleVertex
	{
		D3DXVECTOR3 initialPos;
		D3DXVECTOR3 initialVel;
		D3DXVECTOR2 size;
		float age;
		unsigned int type;
	};
}

PSystem::PSystem()
: md3dDevice(0), mInitVB(0), mDrawVB(0), mStreamOutVB(0), mTexArrayRV(0), mRandomTexRV(0)
{
	mFirstRun = true;
	mGameTime = 0.0f;
	mTimeStep = 0.0f;
	mAge      = 0.0f;

	mEyePosW  = D3DXVECTOR4(0.0f, 0.0f, 0.0f, 1.0f);
	mEmitPosW = D3DXVECTOR4(0.0f, 0.0f, 0.0f, 1.0f);
	mEmitDirW = D3DXVECTOR4(0.0f, 1.0f, 0.0f, 0.0f);
}

PSystem::~PSystem()
{
	ReleaseCOM(mInitVB);
	ReleaseCOM(mDrawVB);
	ReleaseCOM(mStreamOutVB);
}

float PSystem::getAge()const
{
	return mAge;
}

void PSystem::setEyePos(const D3DXVECTOR3& eyePosW)
{
	mEyePosW = D3DXVECTOR4(eyePosW.x, eyePosW.y, eyePosW.z, 1.0f);
}

void PSystem::setEmitPos(const D3DXVECTOR3& emitPosW)
{
	mEmitPosW = D3DXVECTOR4(emitPosW.x, emitPosW.y, emitPosW.z, 1.0f);
}

void PSystem::setEmitDir(const D3DXVECTOR3& emitDirW)
{
	mEmitDirW = D3DXVECTOR4(emitDirW.x, emitDirW.y, emitDirW.z, 0.0f);
}

void PSystem::init(ID3D10Device* device, ID3D10Effect* FX, ID3D10ShaderResourceView* texArrayRV,
				   UINT maxParticles)
{
	md3dDevice = device;

	mMaxParticles = maxParticles;

	mTexArrayRV  = texArrayRV;
	mRandomTexRV = GetTextureMgr().getRandomTex(); 

 
	mStreamOutTech  = FX->GetTechniqueByName("StreamOutTech");	
	mDrawTech       = FX->GetTechniqueByName("DrawTech");	
	mfxViewProjVar  = FX->GetVariableByName("gViewProj")->AsMatrix();
	mfxGameTimeVar  = FX->GetVariableByName("gGameTime")->AsScalar();
	mfxTimeStepVar  = FX->GetVariableByName("gTimeStep")->AsScalar();
	mfxEyePosVar    = FX->GetVariableByName("gEyePosW")->AsVector();
	mfxEmitPosVar   = FX->GetVariableByName("gEmitPosW")->AsVector();
	mfxEmitDirVar   = FX->GetVariableByName("gEmitDirW")->AsVector();
	mfxTexArrayVar  = FX->GetVariableByName("gTexArray")->AsShaderResource();
	mfxRandomTexVar = FX->GetVariableByName("gRandomTex")->AsShaderResource();

	buildVB();
}

void PSystem::reset()
{
	mFirstRun = true;
	mAge      = 0.0f;
}

void PSystem::update(float dt, float gameTime)
{
	mGameTime = gameTime;
	mTimeStep = dt;

	mAge += dt;
}

void PSystem::draw()
{
	D3DXMATRIX V = GetCamera().view();
	D3DXMATRIX P = GetCamera().proj();

	//
	// Set constants.
	//
	mfxViewProjVar->SetMatrix((float*)&(V*P));
	mfxGameTimeVar->SetFloat(mGameTime);
	mfxTimeStepVar->SetFloat(mTimeStep);
	mfxEyePosVar->SetFloatVector((float*)&mEyePosW);
	mfxEmitPosVar->SetFloatVector((float*)&mEmitPosW);
	mfxEmitDirVar->SetFloatVector((float*)&mEmitDirW);
	mfxTexArrayVar->SetResource(mTexArrayRV);
	mfxRandomTexVar->SetResource(mRandomTexRV);
	//
	// Set IA stage.
	//
	md3dDevice->IASetInputLayout(InputLayout::Particle);
    md3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);

	UINT stride = sizeof(ParticleVertex);
    UINT offset = 0;

	// On the first pass, use the initialization VB.  Otherwise, use
	// the VB that contains the current particle list.
	if( mFirstRun )
		md3dDevice->IASetVertexBuffers(0, 1, &mInitVB, &stride, &offset);
	else
		md3dDevice->IASetVertexBuffers(0, 1, &mDrawVB, &stride, &offset);

	//
	// Draw the current particle list using stream-out only to update them.  
	// The updated vertices are streamed-out to the target VB. 
	//
	md3dDevice->SOSetTargets(1, &mStreamOutVB, &offset);

    D3D10_TECHNIQUE_DESC techDesc;
    mStreamOutTech->GetDesc( &techDesc );
    for(UINT p = 0; p < techDesc.Passes; ++p)
    {
        mStreamOutTech->GetPassByIndex( p )->Apply(0);
        
		if( mFirstRun )
		{
			md3dDevice->Draw(1, 0);
			mFirstRun = false;
		}
		else
		{
			md3dDevice->DrawAuto();
		}
    }

	// done streaming-out--unbind the vertex buffer
	ID3D10Buffer* bufferArray[1] = {0};
	md3dDevice->SOSetTargets(1, bufferArray, &offset);

	// ping-pong the vertex buffers
	std::swap(mDrawVB, mStreamOutVB);

	//
	// Draw the updated particle system we just streamed-out. 
	//
	md3dDevice->IASetVertexBuffers(0, 1, &mDrawVB, &stride, &offset);

	mDrawTech->GetDesc( &techDesc );
    for(UINT p = 0; p < techDesc.Passes; ++p)
    {
        mDrawTech->GetPassByIndex( p )->Apply(0);
        
		md3dDevice->DrawAuto();
    }
}

void PSystem::buildVB()
{
	//
	// Create the buffer to kick-off the particle system.
	//

    D3D10_BUFFER_DESC vbd;
    vbd.Usage = D3D10_USAGE_DEFAULT;
    vbd.ByteWidth = sizeof(ParticleVertex) * 1;
    vbd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;

	// The initial particle emitter has type 0 and age 0.  The rest
	// of the particle attributes do not apply to an emitter.
	ParticleVertex p;
	ZeroMemory(&p, sizeof(ParticleVertex));
	p.age  = 0.0f;
	p.type = 0; 
 
    D3D10_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = &p;

	HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mInitVB));
	
	//
	// Create the ping-pong buffers for stream-out and drawing.
	//
	vbd.ByteWidth = sizeof(ParticleVertex) * mMaxParticles;
    vbd.BindFlags = D3D10_BIND_VERTEX_BUFFER | D3D10_BIND_STREAM_OUTPUT;

    HR(md3dDevice->CreateBuffer(&vbd, 0, &mDrawVB));
	HR(md3dDevice->CreateBuffer(&vbd, 0, &mStreamOutVB));
}