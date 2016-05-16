//=======================================================================================
// d3dxmatrix.cpp by Frank Luna (C) 2008 All Rights Reserved.
//
// Illustrates the D3DXMATRIX class.
//=======================================================================================

// Remember to link d3dx10.lib.

#include <d3dx10.h>
#include <iostream>
using namespace std;

// Overload the  "<<" operators so that we can use cout to 
// output D3DXVECTOR4 and D3DXMATRIX objects.

ostream& operator<<(ostream& os, D3DXVECTOR4& v)
{
	os << "(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
	return os;
}

ostream& operator<<(ostream& os, D3DXMATRIX& m)
{
	for(int i = 0; i < 4; ++i)
	{
		for(int j = 0; j < 4; ++j)
			os << m(i, j) << "  ";
		os << endl;
	}
	return os;
}

int main()
{
	D3DXMATRIX A(1.0f, 0.0f, 0.0f, 0.0f,
                 0.0f, 2.0f, 0.0f, 0.0f,
				 0.0f, 0.0f, 4.0f, 0.0f,
				 1.0f, 2.0f, 3.0f, 1.0f);

	D3DXMATRIX B;
	D3DXMatrixIdentity(&B);

	// matrix-matrix multiplication
	D3DXMATRIX C = A*B;

	D3DXMATRIX D, E, F;
	
	D3DXMatrixTranspose(&D, &A);

	D3DXMatrixInverse(&E, 0, &A);

	F = A * E;

	D3DXVECTOR4 P(2.0f, 2.0f, 2.0f, 1.0f);
	D3DXVECTOR4 Q(2.0f, 2.0f, 2.0f, 0.0f);
	D3DXVECTOR4 R, S;
	D3DXVec4Transform(&R, &P, &A);
	D3DXVec4Transform(&S, &Q, &A);
 
	cout << "A = "               << endl << A << endl;
	cout << "B = "               << endl << B << endl;
	cout << "C = A*B = "         << endl << C << endl;
	cout << "D = transpose(A)= " << endl << D << endl;
	cout << "E = inverse(A) = "  << endl << E << endl;
	cout << "F = A*E = "         << endl << F << endl;
	cout << "P = "               << P << endl;
	cout << "Q = "               << Q << endl;
	cout << "R = P*A = "         << R << endl;
	cout << "S = Q*A = "         << S << endl;

	return 0;
}