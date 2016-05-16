#include "Terrain.h"
#include "TextureMgr.h"
#include "Camera.h"
#include "Effects.h"
#include "InputLayouts.h"
#include <fstream>
#include <sstream>

namespace 
{
	struct TerrainVertex
	{
		D3DXVECTOR3 pos;
		D3DXVECTOR3 normal;
		D3DXVECTOR2 texC;
	};
}

Terrain::Terrain()
: md3dDevice(0), mVB(0), mIB(0), mLayer0(0), mLayer1(0), 
  mLayer2(0), mLayer3(0), mLayer4(0), mBlendMap(0)
{
}

Terrain::~Terrain()
{
	ReleaseCOM(mVB);
	ReleaseCOM(mIB);
}

float Terrain::width()const
{
	return (mInfo.NumCols-1)*mInfo.CellSpacing;
}

float Terrain::depth()const
{
	return (mInfo.NumRows-1)*mInfo.CellSpacing;
}

float Terrain::getHeight(float x, float z)const
{
	// Transform from terrain local space to "cell" space.
	float c = (x + 0.5f*width()) /  mInfo.CellSpacing;
	float d = (z - 0.5f*depth()) / -mInfo.CellSpacing;

	// Get the row and column we are in.
	int row = (int)floorf(d);
	int col = (int)floorf(c);

	// Grab the heights of the cell we are in.
	// A*--*B
	//  | /|
	//  |/ |
	// C*--*D
	float A = mHeightmap[row*mInfo.NumCols + col];
	float B = mHeightmap[row*mInfo.NumCols + col + 1];
	float C = mHeightmap[(row+1)*mInfo.NumCols + col];
	float D = mHeightmap[(row+1)*mInfo.NumCols + col + 1];

	// Where we are relative to the cell.
	float s = c - (float)col;
	float t = d - (float)row;

	// If upper triangle ABC.
	if( s + t <= 1.0f)
	{
		float uy = B - A;
		float vy = C - A;
		return A + s*uy + t*vy;
	}
	else // lower triangle DCB.
	{
		float uy = C - D;
		float vy = B - D;
		return D + (1.0f-s)*uy + (1.0f-t)*vy;
	}
}

void Terrain::init(ID3D10Device* device, const InitInfo& initInfo)
{
	md3dDevice = device;

	mTech          = fx::TerrainFX->GetTechniqueByName("TerrainTech");
	mfxWVPVar      = fx::TerrainFX->GetVariableByName("gWVP")->AsMatrix();
	mfxWorldVar    = fx::TerrainFX->GetVariableByName("gWorld")->AsMatrix();
	mfxDirToSunVar = fx::TerrainFX->GetVariableByName("gDirToSunW")->AsVector();
	mfxLayer0Var   = fx::TerrainFX->GetVariableByName("gLayer0")->AsShaderResource();
	mfxLayer1Var   = fx::TerrainFX->GetVariableByName("gLayer1")->AsShaderResource();
	mfxLayer2Var   = fx::TerrainFX->GetVariableByName("gLayer2")->AsShaderResource();
	mfxLayer3Var   = fx::TerrainFX->GetVariableByName("gLayer3")->AsShaderResource();
	mfxLayer4Var   = fx::TerrainFX->GetVariableByName("gLayer4")->AsShaderResource();
	mfxBlendMapVar = fx::TerrainFX->GetVariableByName("gBlendMap")->AsShaderResource();

	mInfo = initInfo;

	mNumVertices = mInfo.NumRows*mInfo.NumCols;
	mNumFaces    = (mInfo.NumRows-1)*(mInfo.NumCols-1)*2;

	loadHeightmap();
	smooth();

	buildVB();
	buildIB();

	mLayer0   = GetTextureMgr().createTex(initInfo.LayerMapFilename0);
	mLayer1   = GetTextureMgr().createTex(initInfo.LayerMapFilename1);
	mLayer2   = GetTextureMgr().createTex(initInfo.LayerMapFilename2);
	mLayer3   = GetTextureMgr().createTex(initInfo.LayerMapFilename3);
	mLayer4   = GetTextureMgr().createTex(initInfo.LayerMapFilename4);
	mBlendMap = GetTextureMgr().createTex(initInfo.BlendMapFilename);
}

