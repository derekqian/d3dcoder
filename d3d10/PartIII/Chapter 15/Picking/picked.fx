//=============================================================================
// picked.fx by Frank Luna (C) 2008 All Rights Reserved.
//
// Simple effect used to color the picked triangle to distinguish it
// from the other triangles.
//=============================================================================

cbuffer cbPerObject
{

	float4x4 gWVP; 
};


struct VS_IN
{
	float3 posL     : POSITION;
};

struct VS_OUT
{
	float4 posH     : SV_POSITION;
};
 
VS_OUT VS(VS_IN vIn)
{
	VS_OUT vOut;
	
	vOut.posH = mul(float4(vIn.posL, 1.0f), gWVP);
	
	return vOut;
}

float4 PS(VS_OUT pIn) : SV_Target
{    
    // green with 50% opacity
    return float4(0.0f, 1.0f, 0.0f, 0.5f);
}

DepthStencilState DisableDepth
{
    DepthEnable = FALSE;
    DepthWriteMask = ZERO;
};

BlendState Transparency
{
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = TRUE;
    SrcBlend = SRC_ALPHA;
    DestBlend = INV_SRC_ALPHA;
    BlendOp = ADD;
    SrcBlendAlpha = ZERO;
    DestBlendAlpha = ZERO;
    BlendOpAlpha = ADD;
    RenderTargetWriteMask[0] = 0x0F;
};

technique10 PickedTech
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS() ) );
        
        SetBlendState(Transparency, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xffffffff);
        
        // Turn off depth test so that there is no z-fighting when we render
        // the picked triangle twice.
        SetDepthStencilState( DisableDepth, 0 );
    }
}
