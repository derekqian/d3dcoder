//=============================================================================
// TerrainMultiTex.fx by Frank Luna (C) 2004 All Rights Reserved.
//
// Blends three textures together with a blend map.
//=============================================================================

uniform extern float4x4 gWorld;
uniform extern float4x4 gWorldInvTrans;
uniform extern float4x4 gWVP;

uniform extern float4 gAmbientMtrl;
uniform extern float4 gAmbientLight;
uniform extern float4 gDiffuseMtrl;
uniform extern float4 gDiffuseLight;
uniform extern float4 gSpecularMtrl;
uniform extern float4 gSpecularLight;
uniform extern float  gSpecularPower;
uniform extern float3 gLightVecW;
uniform extern float3 gEyePosW;
uniform extern texture gTex0;
uniform extern texture gTex1;
uniform extern texture gTex2;
uniform extern texture gBlendMap;

// Use Anisotropic filtering since when we are low to the ground, the 
// ground plane is near a 90 degree angle with our view direction.
sampler Tex0S = sampler_state
{
	Texture = <gTex0>;
	MinFilter = Anisotropic;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	MaxAnisotropy = 8;
	AddressU  = WRAP;
    AddressV  = WRAP;
};

sampler Tex1S = sampler_state
{
	Texture = <gTex1>;
	MinFilter = Anisotropic;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	MaxAnisotropy = 8;
	AddressU  = WRAP;
    AddressV  = WRAP;
};

sampler Tex2S = sampler_state
{
	Texture = <gTex2>;
	MinFilter = Anisotropic;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	MaxAnisotropy = 8;
	AddressU  = WRAP;
    AddressV  = WRAP;
};

sampler BlendMapS = sampler_state
{
	Texture = <gBlendMap>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU  = WRAP;
    AddressV  = WRAP;
};
 
struct OutputVS
{
    float4 posH         : POSITION0;
    float4 diffuse      : COLOR0;
    float4 spec         : COLOR1;
    float2 tiledTexC    : TEXCOORD0;
    float2 nonTiledTexC : TEXCOORD1;
};

OutputVS TerrainMultiTexVS(float3 posL : POSITION0, 
                           float3 normalL : NORMAL0, 
                           float2 tex0: TEXCOORD0)
{
    // Zero out our output.
	OutputVS outVS = (OutputVS)0;
	
	// Transform normal to world space.
	float3 normalW = mul(float4(normalL, 0.0f), gWorldInvTrans).xyz;
	normalW = normalize(normalW);
	
	// Transform vertex position to world space.
	float3 posW  = mul(float4(posL, 1.0f), gWorld).xyz;
	
	//=======================================================
	// Compute the color: Equation 10.3.
	
	// Compute the vector from the vertex to the eye position.
	float3 toEye = normalize(gEyePosW - posW);
	
	// Compute the reflection vector.
	float3 r = reflect(-gLightVecW, normalW);
	
	// Determine how much (if any) specular light makes it into the eye.
	float t  = pow(max(dot(r, toEye), 0.0f), gSpecularPower);
	
	// Determine the diffuse light intensity that strikes the vertex.
	float s = max(dot(gLightVecW, normalW), 0.0f);
	
	// Compute the ambient, diffuse and specular terms separatly. 
	float3 spec = t*(gSpecularMtrl*gSpecularLight).rgb;
	float3 diffuse = s*(gDiffuseMtrl*gDiffuseLight).rgb;
	float3 ambient = gAmbientMtrl*gAmbientLight;
	
	// Sum all the terms together and copy over the diffuse alpha.
	outVS.diffuse.rgb = ambient + diffuse;
	outVS.diffuse.a   = gDiffuseMtrl.a;
	outVS.spec = float4(spec, 0.0f);
	//=======================================================
	
	// Transform to homogeneous clip space.
	outVS.posH = mul(float4(posL, 1.0f), gWVP);
	
	// Pass on texture coordinates to be interpolated in rasterization.
	outVS.tiledTexC    = tex0 * 16.0f; // Scale tex-coord to tile 16 times.
	outVS.nonTiledTexC = tex0;
	
	// Done--return the output.
    return outVS;
}

float4 TerrainMultiTexPS(float4 diffuse : COLOR0, 
						 float4 spec    : COLOR1,
                         float2 tiledTexC : TEXCOORD0, 
                         float2 nonTiledTexC : TEXCOORD1) : COLOR
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
    
    // Sum the colors and modulate with the lighting color.
    float3 final = (c0 + c1 + c2) * diffuse.rgb;
    
    return float4(final + spec, diffuse.a);
}

technique TerrainMultiTexTech
{
    pass P0
    {
        // Specify the vertex and pixel shader associated with this pass.
        vertexShader = compile vs_2_0 TerrainMultiTexVS();
        pixelShader  = compile ps_2_0 TerrainMultiTexPS();
    }
}
