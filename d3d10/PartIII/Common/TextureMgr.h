#ifndef TEXTUREMGR_H
#define TEXTUREMGR_H

#include "d3dUtil.h"
#include <vector>
#include <string>

class TextureMgr
{
public:
	friend TextureMgr& GetTextureMgr();
	typedef std::vector<std::wstring> StringVector;
 
	void init(ID3D10Device* device);

	// for debug
	void dumpInfo()const;

	ID3D10ShaderResourceView* getRandomTex();
	ID3D10ShaderResourceView* createTex(std::wstring filename);
	ID3D10ShaderResourceView* createTexArray(
		std::wstring arrayName, 
		const std::vector<std::wstring>& filenames);

	// .dds files can store cube textures in one file
	ID3D10ShaderResourceView* createCubeTex(std::wstring filename);


private:
	TextureMgr();
	TextureMgr(const TextureMgr& rhs);
	TextureMgr& operator=(const TextureMgr& rhs);
	~TextureMgr();

	void buildRandomTex();

private:
	ID3D10Device* md3dDevice;

	StringVector mTextureNames;
	std::vector<ID3D10ShaderResourceView*> mTextureRVs;

	ID3D10ShaderResourceView* mRandomTexRV;
};

TextureMgr& GetTextureMgr();

#endif // TEXTUREMGR_H