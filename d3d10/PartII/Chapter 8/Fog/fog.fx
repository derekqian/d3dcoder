//=============================================================================
// fog.fx by Frank Luna (C) 2008 All Rights Reserved.
//
// Same as clip.fx, but adds fog in the vertex shader.
// Note that we could do fog in the pixel shader, but if the scene
// has sufficient triangle density, not much is gained. 
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
};

cbuffer cbFixed
{
	// For this demo, we hardcode the fog values.  However, in a real
	// application, the program may want to change the fog settings
	// at runtime; for example, to fade the fog in and out based on 
	// the time of day or the location of the game player.
 	
	float  gFogStart = 5.0f;
	float  gFogRange = 140.0f;
	float3 gFogColor = {0.7f, 0.7f, 0.7f};
};

// Nonnumeric values cannot be added to a cbuffer.
Texture2D gDiffuseMap;
Texture2D gSpecMap;

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
	float4 posH    : SV_POSITION;
    float3 posW    : POSITION;
    float3 normalW : NORMAL;
    float2 texC0   : TEXCOORD0;
    float2 texC1   : TEXCOORD1;
    float  fogLerp : FOG;
};
 
VS_OUT VS(VS_IN vIn)
{
	VS_OUT vOut;
	
	// Transform to world space space.
	vOut.posW    = mul(float4(vIn.posL, 1.0f), gWorld);
	vOut.normalW = mul(float4(vIn.normalL, 0.0f), gWorld);
		
	// Transform to homogeneous clip space.
	vOut.posH = mul(float4(vIn.posL, 1.0f), gWVP);
	
	// Output vertex attributes for interpolation across triangle.
	vOut.texC0  = vIn.texC;
	vOut.texC1  = mul(float4(vIn.texC, 0.0f, 1.0f), gTexMtx);
	
	float d      = distance(vOut.posW, gEyePosW);
	vOut.fogLerp = saturate( (d - gFogStart) / gFogRange ); 
	
	return vOut;
}

float4 PS(VS_OUT pIn) : SV_Target
{
	// Get materials from texture maps.
	float alpha    = gDiffuseMap.Sample( gTriLinearSam, pIn.texC0 ).a;
	
	// Discard pixel if texture alpha < 0.1.  Note that we do this
	// test as soon as possible so that we can potentially exit the shader 
	// early, thereby skipping the rest of the shader code.
	clip(alpha - 0.1f);
	
	float4 diffuse = gDiffuseMap.Sample( gTriLinearSam, pIn.texC1 );
	float4 spec    = gSpecMap.Sample( gTriLinearSam, pIn.texC1 );
	
	// Map [0,1] --> [0,256]
	spec.a *= 256.0f;
	
	// Interpolating normal can make it not be of unit length so normalize it.
    float3 normalW = normalize(pIn.normalW);
    
	// Compute the lit color for this pixel.
    SurfaceInfo v = {pIn.posW, normalW, diffuse, spec};
	float3 litColor = ParallelLight(v, gLight, gEyePosW);
	
	// Blend the fog color and the lit color.
	float3 foggedColor = lerp(litColor, gFogColor, pIn.fogLerp);
	
    return float4(foggedColor, alpha);
}

technique10 FogTech
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS() ) );
    }
}
