//=============================================================================
// Camera.cpp by Frank Luna (C) 2004 All Rights Reserved.
//=============================================================================

#include "Camera.h"
#include "DirectInput.h"
#include "d3dUtil.h"
#include "Terrain.h"

Camera* gCamera = 0;

Camera::Camera()
{
	D3DXMatrixIdentity(&mView);
	D3DXMatrixIdentity(&mProj);
	D3DXMatrixIdentity(&mViewProj);

	mPosW   = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	mRightW = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
	mUpW    = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
	mLookW  = D3DXVECTOR3(0.0f, 0.0f, 1.0f);

	// Client should adjust to a value that makes sense for application's
	// unit scale, and the object the camera is attached to--e.g., car, jet,
	// human walking, etc.
	mSpeed  = 50.0f;
}

const D3DXMATRIX& Camera::view() const
{
	return mView;
}

const D3DXMATRIX& Camera::proj() const
{
	return mProj;
}

const D3DXMATRIX& Camera::viewProj() const
{
	return mViewProj;
}

const D3DXVECTOR3& Camera::right() const
{
	return mRightW;
}

const D3DXVECTOR3& Camera::up() const
{
	return mUpW;
}

const D3DXVECTOR3& Camera::look() const
{
	return mLookW;
}

D3DXVECTOR3& Camera::pos()
{
	return mPosW;
}

void Camera::lookAt(D3DXVECTOR3& pos, D3DXVECTOR3& target, D3DXVECTOR3& up)
{
	D3DXVECTOR3 L = target - pos;
	D3DXVec3Normalize(&L, &L);

	D3DXVECTOR3 R;
	D3DXVec3Cross(&R, &up, &L);
	D3DXVec3Normalize(&R, &R);

	D3DXVECTOR3 U;
	D3DXVec3Cross(&U, &L, &R);
	D3DXVec3Normalize(&U, &U);

	mPosW   = pos;
	mRightW = R;
	mUpW    = U;
	mLookW  = L;

	buildView();
	buildWorldFrustumPlanes();

	mViewProj = mView * mProj;
}

void Camera::setLens(float fov, float aspect, float nearZ, float farZ)
{
	D3DXMatrixPerspectiveFovLH(&mProj, fov, aspect, nearZ, farZ);
	buildWorldFrustumPlanes();
	mViewProj = mView * mProj;
}

void Camera::setSpeed(float s)
{
	mSpeed = s;
}

bool Camera::isVisible(const AABB& box)const
{
	// Test assumes frustum planes face inward.

	D3DXVECTOR3 P;
	D3DXVECTOR3 Q;

	//      N  *Q                    *P
	//      | /                     /
	//      |/                     /
	// -----/----- Plane     -----/----- Plane    
	//     /                     / |
	//    /                     /  |
	//   *P                    *Q  N
	//
	// PQ forms diagonal most closely aligned with plane normal.

	// For each frustum plane, find the box diagonal (there are four main
	// diagonals that intersect the box center point) that points in the
	// same direction as the normal along each axis (i.e., the diagonal 
	// that is most aligned with the plane normal).  Then test if the box
	// is in front of the plane or not.
	for(int i = 0; i < 6; ++i)
	{
		// For each coordinate axis x, y, z...
		for(int j = 0; j < 3; ++j)
		{
			// Make PQ point in the same direction as the plane normal on this axis.
			if( mFrustumPlanes[i][j] >= 0.0f )
			{
				P[j] = box.minPt[j];
				Q[j] = box.maxPt[j];
			}
			else 
			{
				P[j] = box.maxPt[j];
				Q[j] = box.minPt[j];
			}
		}

		// If box is in negative half space, it is behind the plane, and thus, completely
		// outside the frustum.  Note that because PQ points roughly in the direction of the 
		// plane normal, we can deduce that if Q is outside then P is also outside--thus we
		// only need to test Q.
		if( D3DXPlaneDotCoord(&mFrustumPlanes[i], &Q) < 0.0f  ) // outside
			return false;
	}
	return true;
}

