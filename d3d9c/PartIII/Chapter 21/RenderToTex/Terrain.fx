//=============================================================================
// Terrain.fx by Frank Luna (C) 2004 All Rights Reserved.
//
// Blends three textures together with a blend map.
//=============================================================================


uniform extern float4x4 gViewProj;
uniform extern float3  gDirToSunW;
uniform extern texture gTex0;
uniform extern texture gTex1;
uniform extern texture gTex2;
uniform extern texture gBlendMap;

static float gTexScale = 48.0f;

sampler Tex0S = sampler_state
{
	Texture = <gTex0>;
	MinFilter = ANISOTROPIC;
	MaxAnisotropy = 8;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU  = WRAP;
    AddressV  = WRAP;
};

sampler Tex1S = sampler_state
{
	Texture = <gTex1>;
	MinFilter = ANISOTROPIC;
	MaxAnisotropy = 8;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU  = WRAP;
    AddressV  = WRAP;
};

sampler Tex2S = sampler_state
{
	Texture = <gTex2>;
	MinFilter = ANISOTROPIC;
	MaxAnisotropy = 8;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU  = WRAP;
    AddressV  = WRAP;
};

sampler BlendMapS = sampler_state
{
	Texture = <gBlendMap>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = POINT;
	AddressU  = WRAP;
    AddressV  = WRAP;
};
 
struct OutputVS
{
    float4 posH         : POSITION0;
    float2 tiledTexC    : TEXCOORD0;
    float2 nonTiledTexC : TEXCOORD1;
    float  shade        : TEXCOORD2;
};

OutputVS TerrainVS(float3 posW : POSITION0,  // We assume terrain geometry is specified
                   float3 normalW : NORMAL0, // directly in world space.
                   float2 tex0: TEXCOORD0)
{
    // Zero out our output.
	OutputVS outVS = (OutputVS)0;
	
	// Just compute a grayscale diffuse and ambient lighting 
	// term--terrain has no specular reflectance.  The color 
	// comes from the texture.
    outVS.shade = saturate(max(0.0f, dot(normalW, gDirToSunW)) + 0.3f);
    
	// Transform to homogeneous clip space.
	outVS.posH = mul(float4(posW, 1.0f), gViewProj);
	
	// Pass on texture coordinates to be interpolated in rasterization.
	outVS.tiledTexC    = tex0 * gTexScale; // Scale tex-coord to tile.
	outVS.nonTiledTexC = tex0; // Blend map not tiled.
	
	// Done--return the output.
    return outVS;
}

float4 TerrainPS(float2 tiledTexC : TEXCOORD0, 
                 float2 nonTiledTexC : TEXCOORD1,
                 float shade : TEXCOORD2) : COLOR
{
	// Layer maps are tiled
    float3 c0 = tex2D(Tex0S, tiledTexC).rgb;
    float3 c1 = tex2D(Tex1S, tiledTexC).rgb;
    float3 c2 = tex2D(Tex2S, tiledTexC).rgb;
    
    // Blendmap is not tiled.
    float3 B = tex2D(BlendMapS, nonTiledTexC).rgb;

	// Find the inverse of all the blend weights so that we can
	// scale the total color to the range [0, 1].
    float totalInverse = 1.0f / (B.r + B.g + B.b);
    
    // Scale the colors by each layer by its corresponding weight
    // stored in the blendmap.  
    c0 *= B.r * totalInverse;
    c1 *= B.g * totalInverse;
    c2 *= B.b * totalInverse;
    
    // Sum the colors and modulate with the shade to brighten/darken.
    float3 final = (c0 + c1 + c2) * shade;
    
    return float4(final, 1.0f);
}

technique TerrainTech
{
    pass P0
    {
        vertexShader = compile vs_2_0 TerrainVS();
        pixelShader  = compile ps_2_0 TerrainPS();
    }
}