void Terrain::setDirectionToSun(const D3DXVECTOR3& v)
{
	D3DXVECTOR4 temp(v.x, v.y, v.z, 0.0f);
	mfxDirToSunVar->SetFloatVector((float*)temp);
}

void Terrain::draw(const D3DXMATRIX& world)
{
	md3dDevice->IASetInputLayout(InputLayout::PosNormalTex);

	UINT stride = sizeof(TerrainVertex);
    UINT offset = 0;
    md3dDevice->IASetVertexBuffers(0, 1, &mVB, &stride, &offset);
	md3dDevice->IASetIndexBuffer(mIB, DXGI_FORMAT_R32_UINT, 0);

	D3DXMATRIX view = GetCamera().view();
	D3DXMATRIX proj = GetCamera().proj();

	D3DXMATRIX WVP = world*view*proj;


	mfxWVPVar->SetMatrix((float*)&WVP);
	mfxWorldVar->SetMatrix((float*)&world);

	mfxLayer0Var->SetResource(mLayer0);
	mfxLayer1Var->SetResource(mLayer1);
	mfxLayer2Var->SetResource(mLayer2);
	mfxLayer3Var->SetResource(mLayer3);
	mfxLayer4Var->SetResource(mLayer4);
	mfxBlendMapVar->SetResource(mBlendMap);

    D3D10_TECHNIQUE_DESC techDesc;
    mTech->GetDesc( &techDesc );

    for(UINT i = 0; i < techDesc.Passes; ++i)
    {
        ID3D10EffectPass* pass = mTech->GetPassByIndex(i);
		pass->Apply(0);

		md3dDevice->DrawIndexed(mNumFaces*3, 0, 0);
	}	
}

void Terrain::loadHeightmap()
{
	// A height for each vertex
	std::vector<unsigned char> in( mInfo.NumRows * mInfo.NumCols );

	// Open the file.
	std::ifstream inFile;
	inFile.open(mInfo.HeightmapFilename.c_str(), std::ios_base::binary);

	if(inFile)
	{
		// Read the RAW bytes.
		inFile.read((char*)&in[0], (std::streamsize)in.size());

		// Done with file.
		inFile.close();
	}

	// Copy the array data into a float array, and scale and offset the heights.
	mHeightmap.resize(mInfo.NumRows * mInfo.NumCols, 0);
	for(UINT i = 0; i < mInfo.NumRows * mInfo.NumCols; ++i)
	{
		mHeightmap[i] = (float)in[i] * mInfo.HeightScale + mInfo.HeightOffset;
	}
}

void Terrain::smooth()
{
	std::vector<float> dest( mHeightmap.size() );

	for(UINT i = 0; i < mInfo.NumRows; ++i)
	{
		for(UINT j = 0; j < mInfo.NumCols; ++j)
		{
			dest[i*mInfo.NumCols+j] = average(i,j);
		}
	}

	// Replace the old heightmap with the filtered one.
	mHeightmap = dest;
}

bool Terrain::inBounds(UINT i, UINT j)
{
	// True if ij are valid indices; false otherwise.
	return 
		i >= 0 && i < mInfo.NumRows && 
		j >= 0 && j < mInfo.NumCols;
}

float Terrain::average(UINT i, UINT j)
{
	// Function computes the average height of the ij element.
	// It averages itself with its eight neighbor pixels.  Note
	// that if a pixel is missing neighbor, we just don't include it
	// in the average--that is, edge pixels don't have a neighbor pixel.
	//
	// ----------
	// | 1| 2| 3|
	// ----------
	// |4 |ij| 6|
	// ----------
	// | 7| 8| 9|
	// ----------

	float avg = 0.0f;
	float num = 0.0f;

	for(UINT m = i-1; m <= i+1; ++m)
	{
		for(UINT n = j-1; n <= j+1; ++n)
		{
			if( inBounds(m,n) )
			{
				avg += mHeightmap[m*mInfo.NumCols + n];
				num += 1.0f;
			}
		}
	}

	return avg / num;
}

