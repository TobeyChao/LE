#pragma once
#include "D3D12Util.h"

class Camera
{
public:
	Camera();
	~Camera();

	void MoveForward();

	void MoveBack();

	void MoveLeft();

	void MoveRight();

	void SetPitch(float pitch);
	void SetYaw(float yaw);
	void SetRoll(float roll);

	void ComputeInfo();

	inline const DirectX::XMVECTOR& GetCameraPosition() const { return mPosition; }

	inline const DirectX::XMMATRIX& GetViewMatrix() const { return mViewMat; }
private:
	float mSpeed = 20.0;

	bool mDirty = true;

	float mPitch = 0;
	float mYaw = 0;
	float mRoll = 0;

	// Camera coordinate system with coordinates relative to world space.
	DirectX::XMVECTOR mPosition = { 0.0f, 10.0f, 0.0f };
	DirectX::XMVECTOR mRight = { 1.0f, 0.0f, 0.0f };
	DirectX::XMVECTOR mUp = { 0.0f, 1.0f, 0.0f };
	DirectX::XMVECTOR mForward = { 0.0f, 0.0f, 1.0f };

	DirectX::XMVECTOR mDefaultPosition = { 0.0f, 10.0f, 0.0f };
	DirectX::XMVECTOR mDefaultRight = { 1.0f, 0.0f, 0.0f };
	DirectX::XMVECTOR mDefaultUp = { 0.0f, 1.0f, 0.0f };
	DirectX::XMVECTOR mDefaultForward = { 0.0f, 0.0f, 1.0f };

	DirectX::XMMATRIX mViewMat;
};