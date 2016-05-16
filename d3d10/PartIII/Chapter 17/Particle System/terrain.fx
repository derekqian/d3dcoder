//=============================================================================
// terrain.fx by Frank Luna (C) 2008 All Rights Reserved.
//
// Effect used for terrain.
//=============================================================================


#include "lighthelper.fx"
 
cbuffer cbPerFrame
{
	float4x4 gWorld;
	float4x4 gWVP; 
	float4 gDirToSunW;
};

cbuffer cbFixed
{
	float gTexScale = 20;
};
 
// Nonnumeric values cannot be added to a cbuffer.
Texture2D gLayer0;
Texture2D gLayer1;
Texture2D gLayer2;
Texture2D gLayer3;
Texture2D gLayer4;
Texture2D gBlendMap;

SamplerState gTriLinearSam
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

struct VS_IN
{
	float3 posL    : POSITION;
	float3 normalL : NORMAL;
	float2 texC    : TEXCOORD;
};

struct VS_OUT
{
	float4 posH         : SV_POSITION;
	float shade         : SHADE;
    float2 tiledUV      : TEXCOORD0;
    float2 stretchedUV  : TEXCOORD1; 
};

VS_OUT VS(VS_IN vIn)
{
	VS_OUT vOut;
	
	vOut.posH = mul(float4(vIn.posL, 1.0f), gWVP);
	
	float4 normalW = mul(float4(vIn.normalL, 0.0f), gWorld);
	
	vOut.shade = saturate(max(dot(normalW, gDirToSunW), 0.0f) + 0.1f);

	vOut.tiledUV     = gTexScale*vIn.texC;
	vOut.stretchedUV = vIn.texC;
	
	return vOut;
}

float4 PS(VS_OUT pIn) : SV_Target
{
	float4 c0 = gLayer0.Sample( gTriLinearSam, pIn.tiledUV );
	float4 c1 = gLayer1.Sample( gTriLinearSam, pIn.tiledUV );
	float4 c2 = gLayer2.Sample( gTriLinearSam, pIn.tiledUV );
	float4 c3 = gLayer3.Sample( gTriLinearSam, pIn.tiledUV );
	float4 c4 = gLayer4.Sample( gTriLinearSam, pIn.tiledUV ); 
	
	float4 t = gBlendMap.Sample( gTriLinearSam, pIn.stretchedUV ); 
    
    float4 C = c0;
    C = lerp(C, c1, t.r);
    C = lerp(C, c2, t.g);
    C = lerp(C, c3, t.b);
    C = lerp(C, c4, t.a);
    
    C *= pIn.shade;
      
    return C;
}

technique10 TerrainTech
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS() ) );
    }
}
