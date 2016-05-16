//=======================================================================================
// d3dxvec3.cpp by Frank Luna (C) 2008 All Rights Reserved.
//
// Illustrates the D3DXVECTOR3 class.
//=======================================================================================


// Remember to link d3dx10.lib.

#include <d3dx10.h>
#include <iostream>
using namespace std;

// Overload the  "<<" operators so that we can use cout to 
// output D3DXVECTOR3 objects.

ostream& operator<<(ostream& os, D3DXVECTOR3& v)
{
	os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
	return os;
}

int main()
{
	// Using constructor, D3DXVECTOR3(FLOAT x, FLOAT y, FLOAT z);
	D3DXVECTOR3 u(1.0f, 2.0f, 3.0f);

	// Using constructor, D3DXVECTOR3(CONST FLOAT *);
	float x[3] = {-2.0f, 1.0f, -3.0f};
	D3DXVECTOR3 v(x);

	// Using constructor, D3DXVECTOR3() {};
	D3DXVECTOR3 a, b, c, d, e;  

	// Vector addition: D3DXVECTOR3 operator + 
	a = u + v;

	// Vector subtraction: D3DXVECTOR3 operator - 
	b = u - v;

	// Scalar multiplication: D3DXVECTOR3 operator * 
	c = u * 10;

	// ||u||
	float L = D3DXVec3Length(&u);

	// d = u / ||u||
	D3DXVec3Normalize(&d, &u);

	// s = u dot v
	float s = D3DXVec3Dot(&u, &v);

	// e = u x v
	D3DXVec3Cross(&e, &u, &v);

	cout << "u             = " << u << endl;
	cout << "v             = " << v << endl;
	cout << "a = u + v     = " << a << endl;
	cout << "b = u - v     = " << b << endl;
	cout << "c = u * 10    = " << c << endl;
	cout << "d = u / ||u|| = " << d << endl;
	cout << "e = u x v     = " << e << endl;
	cout << "L = ||u||     = " << L << endl;
	cout << "s = u.v       = " << s << endl;

	return 0;
}