//=============================================================================
// normalmap.fx by Frank Luna (C) 2008 All Rights Reserved.
//
// Demonstrates normal mapping.
//=============================================================================


#include "lighthelper.fx"
 
 
cbuffer cbPerFrame
{
	Light gLight;
	float3 gEyePosW;
};

cbuffer cbPerObject
{
	float4x4 gWorld;
	float4x4 gWVP; 
	float4x4 gTexMtx;
	float4 gReflectMtrl;
	bool gCubeMapEnabled;
};

// Nonnumeric values cannot be added to a cbuffer.
Texture2D gDiffuseMap;
Texture2D gSpecMap;
Texture2D gNormalMap;
TextureCube gCubeMap;

SamplerState gTriLinearSam
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

struct VS_IN
{
	float3 posL     : POSITION;
	float3 tangentL : TANGENT;
	float3 normalL  : NORMAL;
	float2 texC     : TEXCOORD;
};

struct VS_OUT
{
	float4 posH     : SV_POSITION;
    float3 posW     : POSITION;
    float3 tangentW : TANGENT;
    float3 normalW  : NORMAL;
    float2 texC     : TEXCOORD;
};
 
VS_OUT VS(VS_IN vIn)
{
	VS_OUT vOut;
	
	// Transform to world space space.
	vOut.posW     = mul(float4(vIn.posL, 1.0f), gWorld);
	vOut.tangentW = mul(float4(vIn.tangentL, 0.0f), gWorld);
	vOut.normalW  = mul(float4(vIn.normalL, 0.0f), gWorld);
		
	// Transform to homogeneous clip space.
	vOut.posH = mul(float4(vIn.posL, 1.0f), gWVP);
	
	// Output vertex attributes for interpolation across triangle.
	vOut.texC = mul(float4(vIn.texC, 0.0f, 1.0f), gTexMtx);
	
	return vOut;
}

float4 PS(VS_OUT pIn) : SV_Target
{
	float4 diffuse = gDiffuseMap.Sample( gTriLinearSam, pIn.texC );
	
	// Kill transparent pixels.
	clip(diffuse.a - 0.15f);
	
	float4 spec    = gSpecMap.Sample( gTriLinearSam, pIn.texC );
	float3 normalT = gNormalMap.Sample( gTriLinearSam, pIn.texC );
	
	// Map [0,1] --> [0,256]
	spec.a *= 256.0f;
	
	// Uncompress each component from [0,1] to [-1,1].
	normalT = 2.0f*normalT - 1.0f;
	
	// build orthonormal basis
	float3 N = normalize(pIn.normalW);
	float3 T = normalize(pIn.tangentW - dot(pIn.tangentW, N)*N);
	float3 B = cross(N,T);
	
	float3x3 TBN = float3x3(T, B, N);
	
	// Transform from tangent space to world space.
	float3 bumpedNormalW = normalize(mul(normalT, TBN));
    
	// Compute the lit color for this pixel.
    SurfaceInfo v = {pIn.posW, bumpedNormalW, diffuse, spec};
	float3 litColor = ParallelLight(v, gLight, gEyePosW);
	
	[branch]
	if( gCubeMapEnabled )
	{
		float3 incident = pIn.posW - gEyePosW;
		float3 refW = reflect(incident, bumpedNormalW);
		float4 reflectedColor = gCubeMap.Sample(gTriLinearSam, refW);
		litColor += (gReflectMtrl*reflectedColor).rgb;
	}
    
    return float4(litColor, diffuse.a);
}
 
technique10 NormalMapTech
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS() ) );
    }
}
