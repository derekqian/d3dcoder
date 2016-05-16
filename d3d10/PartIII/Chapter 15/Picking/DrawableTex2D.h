#ifndef DRAWABLETEX2D_H
#define DRAWABLETEX2D_H

#include "d3dUtil.h"

class DrawableTex2D
{
public:
	DrawableTex2D();
	~DrawableTex2D();

	void init(ID3D10Device* device, UINT width, UINT height, bool hasColorMap,
		DXGI_FORMAT colorFormat);

	ID3D10ShaderResourceView* colorMap();
	ID3D10ShaderResourceView* depthMap();

	void begin();
	void end();

private:
	DrawableTex2D(const DrawableTex2D& rhs);
	DrawableTex2D& operator=(const DrawableTex2D& rhs);

	void buildDepthMap();
	void buildColorMap();
private:
	UINT mWidth;
	UINT mHeight;
	DXGI_FORMAT mColorMapFormat;

	ID3D10Device* md3dDevice;

	ID3D10ShaderResourceView* mColorMapSRV;
	ID3D10RenderTargetView* mColorMapRTV;

	ID3D10ShaderResourceView* mDepthMapSRV;
	ID3D10DepthStencilView* mDepthMapDSV;

	D3D10_VIEWPORT mViewport;
};

#endif // DRAWABLETEX2D_H