void Camera::update(float dt, Terrain* terrain, float offsetHeight)
{
	// Find the net direction the camera is traveling in (since the
	// camera could be running and strafing).
	D3DXVECTOR3 dir(0.0f, 0.0f, 0.0f);
	if( gDInput->keyDown(DIK_W) )
		dir += mLookW;
	if( gDInput->keyDown(DIK_S) )
		dir -= mLookW;
	if( gDInput->keyDown(DIK_D) )
		dir += mRightW;
	if( gDInput->keyDown(DIK_A) )
		dir -= mRightW;

	// Move at mSpeed along net direction.
	D3DXVec3Normalize(&dir, &dir);
	D3DXVECTOR3 newPos = mPosW + dir*mSpeed*dt;

	if( terrain != 0)
	{
		// New position might not be on terrain, so project the
		// point onto the terrain.
		newPos.y = terrain->getHeight(newPos.x, newPos.z) + offsetHeight;

		// Now the difference of the new position and old (current) 
		// position approximates a tangent vector on the terrain.
		D3DXVECTOR3 tangent = newPos - mPosW;
		D3DXVec3Normalize(&tangent, &tangent);

		// Now move camera along tangent vector.
		mPosW += tangent*mSpeed*dt;

		// After update, there may be errors in the camera height since our
		// tangent is only an approximation.  So force camera to correct height,
		// and offset by the specified amount so that camera does not sit
		// exactly on terrain, but instead, slightly above it.
		mPosW.y = terrain->getHeight(mPosW.x, mPosW.z) + offsetHeight;
	}
	else
	{
		mPosW = newPos;
	}
	

	// We rotate at a fixed speed.
	float pitch  = gDInput->mouseDY() / 150.0f;
	float yAngle = gDInput->mouseDX() / 150.0f;


	// Rotate camera's look and up vectors around the camera's right vector.
	D3DXMATRIX R;
	D3DXMatrixRotationAxis(&R, &mRightW, pitch);
	D3DXVec3TransformCoord(&mLookW, &mLookW, &R);
	D3DXVec3TransformCoord(&mUpW, &mUpW, &R);


	// Rotate camera axes about the world's y-axis.
	D3DXMatrixRotationY(&R, yAngle);
	D3DXVec3TransformCoord(&mRightW, &mRightW, &R);
	D3DXVec3TransformCoord(&mUpW, &mUpW, &R);
	D3DXVec3TransformCoord(&mLookW, &mLookW, &R);


	// Rebuild the view matrix to reflect changes.
	buildView();
	buildWorldFrustumPlanes();

	mViewProj = mView * mProj;
}

void Camera::buildView()
{
	// Keep camera's axes orthogonal to each other and of unit length.
	D3DXVec3Normalize(&mLookW, &mLookW);

	D3DXVec3Cross(&mUpW, &mLookW, &mRightW);
	D3DXVec3Normalize(&mUpW, &mUpW);

	D3DXVec3Cross(&mRightW, &mUpW, &mLookW);
	D3DXVec3Normalize(&mRightW, &mRightW);

	// Fill in the view matrix entries.

	float x = -D3DXVec3Dot(&mPosW, &mRightW);
	float y = -D3DXVec3Dot(&mPosW, &mUpW);
	float z = -D3DXVec3Dot(&mPosW, &mLookW);

	mView(0,0) = mRightW.x; 
	mView(1,0) = mRightW.y; 
	mView(2,0) = mRightW.z; 
	mView(3,0) = x;   

	mView(0,1) = mUpW.x;
	mView(1,1) = mUpW.y;
	mView(2,1) = mUpW.z;
	mView(3,1) = y;  

	mView(0,2) = mLookW.x; 
	mView(1,2) = mLookW.y; 
	mView(2,2) = mLookW.z; 
	mView(3,2) = z;   

	mView(0,3) = 0.0f;
	mView(1,3) = 0.0f;
	mView(2,3) = 0.0f;
	mView(3,3) = 1.0f;
}

void Camera::buildWorldFrustumPlanes()
{
	// Note: Extract the frustum planes in world space.

	D3DXMATRIX VP = mView * mProj;

	D3DXVECTOR4 col0(VP(0,0), VP(1,0), VP(2,0), VP(3,0));
	D3DXVECTOR4 col1(VP(0,1), VP(1,1), VP(2,1), VP(3,1));
	D3DXVECTOR4 col2(VP(0,2), VP(1,2), VP(2,2), VP(3,2));
	D3DXVECTOR4 col3(VP(0,3), VP(1,3), VP(2,3), VP(3,3));

	// Planes face inward.
	mFrustumPlanes[0] = (D3DXPLANE)(col2);        // near
	mFrustumPlanes[1] = (D3DXPLANE)(col3 - col2); // far
	mFrustumPlanes[2] = (D3DXPLANE)(col3 + col0); // left
	mFrustumPlanes[3] = (D3DXPLANE)(col3 - col0); // right
	mFrustumPlanes[4] = (D3DXPLANE)(col3 - col1); // top
	mFrustumPlanes[5] = (D3DXPLANE)(col3 + col1); // bottom

	for(int i = 0; i < 6; i++)
		D3DXPlaneNormalize(&mFrustumPlanes[i], &mFrustumPlanes[i]);
}
 