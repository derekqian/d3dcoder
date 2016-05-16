//=============================================================================
// sprinkler.fx by Frank Luna (C) 2004 All Rights Reserved.
//=============================================================================

uniform extern float4x4 gWVP;
uniform extern texture  gTex;
uniform extern float3   gEyePosL;
uniform extern float3   gAccel;
uniform extern float    gTime;
uniform extern int      gViewportHeight;

sampler TexS = sampler_state
{
	Texture = <gTex>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = POINT;
	AddressU  = CLAMP;
    AddressV  = CLAMP;
};
 
struct OutputVS
{
    float4 posH  : POSITION0;
    float2 tex0  : TEXCOORD0; // D3D fills in for point sprites.
    float size   : PSIZE; // In pixels.
};

OutputVS SprinklerVS(float3 posL    : POSITION0, 
                     float3 vel     : TEXCOORD0,
                     float size     : TEXCOORD1,
                     float time     : TEXCOORD2,
                     float lifeTime : TEXCOORD3,
                     float mass     : TEXCOORD4,
                     float4 color   : COLOR0)
{
    // Zero out our output.
	OutputVS outVS = (OutputVS)0;
	
	// Get age of particle from creation time.
	float t = gTime - time;
	
	// Constant acceleration.
	posL = posL + vel*t + 0.5f * gAccel * t * t;
	
	// Transform to homogeneous clip space.
	outVS.posH = mul(float4(posL, 1.0f), gWVP);
		
	// Compute size as a function of the distance from the camera,
	// and the viewport heights.  The constants were found by 
	// experimenting.
	float d = distance(posL, gEyePosL);
	outVS.size = gViewportHeight*size/(1.0f + 8.0f*d);

	// Done--return the output.
    return outVS;
}

float4 SprinklerPS(float2 tex0 : TEXCOORD0) : COLOR
{
	return tex2D(TexS, tex0);
}

technique SprinklerTech
{
    pass P0
    {
        vertexShader = compile vs_2_0 SprinklerVS();
        pixelShader  = compile ps_2_0 SprinklerPS();
        
        PointSpriteEnable = true;
        AlphaBlendEnable = true;
	    SrcBlend     = One;
	    DestBlend    = One;
	    
	    
	    
	    // Don't write to depth buffer; that is, particles don't obscure
	    // the objects behind them.
	    ZWriteEnable = false;
    }
}
