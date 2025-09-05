#pragma once
#include "stdafx.h"
#include <DirectXMath.h>



namespace IGCS
{
	using namespace DirectX;
	//struct Vec3
	//{
	//	float values[3];

	//	//----------------------------------
	//	// Methods below this line
	//	Vec3()
	//	{
	//		values[0] = 0.0f;
	//		values[1] = 0.0f;
	//		values[2] = 0.0f;
	//	}

	//	Vec3(XMFLOAT3 v)
	//	{
	//		values[0] = v.x;
	//		values[1] = v.y;
	//		values[2] = v.z;
	//	}

	//	Vec3(Vector3 v)
	//	{
	//		values[0] = v.x;
	//		values[1] = v.y;
	//		values[2] = v.z;
	//	}

	//	operator DirectX::XMFLOAT3() const
	//	{
	//		return {values[0], values[1], values[2]};
	//	}

	//	void setValues(XMFLOAT3 v)
	//	{
	//		setValues(v.x, v.y, v.z);
	//	}

	//	void setValues(float x, float y, float z)
	//	{
	//		values[0] = x;
	//		values[1] = y;
	//		values[2] = z;
	//	}

	//	float x() const { return values[0]; }
	//	float y() const { return values[1]; }
	//	float z() const { return values[2]; }
	//};


	//struct Vec4
	//{
	//	float values[4];

	//	//----------------------------------
	//	// Methods below this line
	//	Vec4()
	//	{
	//		values[0] = 0.0f;
	//		values[1] = 0.0f;
	//		values[2] = 0.0f;
	//		values[3] = 0.0f;
	//	}

	//	Vec4(DirectX::XMFLOAT4 v)
	//	{
	//		values[0] = v.x;
	//		values[1] = v.y;
	//		values[2] = v.z;
	//		values[3] = v.w;
	//	}

	//	void setValues(DirectX::XMFLOAT4 v)
	//	{
	//		setValues(v.x, v.y, v.z, v.w);
	//	}

	//	void setValues(float x, float y, float z, float w)
	//	{
	//		values[0] = x;
	//		values[1] = y;
	//		values[2] = z;
	//		values[3] = w;
	//	}

	//	float x() { return values[0]; }
	//	float y() { return values[1]; }
	//	float z() { return values[2]; }
	//	float w() { return values[3]; }
	//};

	struct CameraToolsData
	{
		uint8_t cameraEnabled;					// 1 is enabled 0 is not enabled
		uint8_t cameraMovementLocked;			// 1 is camera movement is locked, 0 is camera movement isn't locked.
		uint8_t reserved1;
		uint8_t reserved2;
		float fov;								// in degrees
		XMFLOAT3 coordinates;					// camera coordinates (x, y, z)
		XMFLOAT4 lookQuaternion;					// camera look quaternion qx, qy, qz, qw
		XMFLOAT3 rotationMatrixUpVector;			// up vector from the rotation matrix calculated from the look quaternion. 
		XMFLOAT3 rotationMatrixRightVector;		// right vector from the rotation matrix calculated from the look quaternion. 
		XMFLOAT3 rotationMatrixForwardVector;	// forward vector from the rotation matrix calculated from the look quaternion. 
		float pitch;							// in radians
		float yaw;
		float roll;
	};


}



