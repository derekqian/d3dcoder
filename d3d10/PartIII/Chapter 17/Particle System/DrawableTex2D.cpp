#include "DrawableTex2D.h"

DrawableTex2D::DrawableTex2D()
: mWidth(0), mHeight(0), mColorMapFormat(DXGI_FORMAT_UNKNOWN), 
  md3dDevice(0), mColorMapSRV(0), mColorMapRTV(0), mDepthMapSRV(0), mDepthMapDSV(0)
{
	ZeroMemory(&mViewport, sizeof(D3D10_VIEWPORT));
}

DrawableTex2D::~DrawableTex2D()
{
	ReleaseCOM(mColorMapSRV);
	ReleaseCOM(mColorMapRTV);

	ReleaseCOM(mDepthMapSRV);
	ReleaseCOM(mDepthMapDSV);
}

void DrawableTex2D::init(ID3D10Device* device, UINT width, UINT height, bool hasColorMap, 
		                 DXGI_FORMAT colorFormat)
{
	mWidth  = width;
	mHeight = height;

	mColorMapFormat = colorFormat;

	md3dDevice = device;

	buildDepthMap();

	// shadow maps don't need color maps, for example
	if( hasColorMap )
		buildColorMap();
 
	mViewport.TopLeftX = 0;
	mViewport.TopLeftY = 0;
	mViewport.Width    = width;
	mViewport.Height   = height;
	mViewport.MinDepth = 0.0f;
	mViewport.MaxDepth = 1.0f;
}

ID3D10ShaderResourceView* DrawableTex2D::colorMap()
{
	return mColorMapSRV;
}

ID3D10ShaderResourceView* DrawableTex2D::depthMap()
{
	return mDepthMapSRV;
}

void DrawableTex2D::begin()
{
	ID3D10RenderTargetView* renderTargets[1] = {mColorMapRTV};
	md3dDevice->OMSetRenderTargets(1, renderTargets, mDepthMapDSV);

	md3dDevice->RSSetViewports(1, &mViewport);

	// only clear is we actually created a color map.
	if( mColorMapRTV )
		md3dDevice->ClearRenderTargetView(mColorMapRTV, BLACK);

	md3dDevice->ClearDepthStencilView(mDepthMapDSV, D3D10_CLEAR_DEPTH, 1.0f, 0);
}

void DrawableTex2D::end()
{
	// After we have drawn to the color map, have the hardware generate
	// the lower mipmap levels.

	if( mColorMapSRV )
		md3dDevice->GenerateMips(mColorMapSRV);
}	

void DrawableTex2D::buildDepthMap()
{
	ID3D10Texture2D* depthMap = 0;

	D3D10_TEXTURE2D_DESC texDesc;
	
	texDesc.Width     = mWidth;
	texDesc.Height    = mHeight;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format    = DXGI_FORMAT_R32_TYPELESS;
	texDesc.SampleDesc.Count   = 1;  
	texDesc.SampleDesc.Quality = 0;  
	texDesc.Usage          = D3D10_USAGE_DEFAULT;
	texDesc.BindFlags      = D3D10_BIND_DEPTH_STENCIL | D3D10_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0; 
	texDesc.MiscFlags      = 0;

	HR(md3dDevice->CreateTexture2D(&texDesc, 0, &depthMap));

	D3D10_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	HR(md3dDevice->CreateDepthStencilView(depthMap, &dsvDesc, &mDepthMapDSV));


	D3D10_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;
	HR(md3dDevice->CreateShaderResourceView(depthMap, &srvDesc, &mDepthMapSRV));

	// View saves a reference to the texture so we can release our reference.
	ReleaseCOM(depthMap);
}

void DrawableTex2D::buildColorMap()
{
	ID3D10Texture2D* colorMap = 0;

	D3D10_TEXTURE2D_DESC texDesc;
	
	texDesc.Width     = mWidth;
	texDesc.Height    = mHeight;
	texDesc.MipLevels = 0;
	texDesc.ArraySize = 1;
	texDesc.Format    = mColorMapFormat;
	texDesc.SampleDesc.Count   = 1;  
	texDesc.SampleDesc.Quality = 0;  
	texDesc.Usage          = D3D10_USAGE_DEFAULT;
	texDesc.BindFlags      = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0; 
	texDesc.MiscFlags      = D3D10_RESOURCE_MISC_GENERATE_MIPS;

	HR(md3dDevice->CreateTexture2D(&texDesc, 0, &colorMap));

	// Null description means to create a view to all mipmap levels using 
	// the format the texture was created with.
	HR(md3dDevice->CreateRenderTargetView(colorMap, 0, &mColorMapRTV));
	HR(md3dDevice->CreateShaderResourceView(colorMap, 0, &mColorMapSRV));

	// View saves a reference to the texture so we can release our reference.
	ReleaseCOM(colorMap);
}