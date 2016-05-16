#include "TextureMgr.h"
#include <fstream>

using namespace std;

TextureMgr& GetTextureMgr()
{
	static TextureMgr tm;

	return tm;
}

TextureMgr::TextureMgr()
: md3dDevice(0), mRandomTexRV(0)
{
}

TextureMgr::~TextureMgr()
{
	for(size_t i = 0; i < mTextureRVs.size(); ++i)
		ReleaseCOM(mTextureRVs[i]);

	ReleaseCOM(mRandomTexRV);
}

void TextureMgr::init(ID3D10Device* device)
{
	md3dDevice = device;

	buildRandomTex();
}

void TextureMgr::dumpInfo()const
{
	wofstream fout("Textures.txt");

	fout << "Num Textures = " << mTextureRVs.size() << endl;
	for(size_t i = 0; i < mTextureNames.size(); ++i)
	{
		fout << mTextureNames[i] << endl;
	}
	fout << endl;
}

ID3D10ShaderResourceView* TextureMgr::getRandomTex()
{
	return mRandomTexRV;
}

ID3D10ShaderResourceView* TextureMgr::createTex(wstring filename)
{
	// Has this texture already been created?
	for(size_t i = 0; i < mTextureRVs.size(); ++i)
		if( mTextureNames[i] == filename )
			return mTextureRVs[i];

	// If not, create it.
	ID3D10ShaderResourceView* rv = 0;
	HR(D3DX10CreateShaderResourceViewFromFile(md3dDevice, filename.c_str(), 0, 0, &rv, 0 ));

	mTextureNames.push_back(filename);
	mTextureRVs.push_back(rv);

	return rv;
}

ID3D10ShaderResourceView* TextureMgr::createTexArray(wstring arrayName, const StringVector& filenames)
{
	//
	// Has this texture already been created?
	//
	for(size_t i = 0; i < mTextureRVs.size(); ++i)
		if( mTextureNames[i] == arrayName )
			return mTextureRVs[i];

	//
	// Load the texture elements individually from file.  These textures
	// won't be used by the GPU (0 bind flags), they are just used to 
	// load the image data from file.  We use the STAGING usage so the
	// CPU can read the resource.
	//

	UINT arraySize = (UINT)filenames.size();

	vector<ID3D10Texture2D*> srcTex(arraySize, 0);
	for(UINT i = 0; i < arraySize; ++i)
	{
		D3DX10_IMAGE_LOAD_INFO loadInfo;

        loadInfo.Width  = D3DX10_FROM_FILE;
        loadInfo.Height = D3DX10_FROM_FILE;
        loadInfo.Depth  = D3DX10_FROM_FILE;
        loadInfo.FirstMipLevel = 0;
        loadInfo.MipLevels = D3DX10_FROM_FILE;
        loadInfo.Usage = D3D10_USAGE_STAGING;
        loadInfo.BindFlags = 0;
        loadInfo.CpuAccessFlags = D3D10_CPU_ACCESS_WRITE | D3D10_CPU_ACCESS_READ;
        loadInfo.MiscFlags = 0;
        loadInfo.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        loadInfo.Filter = D3DX10_FILTER_NONE;
        loadInfo.MipFilter = D3DX10_FILTER_NONE;
		loadInfo.pSrcInfo  = 0;

        HR(D3DX10CreateTextureFromFile(md3dDevice, filenames[i].c_str(), 
			&loadInfo, 0, (ID3D10Resource**)&srcTex[i], 0));
	}

	//
	// Create the texture array.  Each element in the texture 
	// array has the same format/dimensions.
	//

	D3D10_TEXTURE2D_DESC texElementDesc;
	srcTex[0]->GetDesc(&texElementDesc);

	D3D10_TEXTURE2D_DESC texArrayDesc;
	texArrayDesc.Width              = texElementDesc.Width;
	texArrayDesc.Height             = texElementDesc.Height;
	texArrayDesc.MipLevels          = texElementDesc.MipLevels;
	texArrayDesc.ArraySize          = arraySize;
	texArrayDesc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
	texArrayDesc.SampleDesc.Count   = 1;
	texArrayDesc.SampleDesc.Quality = 0;
	texArrayDesc.Usage              = D3D10_USAGE_DEFAULT;
	texArrayDesc.BindFlags          = D3D10_BIND_SHADER_RESOURCE;
	texArrayDesc.CPUAccessFlags     = 0;
	texArrayDesc.MiscFlags          = 0;

	ID3D10Texture2D* texArray = 0;
	HR(md3dDevice->CreateTexture2D( &texArrayDesc, 0, &texArray));

	//
	// Copy individual texture elements into texture array.
	//

	// for each texture element...
	for(UINT i = 0; i < arraySize; ++i)
	{
		// for each mipmap level...
		for(UINT j = 0; j < texElementDesc.MipLevels; ++j)
		{
			D3D10_MAPPED_TEXTURE2D mappedTex2D;
			srcTex[i]->Map(j, D3D10_MAP_READ, 0, &mappedTex2D);
                    
            md3dDevice->UpdateSubresource(texArray, 
				D3D10CalcSubresource(j, i, texElementDesc.MipLevels),
                0, mappedTex2D.pData, mappedTex2D.RowPitch, 0);

            srcTex[i]->Unmap(j);
		}
	}	

	//
	// Create a resource view to the texture array.
	//
	
	D3D10_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texArrayDesc.Format;
	viewDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2DARRAY;
	viewDesc.Texture2DArray.MostDetailedMip = 0;
	viewDesc.Texture2DArray.MipLevels = texArrayDesc.MipLevels;
	viewDesc.Texture2DArray.FirstArraySlice = 0;
	viewDesc.Texture2DArray.ArraySize = arraySize;

	ID3D10ShaderResourceView* texArrayRV = 0;
	HR(md3dDevice->CreateShaderResourceView(texArray, &viewDesc, &texArrayRV));

	//
	// Cleanup--we only need the resource view.
	//

	ReleaseCOM(texArray);

	for(UINT i = 0; i < arraySize; ++i)
		ReleaseCOM(srcTex[i]); 

	mTextureNames.push_back(arrayName);
	mTextureRVs.push_back(texArrayRV);

	return texArrayRV;
}

