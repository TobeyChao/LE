#include "Camera.h"

Camera::Camera()
{
}

Camera::~Camera()
{
}

void Camera::MoveForward()
{
	mPosition = DirectX::XMVectorMultiplyAdd(mForward, { 1.f, 1.f, 1.f }, mPosition);
	mDirty = true;
}

void Camera::MoveBack()
{
	mPosition = DirectX::XMVectorMultiplyAdd(mForward, { -1.f, -1.f, -1.f }, mPosition);
	mDirty = true;
}

void Camera::MoveLeft()
{
	mPosition = DirectX::XMVectorMultiplyAdd(mRight, { -1.f, -1.f, -1.f }, mPosition);
	mDirty = true;
}

void Camera::MoveRight()
{
	mPosition = DirectX::XMVectorMultiplyAdd(mRight, { 1.f, 1.f, 1.f }, mPosition);
	mDirty = true;
}

void Camera::SetPitch(float pitch)
{
	if (mPitch == pitch)
	{
		return;
	}
	mPitch = pitch;
	mDirty = true;
}

void Camera::SetYaw(float yaw)
{
	if (mYaw == yaw)
	{
		return;
	}
	mYaw = yaw;
	mDirty = true;
}

void Camera::SetRoll(float roll)
{
	mRoll = roll;
	mDirty = true;
}

void Camera::ComputeInfo()
{
	if (!mDirty)
	{
		return;
	}
	// 这里默认以：
	// z轴正方向为forward
	// y轴正方向为up
	// x轴正方向为right

	//float cosP = cosf(mPitch);
	//float cosY = cosf(mYaw);
	//float cosR = cosf(mRoll);
	//float sinP = sinf(mPitch);
	//float sinY = sinf(mYaw);
	//float sinR = sinf(mRoll);

	//mForward = DirectX::XMVectorSet(sinY * cosP, -sinP, cosP * cosY, 1.0f);
	
	auto rotateMat = DirectX::XMMatrixRotationRollPitchYaw(mPitch, mYaw, mRoll);
	mForward = DirectX::XMVector3TransformNormal(mDefaultForward, rotateMat);
	mUp = DirectX::XMVector3TransformNormal(mDefaultUp, rotateMat);
	mRight = DirectX::XMVector3Cross(mUp, mForward);
	DirectX::XMVECTOR at = DirectX::XMVectorAdd(mPosition, mForward);
	mViewMat = DirectX::XMMatrixLookToLH(mPosition, mForward, mUp);

	mDirty = false;
}