void Terrain::buildVB()
{
	std::vector<TerrainVertex> vertices(mNumVertices);

	float halfWidth = (mInfo.NumCols-1)*mInfo.CellSpacing*0.5f;
	float halfDepth = (mInfo.NumRows-1)*mInfo.CellSpacing*0.5f;

	float du = 1.0f / (mInfo.NumCols-1);
	float dv = 1.0f / (mInfo.NumRows-1);
	for(UINT i = 0; i < mInfo.NumRows; ++i)
	{
		float z = halfDepth - i*mInfo.CellSpacing;
		for(UINT j = 0; j < mInfo.NumCols; ++j)
		{
			float x = -halfWidth + j*mInfo.CellSpacing;

			float y = mHeightmap[i*mInfo.NumCols+j];
			vertices[i*mInfo.NumCols+j].pos    = D3DXVECTOR3(x, y, z);
			vertices[i*mInfo.NumCols+j].normal = D3DXVECTOR3(0.0f, 1.0f, 0.0f);

			// Stretch texture over grid.
			vertices[i*mInfo.NumCols+j].texC.x = j*du;
			vertices[i*mInfo.NumCols+j].texC.y = i*dv;
		}
	}
 
	// Estimate normals for interior nodes using central difference.
	float invTwoDX = 1.0f / (2.0f*mInfo.CellSpacing);
	float invTwoDZ = 1.0f / (2.0f*mInfo.CellSpacing);
	for(UINT i = 2; i < mInfo.NumRows-1; ++i)
	{
		for(UINT j = 2; j < mInfo.NumCols-1; ++j)
		{
			float t = mHeightmap[(i-1)*mInfo.NumCols + j];
			float b = mHeightmap[(i+1)*mInfo.NumCols + j];
			float l = mHeightmap[i*mInfo.NumCols + j - 1];
			float r = mHeightmap[i*mInfo.NumCols + j + 1];

			D3DXVECTOR3 tanZ(0.0f, (t-b)*invTwoDZ, 1.0f);
			D3DXVECTOR3 tanX(1.0f, (r-l)*invTwoDX, 0.0f);

			D3DXVECTOR3 N;
			D3DXVec3Cross(&N, &tanZ, &tanX);
			D3DXVec3Normalize(&N, &N);

			vertices[i*mInfo.NumCols+j].normal = N;
		}
	}

    D3D10_BUFFER_DESC vbd;
    vbd.Usage = D3D10_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(TerrainVertex) * mNumVertices;
    vbd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
	D3D10_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = &vertices[0];
    HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mVB));
}

void Terrain::buildIB()
{
	std::vector<DWORD> indices(mNumFaces*3); // 3 indices per face

	// Iterate over each quad and compute indices.
	int k = 0;
	for(UINT i = 0; i < mInfo.NumRows-1; ++i)
	{
		for(UINT j = 0; j < mInfo.NumCols-1; ++j)
		{
			indices[k]   = i*mInfo.NumCols+j;
			indices[k+1] = i*mInfo.NumCols+j+1;
			indices[k+2] = (i+1)*mInfo.NumCols+j;

			indices[k+3] = (i+1)*mInfo.NumCols+j;
			indices[k+4] = i*mInfo.NumCols+j+1;
			indices[k+5] = (i+1)*mInfo.NumCols+j+1;

			k += 6; // next quad
		}
	}

	D3D10_BUFFER_DESC ibd;
    ibd.Usage = D3D10_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof(DWORD) * mNumFaces*3;
    ibd.BindFlags = D3D10_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D10_SUBRESOURCE_DATA iinitData;
    iinitData.pSysMem = &indices[0];
    HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mIB));
}