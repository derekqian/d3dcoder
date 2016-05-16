//=============================================================================
// Heightmap.h by Frank Luna (C) 2004 All Rights Reserved.
//=============================================================================

#ifndef HEIGHTMAP_H
#define HEIGHTMAP_H

#include "d3dUtil.h"
#include "Table.h"
#include <string>

class Heightmap
{
public:
	Heightmap();
	Heightmap(int m, int n);
	Heightmap(int m, int n, 
		const std::string& filename, float heightScale, float heightOffset);

	void recreate(int m, int n);

	void loadRAW(int m, int n,
		const std::string& filename, float heightScale, float heightOffset);

	int numRows()const;
	int numCols()const;

	// For non-const objects
	float& operator()(int i, int j);

	// For const objects
	const float& operator()(int i, int j)const;

private:
	bool  inBounds(int i, int j);
	float sampleHeight3x3(int i, int j);
	void  filter3x3();
private:
	std::string  mHeightMapFilename;
	Table<float> mHeightMap;
	float        mHeightScale;
	float        mHeightOffset;
};

#endif //HEIGHTMAP_H