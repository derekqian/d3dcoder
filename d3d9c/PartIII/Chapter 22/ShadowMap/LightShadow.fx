//=============================================================================
// LightShadow.fx by Frank Luna (C) 2004 All Rights Reserved.
//
// Does lighting and shadowing with shadow maps.
//=============================================================================

uniform extern float4x4 gLightWVP;

static const float SHADOW_EPSILON = 0.00005f;
static const float SMAP_SIZE = 512.0f;

void BuildShadowMapVS(float3 posL : POSITION0,
                      out float4 posH : POSITION0,
                      out float2 depth : TEXCOORD0)
{
	// Render from light's perspective.
	posH = mul(float4(posL, 1.0f), gLightWVP);
	
	// Propagate z- and w-coordinates.
	depth = posH.zw;
}

float4 BuildShadowMapPS(float2 depth : TEXCOORD0) : COLOR
{
	// Each pixel in the shadow map stores the pixel depth from the 
	// light source in normalized device coordinates.
	return depth.x / depth.y; // z / w
}

technique BuildShadowMapTech
{
	pass P0
	{
		vertexShader = compile vs_2_0 BuildShadowMapVS();
        pixelShader  = compile ps_2_0 BuildShadowMapPS();
	}
}

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
uniform extern float4x4  gWVP;
uniform extern Mtrl      gMtrl;
uniform extern SpotLight gLight;
uniform extern float3    gEyePosW;
uniform extern texture   gTex;
uniform extern texture   gShadowMap;
 
sampler TexS = sampler_state
{
	Texture = <gTex>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU  = WRAP; 
    AddressV  = WRAP;
};

sampler ShadowMapS = sampler_state
{
	Texture = <gShadowMap>;
	MinFilter = POINT;
	MagFilter = POINT;
	MipFilter = POINT;
	AddressU  = CLAMP; 
    AddressV  = CLAMP;
};

void LightShadowVS(float3 posL         : POSITION0,
                   float3 normalL      : NORMAL0,
                   float2 tex0         : TEXCOORD0,
                   out float4 oPosH    : POSITION0,
                   out float3 oPosW    : TEXCOORD0,
                   out float3 oNormalW : TEXCOORD1,
                   out float3 oToEyeW  : TEXCOORD2,
                   out float2 oTex0    : TEXCOORD3,
                   out float4 oProjTex : TEXCOORD4)
{
	// Transform to homogeneous clip space.
	oPosH = mul(float4(posL, 1.0f), gWVP);
	
	// Transform vertex position to world space.
	oPosW = mul(float4(posL, 1.0f), gWorld).xyz;
	
	// Transform normal to world space (assume no non-uniform scaling).
	oNormalW = mul(float4(normalL, 0.0f), gWorld).xyz;
	
	// Compute the unit vector from the vertex to the eye.
	oToEyeW = gEyePosW - oPosW;
	
	// Pass on texture coords to PS
	oTex0 = tex0;
	
	// Generate projective texture coordinates.
	oProjTex = mul(float4(posL, 1.0f), gLightWVP);
}

float4 LightShadowPS(float3 posW    : TEXCOORD0,
                     float3 normalW : TEXCOORD1,
                     float3 toEyeW  : TEXCOORD2,
                     float2 tex0    : TEXCOORD3,
                     float4 projTex : TEXCOORD4) : COLOR
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
	
	// Sample decal map.
	float4 texColor = tex2D(TexS, tex0); 
	
	// Project the texture coords and scale/offset to [0, 1].
	projTex.xy /= projTex.w;            
	projTex.x =  0.5f*projTex.x + 0.5f; 
	projTex.y = -0.5f*projTex.y + 0.5f;
	
	// Compute pixel depth for shadowing.
	float depth = projTex.z / projTex.w;
 
	// Transform to texel space
    float2 texelpos = SMAP_SIZE * projTex.xy;
        
    // Determine the lerp amounts.           
    float2 lerps = frac( texelpos );
    
    // 2x2 percentage closest filter.
    float dx = 1.0f / SMAP_SIZE;
	float s0 = (tex2D(ShadowMapS, projTex.xy).r + SHADOW_EPSILON < depth) ? 0.0f : 1.0f;
	float s1 = (tex2D(ShadowMapS, projTex.xy + float2(dx, 0.0f)).r + SHADOW_EPSILON < depth) ? 0.0f : 1.0f;
	float s2 = (tex2D(ShadowMapS, projTex.xy + float2(0.0f, dx)).r + SHADOW_EPSILON < depth) ? 0.0f : 1.0f;
	float s3 = (tex2D(ShadowMapS, projTex.xy + float2(dx, dx)).r   + SHADOW_EPSILON < depth) ? 0.0f : 1.0f;
	
	float shadowCoeff = lerp( lerp( s0, s1, lerps.x ),
                              lerp( s2, s3, lerps.x ),
                              lerps.y );
	
	// Light/Texture pixel.  Note that shadow coefficient only affects diffuse/spec.
	float3 litColor = spot*ambient*texColor.rgb + spot*shadowCoeff*(diffuse*texColor.rgb + spec);
	
	return float4(litColor, gMtrl.diffuse.a*texColor.a);
}

technique LightShadowTech
{
    pass P0
    {
        // Specify the vertex and pixel shader associated with this pass.
        vertexShader = compile vs_2_0 LightShadowVS();
        pixelShader  = compile ps_2_0 LightShadowPS();
    }
}