ID3D10ShaderResourceView* TextureMgr::createCubeTex(std::wstring filename)
{
	// Has this texture already been created?
	for(size_t i = 0; i < mTextureRVs.size(); ++i)
		if( mTextureNames[i] == filename )
			return mTextureRVs[i];

	// If not, create it.
	D3DX10_IMAGE_LOAD_INFO loadInfo;
    loadInfo.MiscFlags = D3D10_RESOURCE_MISC_TEXTURECUBE;

	ID3D10Texture2D* tex = 0;
	HR(D3DX10CreateTextureFromFile(md3dDevice, filename.c_str(), 
		&loadInfo, 0, (ID3D10Resource**)&tex, 0) );

    D3D10_TEXTURE2D_DESC texDesc;
	tex->GetDesc(&texDesc);

    D3D10_SHADER_RESOURCE_VIEW_DESC viewDesc;
    viewDesc.Format = texDesc.Format;
    viewDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURECUBE;
    viewDesc.TextureCube.MipLevels = texDesc.MipLevels;
    viewDesc.TextureCube.MostDetailedMip = 0;
    
	ID3D10ShaderResourceView* rv = 0;
	HR(md3dDevice->CreateShaderResourceView(tex, &viewDesc, &rv));
   
	ReleaseCOM(tex);

	mTextureNames.push_back(filename);
	mTextureRVs.push_back(rv);

	return rv;
}

void TextureMgr::buildRandomTex()
{
	// 
	// Create the random data.
	//
	D3DXVECTOR4 randomValues[1024];

	for(int i = 0; i < 1024; ++i)
	{
		randomValues[i].x = RandF(-1.0f, 1.0f);
		randomValues[i].y = RandF(-1.0f, 1.0f);
		randomValues[i].z = RandF(-1.0f, 1.0f);
		randomValues[i].w = RandF(-1.0f, 1.0f);
	}

    D3D10_SUBRESOURCE_DATA initData;
    initData.pSysMem = randomValues;
	initData.SysMemPitch = 1024*sizeof(D3DXVECTOR4);
    initData.SysMemSlicePitch = 1024*sizeof(D3DXVECTOR4);
	//
	// Create the texture.
	//
    D3D10_TEXTURE1D_DESC texDesc;
    texDesc.Width = 1024;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    texDesc.Usage = D3D10_USAGE_IMMUTABLE;
    texDesc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;
    texDesc.ArraySize = 1;

	ID3D10Texture1D* randomTex = 0;
    HR(md3dDevice->CreateTexture1D(&texDesc, &initData, &randomTex));
	//
	// Create the resource view.
	//
    D3D10_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texDesc.Format;
    viewDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE1D;
    viewDesc.Texture1D.MipLevels = texDesc.MipLevels;
	viewDesc.Texture1D.MostDetailedMip = 0;
	
    HR(md3dDevice->CreateShaderResourceView(randomTex, &viewDesc, &mRandomTexRV));

	ReleaseCOM(randomTex);
}