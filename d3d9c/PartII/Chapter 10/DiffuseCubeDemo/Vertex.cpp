//=============================================================================
// Vertex.cpp by Frank Luna (C) 2005 All Rights Reserved.
//=============================================================================

#include "Vertex.h"
#include "d3dUtil.h"

// Initialize static variables.
IDirect3DVertexDeclaration9* VertexPos::Decl = 0;
IDirect3DVertexDeclaration9* VertexCol::Decl = 0;
IDirect3DVertexDeclaration9* VertexPN::Decl  = 0;


void InitAllVertexDeclarations()
{
	//===============================================================
	// VertexPos

	D3DVERTEXELEMENT9 VertexPosElements[] = 
	{
		{0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		D3DDECL_END()
	};	
	HR(gd3dDevice->CreateVertexDeclaration(VertexPosElements, &VertexPos::Decl));

	//===============================================================
	// VertexCol

	D3DVERTEXELEMENT9 VertexColElements[] = 
	{
		{0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
		D3DDECL_END()
	};	
	HR(gd3dDevice->CreateVertexDeclaration(VertexColElements, &VertexCol::Decl));

	//===============================================================
	// VertexPN

	D3DVERTEXELEMENT9 VertexPNElements[] = 
	{
		{0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
		D3DDECL_END()
	};	
	HR(gd3dDevice->CreateVertexDeclaration(VertexPNElements, &VertexPN::Decl));
}

void DestroyAllVertexDeclarations()
{
	ReleaseCOM(VertexPos::Decl);
	ReleaseCOM(VertexCol::Decl);
	ReleaseCOM(VertexPN::Decl);
}