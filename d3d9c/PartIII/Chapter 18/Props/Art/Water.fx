//=============================================================================
// Water.fx by Frank Luna (C) 2004 All Rights Reserved.
//
// Basic alpha-blended water with fog.
//=============================================================================

uniform extern float4x4 gWorld;
uniform extern float4x4 gWVP;
uniform extern float3   gEyePosW;

static float3 gFogColor = {0.5f, 0.5f, 0.5f};
static float  gFogStart = 1.0f;
static float  gFogRange = 250.0f;

struct OutputVS
{
    float4 posH : POSITION0;
    float  fogLerpParam : TEXCOORD0;
};

OutputVS WaterVS(float3 posL : POSITION0)
{
    // Zero out our output.
	OutputVS outVS = (OutputVS)0;
	
	// Transform to homogeneous clip space.
	outVS.posH = mul(float4(posL, 1.0f), gWVP);
	
	float3 posW = mul(float4(posL, 1.0f), gWorld);
	
	// Compute vertex distance from camera in world space for fog calculation.
	float dist = distance(posW, gEyePosW);
	outVS.fogLerpParam = saturate((dist - gFogStart) / gFogRange);
	
	// Done--return the output.
    return outVS;
}

float4 WaterPS(float  fogLerpParam : TEXCOORD0) : COLOR
{
	// Blue-ish water.
	float3 color = float3(0.0f, 0.2f, 0.4f);
	
	// Add in fog color.
	float3 final = lerp(color, gFogColor, fogLerpParam);
	
	// Return color with transparency alpha.
	return float4(final, 0.5f);
}

technique WaterTech
{
    pass P0
    {
        vertexShader = compile vs_2_0 WaterVS();
        pixelShader  = compile ps_2_0 WaterPS();
        
        AlphaBlendEnable = true;
	    SrcBlend = SrcAlpha;
	    DestBlend = InvSrcAlpha;
    }    
}
