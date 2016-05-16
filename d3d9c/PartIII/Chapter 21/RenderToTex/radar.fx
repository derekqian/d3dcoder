//=============================================================================
// radar.fx by Frank Luna (C) 2004 All Rights Reserved.
//
// FX file texturing the radar quad.
//=============================================================================

uniform extern texture  gTex;

sampler TexS = sampler_state
{
	Texture = <gTex>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU  = WRAP;
    AddressV  = WRAP;
};
 
struct OutputVS
{
    float4 posH    : POSITION0;
    float2 tex0    : TEXCOORD0;
};

OutputVS RadarVS(float3 posL : POSITION0, float2 tex0 : TEXCOORD0)
{
    // Zero out our output.
	OutputVS outVS = (OutputVS)0;
	
	// Position already in normalized device coordinates.
	outVS.posH = float4(posL, 1.0f);
	
	// Pass on texture coordinates to be interpolated in rasterization.
	outVS.tex0 = tex0;

	// Done--return the output.
    return outVS;
}

float4 RadarPS(float2 tex0 : TEXCOORD0) : COLOR
{
	return tex2D(TexS, tex0);
}

technique RadarTech
{
    pass P0
    {
        // Specify the vertex and pixel shader associated with this pass.
        vertexShader = compile vs_2_0 RadarVS();
        pixelShader  = compile ps_2_0 RadarPS();
    }
}
