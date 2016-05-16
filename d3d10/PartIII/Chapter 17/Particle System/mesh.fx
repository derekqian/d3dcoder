//=============================================================================
// mesh.fx by Frank Luna (C) 2008 All Rights Reserved.
//
// Effect used for meshes.
//=============================================================================


#include "lighthelper.fx"
 
static const float SHADOW_EPSILON = 0.002f;
static const float SMAP_SIZE = 1024.0f;
static const float SMAP_DX = 1.0f / SMAP_SIZE;
 
cbuffer cbPerFrame
{
	Light gLight;
	float3 gEyePosW;
};

cbuffer cbPerObject
{
	float4x4 gLightWVP; 
	float4x4 gWorld;
	float4x4 gWVP; 
	float4 gReflectMtrl;
	bool gCubeMapEnabled;
};

// Nonnumeric values cannot be added to a cbuffer.
Texture2D gDiffuseMap;
Texture2D gSpecMap;
Texture2D gNormalMap;
Texture2D gShadowMap;
TextureCube gCubeMap;

SamplerState gShadowSam
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Clamp;
	AddressV = Clamp;
};	

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
    float2 texC     : TEXCOORD0;
    float4 projTexC : TEXCOORD1;
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
	
	// Generate projective tex-coords to project shadow map onto scene.
	vOut.projTexC = mul(float4(vIn.posL, 1.0f), gLightWVP);
	
	// Output vertex attributes for interpolation across triangle.
	vOut.texC = vIn.texC;
	
	return vOut;
}

float CalcShadowFactor(float4 projTexC)
{
	// Complete projection by doing division by w.
	projTexC.xyz /= projTexC.w;
	
	// Points outside the light volume are in shadow.
	if( projTexC.x < -1.0f || projTexC.x > 1.0f || 
	    projTexC.y < -1.0f || projTexC.y > 1.0f ||
	    projTexC.z < 0.0f )
	    return 0.0f;
	    
	// Transform from NDC space to texture space.
	projTexC.x = +0.5f*projTexC.x + 0.5f;
	projTexC.y = -0.5f*projTexC.y + 0.5f;
	
	// Depth in NDC space.
	float depth = projTexC.z;

	
	// Sample shadow map to get nearest depth to light.
	float s0 = gShadowMap.Sample(gShadowSam, projTexC.xy).r;
	float s1 = gShadowMap.Sample(gShadowSam, projTexC.xy + float2(SMAP_DX, 0)).r;
	float s2 = gShadowMap.Sample(gShadowSam, projTexC.xy + float2(0, SMAP_DX)).r;
	float s3 = gShadowMap.Sample(gShadowSam, projTexC.xy + float2(SMAP_DX, SMAP_DX)).r;
	
	// Is the pixel depth <= shadow map value?
	float result0 = depth <= s0 + SHADOW_EPSILON;
	float result1 = depth <= s1 + SHADOW_EPSILON;
	float result2 = depth <= s2 + SHADOW_EPSILON;
	float result3 = depth <= s3 + SHADOW_EPSILON;	
	
	// Transform to texel space.
	float2 texelPos = SMAP_SIZE*projTexC.xy;
	
	// Determine the interpolation amounts.
	float2 t = frac( texelPos );
	
	// Interpolate results.
	return lerp( lerp(result0, result1, t.x), 
	             lerp(result2, result3, t.x), t.y);
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
    
    float shadowFactor = CalcShadowFactor(pIn.projTexC);
    
	// Compute the lit color for this pixel.
    SurfaceInfo v = {pIn.posW, bumpedNormalW, diffuse, spec};
	float3 litColor = ParallelLight(v, gLight, gEyePosW, shadowFactor);
	
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
 
technique10 MeshTech
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS() ) );
    }
}
