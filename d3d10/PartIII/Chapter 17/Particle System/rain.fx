//=============================================================================
// rain.fx by Frank Luna (C) 2008 All Rights Reserved.
//
// Rain particle system.  Particles are emitted directly in world space.
//=============================================================================


//***********************************************
// GLOBALS                                      *
//***********************************************

cbuffer cbPerFrame
{
	float4 gEyePosW;
	
	// for when the emit position/direction is varying
	float4 gEmitPosW;
	float4 gEmitDirW;
	
	float gGameTime;
	float gTimeStep;
	float4x4 gViewProj; 
};

cbuffer cbFixed
{
	// Net constant acceleration used to accerlate the particles.
	float3 gAccelW = {-1.0f, -9.8f, 0.0f};
	
	// Texture coordinates used to stretch texture over quad 
	// when we expand point particle into a quad.
	float2 gQuadTexC[4] = 
	{
		float2(0.0f, 1.0f),
		float2(1.0f, 1.0f),
		float2(0.0f, 0.0f),
		float2(1.0f, 0.0f)
	};
};
 
// Array of textures for texturing the particles.
Texture2DArray gTexArray;

// Random texture used to generate random numbers in shaders.
Texture1D gRandomTex;
 
SamplerState gTriLinearSam
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
};

DepthStencilState DisableDepth
{
    DepthEnable = FALSE;
    DepthWriteMask = ZERO;
};

DepthStencilState NoDepthWrites
{
    DepthEnable = TRUE;
    DepthWriteMask = ZERO;
};


//***********************************************
// HELPER FUNCTIONS                             *
//***********************************************
float3 RandUnitVec3(float offset)
{
	// Use game time plus offset to sample random texture.
	float u = (gGameTime + offset);
	
	// coordinates in [-1,1]
	float3 v = gRandomTex.SampleLevel(gTriLinearSam, u, 0);
	
	// project onto unit sphere
	return normalize(v);
}

float3 RandVec3(float offset)
{
	// Use game time plus offset to sample random texture.
	float u = (gGameTime + offset);
	
	// coordinates in [-1,1]
	float3 v = gRandomTex.SampleLevel(gTriLinearSam, u, 0);
	
	return v;
}
 
//***********************************************
// STREAM-OUT TECH                              *
//***********************************************

#define PT_EMITTER 0
#define PT_FLARE 1
 
struct Particle
{
	float3 initialPosW : POSITION;
	float3 initialVelW : VELOCITY;
	float2 sizeW       : SIZE;
	float age          : AGE;
	uint type          : TYPE;
};
  
Particle StreamOutVS(Particle vIn)
{
	return vIn;
}

// The stream-out GS is just responsible for emitting 
// new particles and destroying old particles.  The logic
// programed here will generally vary from particle system
// to particle system, as the destroy/spawn rules will be 
// different.
[maxvertexcount(6)]
void StreamOutGS(point Particle gIn[1], 
                 inout PointStream<Particle> ptStream)
{	
	gIn[0].age += gTimeStep;
	
	if( gIn[0].type == PT_EMITTER )
	{	
		// time to emit a new particle?
		if( gIn[0].age > 0.001f )
		{
			for(int i = 0; i < 5; ++i)
			{
				// Spread rain drops out above the camera.
				float3 vRandom = 35.0f*RandVec3((float)i/5.0f);
				vRandom.y = 20.0f;
			
				Particle p;
				p.initialPosW = gEmitPosW.xyz + vRandom;
				p.initialVelW = float3(0.0f, 0.0f, 0.0f);
				p.sizeW       = float2(1.0f, 1.0f);
				p.age         = 0.0f;
				p.type        = PT_FLARE;
			
				ptStream.Append(p);
			}
			
			// reset the time to emit
			gIn[0].age = 0.0f;
		}
		
		// always keep emitters
		ptStream.Append(gIn[0]);
	}
	else
	{
		// Specify conditions to keep particle; this may vary from system to system.
		if( gIn[0].age <= 4.0f )
			ptStream.Append(gIn[0]);
	}		
}

GeometryShader gsStreamOut = ConstructGSWithSO( 
	CompileShader( gs_4_0, StreamOutGS() ), 
	"POSITION.xyz; VELOCITY.xyz; SIZE.xy; AGE.x; TYPE.x" );
	
technique10 StreamOutTech
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, StreamOutVS() ) );
        SetGeometryShader( gsStreamOut );
        
        // disable pixel shader for stream-out only
        SetPixelShader(NULL);
        
        // we must also disable the depth buffer for stream-out only
        SetDepthStencilState( DisableDepth, 0 );
    }
}

//***********************************************
// DRAW TECH                                    *
//***********************************************

struct VS_OUT
{
	float3 posW  : POSITION;
	uint   type  : TYPE;
};

VS_OUT DrawVS(Particle vIn)
{
	VS_OUT vOut;
	
	float t = vIn.age;
	
	// constant acceleration equation
	vOut.posW = 0.5f*t*t*gAccelW + t*vIn.initialVelW + vIn.initialPosW;
	
	vOut.type  = vIn.type;
	
	return vOut;
}

struct GS_OUT
{
	float4 posH  : SV_Position;
	float2 texC  : TEXCOORD;
};

// The draw GS just expands points into lines.
[maxvertexcount(2)]
void DrawGS(point VS_OUT gIn[1], 
            inout LineStream<GS_OUT> lineStream)
{	
	// do not draw emitter particles.
	if( gIn[0].type != PT_EMITTER )
	{
		// Slant line in acceleration direction.
		float3 p0 = gIn[0].posW;
		float3 p1 = gIn[0].posW + 0.07f*gAccelW;
		
		GS_OUT v0;
		v0.posH = mul(float4(p0, 1.0f), gViewProj);
		v0.texC = float2(0.0f, 0.0f);
		lineStream.Append(v0);
		
		GS_OUT v1;
		v1.posH = mul(float4(p1, 1.0f), gViewProj);
		v1.texC = float2(1.0f, 1.0f);
		lineStream.Append(v1);
	}
}

float4 DrawPS(GS_OUT pIn) : SV_TARGET
{
	return gTexArray.Sample(gTriLinearSam, float3(pIn.texC, 0));
}

technique10 DrawTech
{
    pass P0
    {
        SetVertexShader(   CompileShader( vs_4_0, DrawVS() ) );
        SetGeometryShader( CompileShader( gs_4_0, DrawGS() ) );
        SetPixelShader(    CompileShader( ps_4_0, DrawPS() ) );
        
        SetDepthStencilState( NoDepthWrites, 0 );
    }
}