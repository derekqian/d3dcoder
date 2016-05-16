//=============================================================================
// ProjTex.fx by Frank Luna (C) 2004 All Rights Reserved.
//
// Projective texturing.
//=============================================================================

struct Mtrl
{
	float4 ambient;
	float4 diffuse;
	float4 spec;
	float  specPower;
};

struct SpotLight
{
	float4 ambient;
	float4 diffuse;
	float4 spec;
	float3 posW;
	float3 dirW;  
	float  spotPower;
};

uniform extern float4x4  gWorld;
uniform extern float4x4  gWorldInvTrans;
uniform extern float4x4  gLightWVP;
uniform extern float4x4  gWVP;
uniform extern Mtrl      gMtrl;
uniform extern SpotLight gLight;
uniform extern float3    gEyePosW;
uniform extern texture   gTex;


sampler TexS = sampler_state
{
	Texture = <gTex>;
	MinFilter = Anisotropic;
	MaxAnisotropy = 8;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU  = CLAMP; 
    AddressV  = CLAMP;
};

struct OutputVS
{
    float4 posH     : POSITION0;
    float3 posW     : TEXCOORD0;
    float3 normalW  : TEXCOORD1;
    float3 toEyeW   : TEXCOORD2;
    float4 projTex  : TEXCOORD3;
};

OutputVS ProjTexVS(float3 posL : POSITION0, float3 normalL : NORMAL0)
{
    // Zero out our output.
	OutputVS outVS = (OutputVS)0;
	
	// Transform normal to world space.
	outVS.normalW = mul(float4(normalL, 0.0f), gWorldInvTrans).xyz;
	
	// Transform vertex position to world space.
	outVS.posW  = mul(float4(posL, 1.0f), gWorld).xyz;
	
	// Compute the unit vector from the vertex to the eye.
	outVS.toEyeW = gEyePosW - outVS.posW;
	
	// Transform to homogeneous clip space.
	outVS.posH = mul(float4(posL, 1.0f), gWVP);
	
	// Render from light source to generate projective texture coordinates.
	outVS.projTex = mul(float4(posL, 1.0f), gLightWVP);
		
	// Done--return the output.
    return outVS;
}

float4 ProjTexPS(float3 posW     : TEXCOORD0,
                 float3 normalW  : TEXCOORD1,
                 float3 toEyeW   : TEXCOORD2,
                 float4 projTex  : TEXCOORD3) : COLOR
{
	// Interpolated normals can become unnormal--so normalize.
	normalW = normalize(normalW);
	toEyeW  = normalize(toEyeW);
	
	// Light vector is from pixel to spotlight position.
	float3 lightVecW = normalize(gLight.posW - posW);
	
	// Compute the reflection vector.
	float3 r = reflect(-lightVecW, normalW);
	
	// Determine how much (if any) specular light makes it into the eye.
	float t  = pow(max(dot(r, toEyeW), 0.0f), gMtrl.specPower);
	
	// Determine the diffuse light intensity that strikes the vertex.
	float s = max(dot(lightVecW, normalW), 0.0f);
	
	// Compute the ambient, diffuse and specular terms separately. 
	float3 spec = t*(gMtrl.spec*gLight.spec).rgb;
	float3 diffuse = s*(gMtrl.diffuse*gLight.diffuse.rgb);
	float3 ambient = gMtrl.ambient*gLight.ambient;
	
	// Compute spotlight coefficient.
	float spot = pow(max( dot(-lightVecW, gLight.dirW), 0.0f), gLight.spotPower);
	
	// Project the texture coords and scale/offset to [0, 1].
	projTex.xy /= projTex.w;            
	projTex.x =  0.5f*projTex.x + 0.5f; 
	projTex.y = -0.5f*projTex.y + 0.5f;

	// Sample tex w/ projective texture coords.
	float4 texColor = tex2D(TexS, projTex.xy); 
	
	// Only project/light in spotlight cone.
	float3 litColor = spot*(ambient+diffuse*texColor.rgb + spec);
	
	// Output the color and the alpha.
    return float4(litColor, gMtrl.diffuse.a*texColor.a);
}

technique ProjTexTech
{
    pass P0
    {
        // Specify the vertex and pixel shader associated with this pass.
        vertexShader = compile vs_2_0 ProjTexVS();
        pixelShader  = compile ps_2_0 ProjTexPS();
    }
}