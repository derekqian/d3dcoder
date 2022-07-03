//=============================================================================
// table.h by Frank Luna (C) 2004 All Rights Reserved.
//============================================================================= 

#ifndef TABLE_H
#define TABLE_H

#include <cassert>
#include <vector> 
#pragma comment(lib, "legacy_stdio_definitions.lib")

template <typename T>
class Table
{
public:
	Table()
		: mRows(0), mCols(0)
	{
	}

	Table(int m, int n)
		: mRows(m), mCols(n), mMatrix(m*n)
	{
	}

	Table(int m, int n, const T& value)
		: mRows(m), mCols(n), mMatrix(m*n, value)
	{
	}

	// For non-const objects
	T& operator()(int i, int j)
	{
		return mMatrix[i*mCols+j];
	}

	// For const objects
	const T& operator()(int i, int j)const
	{
		return mMatrix[i*mCols+j];
	}

	// Add typename to let compiler know type and not static variable.
	typedef typename std::vector<T>::iterator iter;
	typedef typename std::vector<T>::const_iterator citer;

	// For non-const objects
	iter begin(){ return mMatrix.begin(); }
	iter end()	{ return mMatrix.end();   }
	
	// For const objects
	citer begin() const { return mMatrix.begin(); }
	citer end() const { return mMatrix.end();   }

	int numRows() const	{ return mRows;	}
	int numCols() const	{ return mCols;	}

	void resize(int m, int n)
	{
		mRows = m;
		mCols = n;
		mMatrix.resize(m*n);
	}

	void resize(int m, int n, const T& value)
	{
		mRows = m;
		mCols = n;
		mMatrix.resize(m*n, value);
	}

private:
	int mRows;
	int mCols;
	std::vector<T> mMatrix;
};

#endif // TABLE_H