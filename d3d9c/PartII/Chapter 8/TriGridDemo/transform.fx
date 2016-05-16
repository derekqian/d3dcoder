//=============================================================================
// transform.fx by Frank Luna (C) 2004 All Rights Reserved.
//
// Basic FX that simply transforms geometry from local space to 
// homogeneous clip space, and draws the geometry in wireframe mode.
//=============================================================================

uniform extern float4x4 gWVP;
	
// Define a vertex shader output structure; that is, a structure
// that defines the data we output from the vertex shader.  Here,
// we only output a 4D vector in homogeneous clip space.  The
// semantic ": POSITION0" tells Direct3D that the data returned
// in this data member is a vertex position.
struct OutputVS
{
    float4 posH : POSITION0;
};

// Define the vertex shader program.  The parameter posL 
// corresponds to a data member in the vertex structure.
// Specifically, it corresponds to the data member in the 
// vertex structure with usage D3DDECLUSAGE_POSITION and 
// index 0 (as specified by the vertex declaration).
OutputVS TransformVS(float3 posL : POSITION0)
{
    // Zero out our output.
	OutputVS outVS = (OutputVS)0;
	
	// Transform to homogeneous clip space.
	outVS.posH = mul(float4(posL, 1.0f), gWVP);
	 
	// Done--return the output.
    return outVS;
}

// Define the pixel shader program.  Just return a 4D color
// vector (i.e., first component red, second component green,
// third component blue, fourth component alpha).  Here we
// specify black to color the lines black. 
float4 TransformPS() : COLOR
{
    return float4(0.0f, 0.0f, 0.0f, 1.0f);
}

technique TransformTech
{
    pass P0
    {
        // Specify the vertex and pixel shader associated with this pass.
        vertexShader = compile vs_2_0 TransformVS();
        pixelShader  = compile ps_2_0 TransformPS();

		// Specify the render/device states associated with this pass.
		FillMode = Wireframe;
    }
}
