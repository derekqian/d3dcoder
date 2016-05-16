#include "Camera.h"

Camera& GetCamera()
{
	static Camera camera;
	return camera;
}

Camera::Camera()
{
	mPosition = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	mRight    = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
	mUp       = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
	mLook     = D3DXVECTOR3(0.0f, 0.0f, 1.0f);

	D3DXMatrixIdentity(&mView);
	D3DXMatrixIdentity(&mProj);
}

Camera::~Camera()
{
}

D3DXVECTOR3& Camera::position()
{
	return mPosition;
}

D3DXMATRIX Camera::view()const
{
	return mView;
}

D3DXMATRIX Camera::proj()const
{
	return mProj;
}

void Camera::setLens(float fovY, float aspect, float zn, float zf)
{
	D3DXMatrixPerspectiveFovLH(&mProj, fovY, aspect, zn, zf);
}

void Camera::strafe(float d)
{
	mPosition += d*mRight;
}

void Camera::walk(float d)
{
	mPosition += d*mLook;
}

void Camera::pitch(float angle)
{
	D3DXMATRIX R;
	D3DXMatrixRotationAxis(&R, &mRight, angle);

	D3DXVec3TransformNormal(&mUp, &mUp, &R);
	D3DXVec3TransformNormal(&mLook, &mLook, &R);
}

void Camera::rotateY(float angle)
{
	D3DXMATRIX R;
	D3DXMatrixRotationY(&R, angle);

	D3DXVec3TransformNormal(&mRight, &mRight, &R);
	D3DXVec3TransformNormal(&mUp, &mUp, &R);
	D3DXVec3TransformNormal(&mLook, &mLook, &R);
}

void Camera::rebuildView()
{
	// Keep camera's axes orthogonal to each other and of unit length.
	D3DXVec3Normalize(&mLook, &mLook);

	D3DXVec3Cross(&mUp, &mLook, &mRight);
	D3DXVec3Normalize(&mUp, &mUp);

	D3DXVec3Cross(&mRight, &mUp, &mLook);
	D3DXVec3Normalize(&mRight, &mRight);

	// Fill in the view matrix entries.
	float x = -D3DXVec3Dot(&mPosition, &mRight);
	float y = -D3DXVec3Dot(&mPosition, &mUp);
	float z = -D3DXVec3Dot(&mPosition, &mLook);

	mView(0,0) = mRight.x; 
	mView(1,0) = mRight.y; 
	mView(2,0) = mRight.z; 
	mView(3,0) = x;   

	mView(0,1) = mUp.x;
	mView(1,1) = mUp.y;
	mView(2,1) = mUp.z;
	mView(3,1) = y;  

	mView(0,2) = mLook.x; 
	mView(1,2) = mLook.y; 
	mView(2,2) = mLook.z; 
	mView(3,2) = z;   

	mView(0,3) = 0.0f;
	mView(1,3) = 0.0f;
	mView(2,3) = 0.0f;
	mView(3,3) = 1.0f;
}


