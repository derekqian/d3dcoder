//=============================================================================
// clouds.fx by Frank Luna (C) 2004 All Rights Reserved.
//
// Scrolls two cloud textures to simulate moving clouds.
//=============================================================================

 
uniform extern float4x4 gWVP;
uniform extern texture gCloudTex0;
uniform extern texture gCloudTex1;
uniform extern float2  gTexOffset0;
uniform extern float2  gTexOffset1;

sampler CloudS0 = sampler_state
{
	Texture = <gCloudTex0>;
	MinFilter = Anisotropic;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	MaxAnisotropy = 8;
	AddressU  = WRAP;
    AddressV  = WRAP;
};
 
sampler CloudS1 = sampler_state
{
	Texture = <gCloudTex1>;
	MinFilter = Anisotropic;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	MaxAnisotropy = 8;
	AddressU  = WRAP;
    AddressV  = WRAP;
};

struct OutputVS
{
    float4 posH : POSITION0;
    float2 tex0 : TEXCOORD0;
    float2 tex1 : TEXCOORD1;
};

OutputVS CloudsVS(float3 posL : POSITION0, float2 tex0: TEXCOORD0)
{
    // Zero out our output.
	OutputVS outVS = (OutputVS)0;
	
	// Transform to homogeneous clip space.
	outVS.posH = mul(float4(posL, 1.0f), gWVP);
	
	// Pass on texture coordinates to be interpolated in rasterization.
	outVS.tex0 = tex0 + gTexOffset0;
	outVS.tex1 = tex0 + gTexOffset1;
	
	// Done--return the output.
    return outVS;
}

float4 CloudsPS(float4 c : COLOR0, float2 tex0 : TEXCOORD0, float2 tex1 : TEXCOORD1) : COLOR
{
	float3 c0 = tex2D(CloudS0, tex0).rgb;
	float3 c1 = tex2D(CloudS1, tex1).rgb;
	float3 blue = float3(0.0f, 0.0f, 1.0f);
    return float4(c0+c1+blue, 1.0f);
}

technique CloudsTech
{
    pass P0
    {
        vertexShader = compile vs_2_0 CloudsVS();
        pixelShader  = compile ps_2_0 CloudsPS();
    }
}
