//=============================================================================
// heightcolor.fx by Frank Luna (C) 2004 All Rights Reserved.
//
// Animates vertices in a wave motion and generates vertex colors 
// based on their height.
//=============================================================================

uniform extern float4x4 gWVP;
uniform extern float gTime;

struct OutputVS
{
    float4 posH  : POSITION0;
    float4 color : COLOR0;
};

// Amplitudes
static float a[2] = {0.8f, 0.2f};
	
// Angular wave numbers.
static float k[2] = {1.0, 8.0f};
	
// Angular frequency.
static float w[2] = {1.0f, 8.0f};
	
// Phase shifts.
static float p[2] = {0.0f, 1.0f};

float SumOfRadialSineWaves(float x, float z)
{
	// Distance of vertex from source of waves (which we set
	// as the origin of the local space).
	float d = sqrt(x*x + z*z);
	
	// Sum the waves.
	float sum = 0.0f;
	for(int i = 0; i < 2; ++i)
		sum += a[i]*sin(k[i]*d - gTime*w[i] + p[i]);
		
	return sum;
}

float4 GetColorFromHeight(float y)
{
	if( abs(y) <= 0.2f ) // black
		return float4(0.0f, 0.0f, 0.0f, 1.0f);
	else if(abs(y) <= 0.5f ) // blue
		return float4(0.0f, 0.0f, 1.0f, 1.0f);
	else if(abs(y) <= 0.8f ) // green
		return float4(0.0f, 1.0f, 0.0f, 1.0f);
	else if(abs(y) <= 1.0f ) // red
		return float4(1.0f, 0.0f, 0.0f, 1.0f);
	else // yellow
		return float4(1.0f, 1.0f, 0.0f, 1.0f);
}

OutputVS ColorVS(float3 posL : POSITION0)
{
    // Zero out our output.
	OutputVS outVS = (OutputVS)0;
	
	// Get the height of the vertex--the height is given by
	// summing sine waves.
	posL.y = SumOfRadialSineWaves(posL.x, posL.z);
	
	// Generate the vertex color based on its height.
	outVS.color = GetColorFromHeight(posL.y);
	
	// Transform to homogeneous clip space.
	outVS.posH = mul(float4(posL, 1.0f), gWVP);
	 
	// Done--return the output.
    return outVS;
}

float4 ColorPS(float4 c : COLOR0) : COLOR
{
    return c;
}

technique HeightColorTech
{
    pass P0
    {
        // Specify the vertex and pixel shader associated with this pass.
        vertexShader = compile vs_2_0 ColorVS();
        pixelShader  = compile ps_2_0 ColorPS();

		// Specify the render/device states associated with this pass.
		FillMode = WireFrame;
    }
}
