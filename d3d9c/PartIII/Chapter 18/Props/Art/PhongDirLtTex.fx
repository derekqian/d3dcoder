//=============================================================================
// PhongDirLtTex.fx by Frank Luna (C) 2004 All Rights Reserved.
//
// Phong directional light & texture.
//=============================================================================

struct Mtrl
{
	float4 ambient;
	float4 diffuse;
	float4 spec;
	float  specPower;
};

struct DirLight
{
	float4 ambient;
	float4 diffuse;
	float4 spec;
	float3 dirW;  
};

uniform extern float4x4 gWorld;
uniform extern float4x4 gWorldInvTrans;
uniform extern float4x4 gWVP;
uniform extern Mtrl     gMtrl;
uniform extern DirLight gLight;
uniform extern float3   gEyePosW;
uniform extern texture  gTex;

static float3 gFogColor = {0.5f, 0.5f, 0.5f};
static float  gFogStart = 1.0f;
static float  gFogRange = 200.0f;

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
    float3 normalW : TEXCOORD0;
    float3 toEyeW  : TEXCOORD1;
    float2 tex0    : TEXCOORD2;
    float  fogLerpParam : TEXCOORD3;
};

OutputVS PhongDirLtTexVS(float3 posL : POSITION0, float3 normalL : NORMAL0, float2 tex0: TEXCOORD0)
{
    // Zero out our output.
	OutputVS outVS = (OutputVS)0;
	
	// Transform normal to world space.
	outVS.normalW = mul(float4(normalL, 0.0f), gWorldInvTrans).xyz;
	
	// Transform vertex position to world space.
	float3 posW  = mul(float4(posL, 1.0f), gWorld).xyz;
	
	// Compute the unit vector from the vertex to the eye.
	outVS.toEyeW = gEyePosW - posW;
	
	// Transform to homogeneous clip space.
	outVS.posH = mul(float4(posL, 1.0f), gWVP);
	
	// Pass on texture coordinates to be interpolated in rasterization.
	outVS.tex0 = tex0;
	
	// Compute vertex distance from camera in world space for fog calculation.
	float dist = distance(posW, gEyePosW);
	outVS.fogLerpParam = saturate((dist - gFogStart) / gFogRange);

	// Done--return the output.
    return outVS;
}

float4 PhongDirLtTexPS(float3 normalW : TEXCOORD0, 
                       float3 toEyeW  : TEXCOORD1, 
                       float2 tex0 : TEXCOORD2,
                       float  fogLerpParam : TEXCOORD3) : COLOR
{
	// Interpolated normals can become unnormal--so normalize.
	normalW = normalize(normalW);
	toEyeW  = normalize(toEyeW);
	
	// Light vector is opposite the direction of the light.
	float3 lightVecW = -gLight.dirW;
	
	// Compute the reflection vector.
	float3 r = reflect(-lightVecW, normalW);
	
	// Determine how much (if any) specular light makes it into the eye.
	float t  = pow(max(dot(r, toEyeW), 0.0f), gMtrl.specPower);
	
	// Determine the diffuse light intensity that strikes the vertex.
	float s = max(dot(lightVecW, normalW), 0.0f);
	
	// Compute the ambient, diffuse and specular terms separatly. 
	float3 spec = t*(gMtrl.spec*gLight.spec).rgb;
	float3 diffuse = s*(gMtrl.diffuse*gLight.diffuse).rgb;
	float3 ambient = gMtrl.ambient*gLight.ambient;
	
	// Get the texture color.
	float4 texColor = tex2D(TexS, tex0);
	
	// Combine the color from lighting with the texture color.
	float3 color = (ambient + diffuse)*texColor.rgb + spec;
	
	 // Add fog.
    float3 final = lerp(texColor, gFogColor, fogLerpParam);
		
	// Sum all the terms together and copy over the diffuse alpha.
    return float4(final, gMtrl.diffuse.a*texColor.a);
}

technique PhongDirLtTexTech
{
    pass P0
    {
        // Specify the vertex and pixel shader associated with this pass.
        vertexShader = compile vs_2_0 PhongDirLtTexVS();
        pixelShader  = compile ps_2_0 PhongDirLtTexPS();
    }
}