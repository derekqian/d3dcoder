//=============================================================================
// fieeworks.fx by Frank Luna (C) 2004 All Rights Reserved.
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
    float4 color : COLOR0;
    float2 tex0  : TEXCOORD0; // D3D fills in for point sprites.
    float size   : PSIZE; // In pixels.
};

OutputVS FireWorksVS(float3 posL    : POSITION0, 
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
	
	// Oscillate particles up and down.
	float s = sin(6.0f*t);
	posL.y += mass*s;
	
	// Constant acceleration.
	posL = posL + vel*t + 0.5f * mass * gAccel * t * t;
	
	// Transform to homogeneous clip space.
	outVS.posH = mul(float4(posL, 1.0f), gWVP);
		
	// Compute size as a function of the viewport heights.  
	outVS.size = gViewportHeight*size/600.0f;
	
	// Pass on color to pixel pipeline.
	outVS.color = color;
	
	// Done--return the output.
    return outVS;
}

float4 FireWorksPS(float4 color : COLOR0, float2 tex0 : TEXCOORD0) : COLOR
{
	// Remember we are using One blend factors, which don't even use the
	// alpha component.  So multiply color by alpha component of texture
	// to mask it.
	return float4(color.rgb*tex2D(TexS, tex0).rgb, 1.0f);
}

technique FireWorksTech
{
    pass P0
    {
        vertexShader = compile vs_2_0 FireWorksVS();
        pixelShader  = compile ps_2_0 FireWorksPS();
        
        PointSpriteEnable = true;
        AlphaBlendEnable = true;
	    SrcBlend     = One;
	    DestBlend    = One;
	    
	    // Don't write to depth buffer; that is, particles don't obscure
	    // the objects behind them.
	    ZWriteEnable = false;
    }
}
