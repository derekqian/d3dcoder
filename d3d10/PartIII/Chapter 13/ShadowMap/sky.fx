//=============================================================================
// sky.fx by Frank Luna (C) 2008 All Rights Reserved.
//
// Effect used to shade sky dome.
//=============================================================================

cbuffer cbPerFrame
{
	float4x4 gWVP;
};
 
// Nonnumeric values cannot be added to a cbuffer.
TextureCube gCubeMap;

SamplerState gTriLinearSam
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

struct VS_IN
{
	float3 posL : POSITION;
};

struct VS_OUT
{
	float4 posH : SV_POSITION;
    float3 texC : TEXCOORD;
};
 
VS_OUT VS(VS_IN vIn)
{
	VS_OUT vOut;
	
	// set z = w so that z/w = 1 (i.e., skydome always on far plane).
	vOut.posH = mul(float4(vIn.posL, 1.0f), gWVP).xyww;
	
	// use local vertex position as cubemap lookup vector.
	vOut.texC = vIn.posL;
	
	return vOut;
}

float4 PS(VS_OUT pIn) : SV_Target
{
	return gCubeMap.Sample(gTriLinearSam, pIn.texC);
}

RasterizerState NoCull
{
    CullMode = None;
};

DepthStencilState LessEqualDSS
{
	// Make sure the depth function is LESS_EQUAL and not just LESS.  
	// Otherwise, the normalized depth values at z = 1 (NDC) will 
	// fail the depth test if the depth buffer was cleared to 1.
    DepthFunc = LESS_EQUAL;
};

technique10 SkyTech
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS() ) );
        
        SetRasterizerState(NoCull);
        SetDepthStencilState(LessEqualDSS, 0);
    }
}
