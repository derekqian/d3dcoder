//=============================================================================
// buildshadowmap.fx by Frank Luna (C) 2008 All Rights Reserved.
//
// Effect used to build the shadow map.
//=============================================================================

cbuffer cbPerFrame
{
	float4x4 gLightWVP;
};

// Nonnumeric values cannot be added to a cbuffer.
Texture2D gDiffuseMap;
 
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
	float4 posH : SV_POSITION;
	float2 texC : TEXCOORD;
};
 
VS_OUT VS(VS_IN vIn)
{
	VS_OUT vOut;

	vOut.posH = mul(float4(vIn.posL, 1.0f), gLightWVP);
	
	vOut.texC = vIn.texC;
	
	return vOut;
}

void PS(VS_OUT pIn)
{
	float4 diffuse = gDiffuseMap.Sample( gTriLinearSam, pIn.texC );

	// Don't write transparent pixels to the shadow map.
	clip(diffuse.a - 0.15f);
}

technique10 BuildShadowMapTech
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS() ) );
    }
}
