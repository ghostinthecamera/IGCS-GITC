////////////////////////////////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System
// Copyright(c) 2019, Frans Bouma
// All rights reserved.
// https://github.com/FransBouma/InjectableGenericCameraSystem
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met :
//
//  * Redistributions of source code must retain the above copyright notice, this
//	  list of conditions and the following disclaimer.
//
//  * Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and / or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Utils.h"
#include "GameConstants.h"
#include "AOBBlock.h"
#include <comdef.h>
#include <codecvt>
#include <filesystem>
#include "GameImageHooker.h"
#include <DirectXMath.h>
#include <cmath>

#include "CameraManipulator.h"
#include "System.h"

using namespace std;
using namespace DirectX;

namespace IGCS::Utils
{

	void preciseSleep(DWORD ms)
	{
		// grab frequency once per call (cheap enough), or stash in a static if you really care
		static LARGE_INTEGER freq = {};
		// Initialize frequency once
		if (freq.QuadPart == 0) {
			QueryPerformanceFrequency(&freq);
		}
		LARGE_INTEGER start;
		QueryPerformanceCounter(&start);

		// how many ticks we need to wait
		LONGLONG waitTicks = (freq.QuadPart * ms) / 1000;

		LARGE_INTEGER now;
		do {
			Sleep(0);      // yield to other threads at this priority (already declared via stdafx.h)
			QueryPerformanceCounter(&now);
		} while ((now.QuadPart - start.QuadPart) < waitTicks);
	}

	// This table is from Reshade.
	static const char vkCodeToStringLookup[256][16] =
	{
		"", "", "", "Cancel", "", "", "", "", "Backspace", "Tab", "", "", "Clear", "Enter", "", "",
		"Shift", "Control", "Alt", "Pause", "Caps Lock", "", "", "", "", "", "", "Escape", "", "", "", "",
		"Space", "Page Up", "Page Down", "End", "Home", "Left Arrow", "Up Arrow", "Right Arrow", "Down Arrow",
		"Select", "", "", "Print Screen", "Insert", "Delete", "Help",
		"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "", "", "", "", "", "",
		"", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
		"P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "Left Windows", "Right Windows", "", "", "Sleep",
		"Numpad 0", "Numpad 1", "Numpad 2", "Numpad 3", "Numpad 4", "Numpad 5", "Numpad 6", "Numpad 7", "Numpad 8", "Numpad 9",
		"Numpad *", "Numpad +", "", "Numpad -", "Numpad Decimal", "Numpad /",
		"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "F13", "F14", "F15", "F16",
		"F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24", "", "", "", "", "", "", "", "",
		"Num Lock", "Scroll Lock",
	};

	double getCurrentTimeSeconds()
	{
		using namespace std::chrono;

		static auto start = steady_clock::now();
		auto now = steady_clock::now();
		auto elapsed = duration_cast<milliseconds>(now - start).count();
		return static_cast<double>(elapsed) / 1000.0;
	}

	//--------------------------------------------------------------------------------------
	// Create a view matrix given a camera's world position and orientation (as a quaternion).
	// The world matrix is built from the rotation and translation, and then inverted.
	// 
	// Parameters:
	//    cameraPos   - The camera's world position (as an XMFLOAT3).
	//    orientation - The camera's world orientation (as a quaternion).
	//
	// Returns:
	//    The view matrix.
	XMMATRIX CreateViewMatrix(const XMFLOAT3& cameraPos, const XMVECTOR& orientation)
	{
		// Load the camera position into an XMVECTOR.
		XMVECTOR pos = XMLoadFloat3(&cameraPos);

		// Build the world matrix: first rotate, then translate.
		XMMATRIX world = XMMatrixRotationQuaternion(orientation) *
			XMMatrixTranslationFromVector(pos);

		// The view matrix is the inverse of the world matrix.
		XMMATRIX view = XMMatrixInverse(nullptr, world);
		return view;
	}

	//--------------------------------------------------------------------------------------
	// Create a view matrix given Euler angles (in radians) and a camera position.
	// This function converts the Euler angles (with a specified intrinsic rotation order)
	// into a quaternion (optionally negating specified axes), then builds the view matrix.
	// 
	// Parameters:
	//    cameraPos    - The camera's world position (as an XMFLOAT3).
	//    euler        - The Euler angles (pitch, yaw, roll) in radians.
	//    order        - The intrinsic Euler rotation order (see EulerOrder enum).
	//    negatePitch  - If true, uses -pitch instead of pitch.
	//    negateYaw    - If true, uses -yaw instead of yaw.
	//    negateRoll   - If true, uses -roll instead of roll.
	//
	// Returns:
	//    The view matrix.


	XMMATRIX CreateViewMatrixFromEuler(const XMFLOAT3& cameraPos, const XMFLOAT3& euler,
		EulerOrder order,
		bool negatePitch,
		bool negateYaw,
		bool negateRoll)
	{
		// Convert the Euler angles to a quaternion.
		XMVECTOR orientation = generateEulerQuaternion(euler, order,
			negatePitch, negateYaw, negateRoll);
		// Build and return the view matrix using the helper above.
		return CreateViewMatrix(cameraPos, orientation);
	}


	// Extracts the camera's world position from a matrix.
	// Parameters:
	//   m             - The input matrix.
	//   isColumnMajor - If true, the matrix is assumed to be stored in column–major order and will be transposed.
	//   isViewMatrix  - If true, the matrix is assumed to be a view matrix (camera transform); its inverse is taken so that the returned position corresponds to the camera's world position.
	// If isViewMatrix is false, the function assumes that the translation part of the matrix directly contains the world position.
	XMFLOAT3 ExtractCameraPosition(const XMMATRIX& m, bool isColumnMajor, bool isViewMatrix)
	{
		// Start with the provided matrix.
		XMMATRIX mat = m;

		// If the matrix is stored in column–major order, transpose it to match DirectXMath’s row–major expectation.
		if (isColumnMajor)
		{
			mat = XMMatrixTranspose(mat);
		}

		// For a view matrix, the stored matrix is the inverse of the camera’s world transform.
		// Thus, the camera's world position is the translation component of the inverse.
		if (isViewMatrix)
		{
			XMMATRIX invMat = XMMatrixInverse(nullptr, mat);
			XMFLOAT3 pos;
			// In DirectXMath, the translation is stored in the 4th row (r[3]) of a row–major XMMATRIX.
			XMStoreFloat3(&pos, invMat.r[3]);
			return pos;
		}
		else
		{
			// If not a view matrix, assume that the translation component of mat directly represents the world position.
			XMFLOAT3 pos;
			XMStoreFloat3(&pos, mat.r[3]);
			return pos;
		}
	}


	// Assume we already have a function to convert Euler angles to a quaternion:
	// XMVECTOR EulerAnglesToQuaternionWithRotationNormal(const XMFLOAT3& euler, EulerOrder order,
	//                                                     bool negatePitch = false,
	//                                                     bool negateYaw   = false,
	//                                                     bool negateRoll  = false);
	// This function creates a rotation matrix from Euler angles by using an intermediate quaternion.
	XMMATRIX EulerAnglesToRotationMatrix(const XMFLOAT3& euler, EulerOrder order,
		bool negatePitch,
		bool negateYaw,
		bool negateRoll)
	{
		// First, create the quaternion from the Euler angles.
		XMVECTOR q = generateEulerQuaternion(euler, order, negatePitch, negateYaw, negateRoll);

		// Then, convert the quaternion into a rotation matrix.
		XMMATRIX m = XMMatrixRotationQuaternion(q);

		return m;
	}


	// Converts a rotation matrix to Euler angles (pitch, yaw, roll) given a specified intrinsic rotation order.
	// Parameters:
	//   m             - The input rotation matrix.
	//   order         - The intrinsic Euler rotation order (see EulerOrder enum).
	//   isColumnMajor - If true, the input matrix is assumed to be column–major and will be transposed.
	//   isViewMatrix  - If true, the matrix is assumed to be a view matrix (camera transformation),
	//                   so its rotation part is inverted (i.e. the function will return the camera's orientation).
	XMFLOAT3 RotationMatrixToEulerAngles(const XMMATRIX& m, EulerOrder order, bool isColumnMajor, bool isViewMatrix)
	{
		// Start with the provided matrix.
		XMMATRIX mat = m;

		// If the matrix is column–major, transpose it so that it matches DirectXMath’s row–major convention.
		if (isColumnMajor)
		{
			mat = XMMatrixTranspose(mat);
		}

		// Convert the (now properly oriented) rotation matrix to a quaternion.
		XMVECTOR q = XMQuaternionRotationMatrix(mat);

		// If the matrix is a view matrix, its rotation represents the inverse of the camera's orientation.
		// Taking the conjugate (which equals the inverse for unit quaternions) yields the proper orientation.
		if (isViewMatrix)
		{
			q = XMQuaternionConjugate(q);
		}

		// Convert the quaternion to Euler angles using the established routine.
		return QuaternionToEulerAngles(q, order);
	}
	
	
	// Converts Euler angles (pitch, yaw, roll) into a quaternion
	// according to the specified intrinsic rotation order.
	// Input:
	//   euler.x = pitch (rotation about X)
	//   euler.y = yaw   (rotation about Y)
	//   euler.z = roll  (rotation about Z)
	// The EulerOrder enum specifies the order in which these rotations are applied,
	// reading from left to right. For example, if order is EulerOrder::ZYX, then the
	// rotations are applied in the order: first about Z, then about Y, then about X.
	// Because quaternion multiplication applies the right–most factor first, the
	// composite quaternion is computed as:
	//   q = q_firstApplied (rightmost) × q_second × q_thirdApplied (leftmost).
	// In our code, for EulerOrder::ZYX we compute:
	//   q = XMQuaternionMultiply( qx, XMQuaternionMultiply( qy, qz ) );
	// which means that qz (rotation about Z) is applied first, then qy, then qx.
	XMVECTOR generateEulerQuaternionManual(const XMFLOAT3& euler, EulerOrder order, bool negatePitch, bool negateYaw, bool negateRoll)
	{
		// Apply negation as specified.
		float pitch = negatePitch ? -euler.x : euler.x; // rotation about X
		float yaw = negateYaw ? -euler.y : euler.y;   // rotation about Y
		float roll = negateRoll ? -euler.z : euler.z;    // rotation about Z
		
		// Compute half–angles.
		float halfPitch = pitch * 0.5f; // rotation about X
		float halfYaw = yaw * 0.5f; // rotation about Y
		float halfRoll = roll * 0.5f; // rotation about Z

		// Precompute sines and cosines.
		float sinPitch = sinf(halfPitch);
		float cosPitch = cosf(halfPitch);
		float sinYaw = sinf(halfYaw);
		float cosYaw = cosf(halfYaw);
		float sinRoll = sinf(halfRoll);
		float cosRoll = cosf(halfRoll);

		// Elemental quaternions for rotations about X, Y, and Z.
		// Quaternion format is (x, y, z, w)
		XMVECTOR qx = XMVectorSet(sinPitch, 0.0f, 0.0f, cosPitch);
		XMVECTOR qy = XMVectorSet(0.0f, sinYaw, 0.0f, cosYaw);
		XMVECTOR qz = XMVectorSet(0.0f, 0.0f, sinRoll, cosRoll);

		XMVECTOR q = XMQuaternionIdentity();

		// Based on the specified Euler order, multiply the elemental quaternions
		// in the proper order. (Recall: if the intrinsic rotations are applied in
		// the order A, then B, then C, the composite quaternion is: q = q_C * q_B * q_A,
		// because the right–most quaternion is applied first.)
		switch (order)
		{
		case EulerOrder::ZYX:
			// Intrinsic order: first rotation about Z, then Y, then X.
			// Composite quaternion: q = qx * (qy * qz)
			q = XMQuaternionMultiply(qx, XMQuaternionMultiply(qy, qz));
			break;
		case EulerOrder::ZXY:
			// Intrinsic order: first Z, then X, then Y.
			// q = qy * (qx * qz)
			q = XMQuaternionMultiply(qy, XMQuaternionMultiply(qx, qz));
			break;
		case EulerOrder::XYZ:
			// Intrinsic order: first X, then Y, then Z.
			// q = qz * (qy * qx)
			q = XMQuaternionMultiply(qz, XMQuaternionMultiply(qy, qx));
			break;
		case EulerOrder::XZY:
			// Intrinsic order: first X, then Z, then Y.
			// q = qy * (qz * qx)
			q = XMQuaternionMultiply(qy, XMQuaternionMultiply(qz, qx));
			break;
		case EulerOrder::YXZ:
			// Intrinsic order: first Y, then X, then Z.
			// q = qz * (qx * qy)
			q = XMQuaternionMultiply(qz, XMQuaternionMultiply(qx, qy));
			break;
		case EulerOrder::YZX:
			// Intrinsic order: first Y, then Z, then X.
			// q = qx * (qz * qy)
			q = XMQuaternionMultiply(qx, XMQuaternionMultiply(qz, qy));
			break;
		}

		q = XMQuaternionNormalize(q);
		return q;
	}

	XMVECTOR generateEulerQuaternion(const XMFLOAT3& euler, EulerOrder order, bool negatePitch, bool negateYaw, bool negateRoll)
	{
		// Apply negation as specified.
		float pitch = negatePitch ? -euler.x : euler.x; // rotation about X
		float yaw = negateYaw ? -euler.y : euler.y;   // rotation about Y
		float roll = negateRoll ? -euler.z : euler.z;    // rotation about Z

		// Use XMQuaternionRotationNormal.
		// The axes here are the standard unit axes.
		XMVECTOR qx = XMQuaternionRotationNormal(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), pitch);
		XMVECTOR qy = XMQuaternionRotationNormal(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), yaw);
		XMVECTOR qz = XMQuaternionRotationNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), roll);

		// Compose the quaternions according to the specified intrinsic rotation order.
		XMVECTOR q = XMQuaternionIdentity();
		switch (order)
		{
		case EulerOrder::ZYX:
			// Intrinsic order: first Z, then Y, then X.
			q = XMQuaternionMultiply(qx, XMQuaternionMultiply(qy, qz));
			break;
		case EulerOrder::ZXY:
			// Intrinsic order: first Z, then X, then Y.
			q = XMQuaternionMultiply(qy, XMQuaternionMultiply(qx, qz));
			break;
		case EulerOrder::XYZ:
			// Intrinsic order: first X, then Y, then Z.
			q = XMQuaternionMultiply(qz, XMQuaternionMultiply(qy, qx));
			break;
		case EulerOrder::XZY:
			// Intrinsic order: first X, then Z, then Y.
			q = XMQuaternionMultiply(qy, XMQuaternionMultiply(qz, qx));
			break;
		case EulerOrder::YXZ:
			// Intrinsic order: first Y, then X, then Z.
			q = XMQuaternionMultiply(qz, XMQuaternionMultiply(qx, qy));
			break;
		case EulerOrder::YZX:
			// Intrinsic order: first Y, then Z, then X.
			q = XMQuaternionMultiply(qx, XMQuaternionMultiply(qz, qy));
			break;
		}

		q = XMQuaternionNormalize(q);

		return q;
	}


	//XMFLOAT3 QuaternionToEulerAngles(XMVECTOR q, EulerOrder order)
	//{
	//	// 1. Determine mapping indices (i, j, k) based on the rotation order.
	//	int i = 0, j = 0, k = 0;
	//	switch (order)
	//	{
	//	case EulerOrder::XYZ: i = 2; j = 1; k = 0; break;
	//	case EulerOrder::XZY: i = 1; j = 2; k = 0; break;
	//	case EulerOrder::YXZ: i = 2; j = 0; k = 1; break;
	//	case EulerOrder::YZX: i = 0; j = 2; k = 1; break;
	//	case EulerOrder::ZXY: i = 1; j = 0; k = 2; break;
	//	case EulerOrder::ZYX: i = 0; j = 1; k = 2; break;
	//	}

	//	// 2. Compute the sign factor to adjust for handedness differences.
	//	float sign = static_cast<float>((i - j) * (j - k) * (k - i)) / 2.0f;

	//	// 3. Extract quaternion components (assumed order: x, y, z, w).
	//	float quat[4] = {
	//		XMVectorGetX(q),
	//		XMVectorGetY(q),
	//		XMVectorGetZ(q),
	//		XMVectorGetW(q)
	//	};

	//	// 4. Compute intermediate values based on the mapping indices and sign.
	//	float a = quat[3] - quat[j];
	//	float b = quat[i] + quat[k] * sign;
	//	float c = quat[j] + quat[3];
	//	float d = quat[k] * sign - quat[i];

	//	float n2 = a * a + b * b + c * c + d * d;

	//	// 5. Compute the middle Euler angle.
	//	float angles[3] = { 0.0f, 0.0f, 0.0f };
	//	angles[1] = acosf((2.0f * (a * a + b * b) / n2) - 1.0f);

	//	// Use a small epsilon to check for singularities.
	//	const float eps = numeric_limits<float>::epsilon();
	//	bool safe1 = (fabsf(angles[1]) >= eps);
	//	bool safe2 = (fabsf(angles[1] - XM_PI) >= eps);

	//	float half_sum = 0.0f, half_diff = 0.0f;
	//	if (safe1 && safe2)
	//	{
	//		half_sum = atan2f(b, a);
	//		half_diff = atan2f(-d, c);
	//		angles[0] = half_sum + half_diff;
	//		angles[2] = half_sum - half_diff;
	//	}
	//	else
	//	{
	//		angles[0] = 0.0f;
	//		if (!safe1)
	//		{
	//			half_sum = atan2f(b, a);
	//			angles[2] = 2.0f * half_sum;
	//		}
	//		if (!safe2)
	//		{
	//			half_diff = atan2f(-d, c);
	//			angles[2] = -2.0f * half_diff;
	//		}
	//	}

	//	// 6. Clamp each angle to the range [-pi, pi].
	//	for (int idx = 0; idx < 3; idx++)
	//	{
	//		if (angles[idx] < -XM_PI)
	//			angles[idx] += XM_2PI;
	//		else if (angles[idx] > XM_PI)
	//			angles[idx] -= XM_2PI;
	//	}

	//	// 7. Adjust angles:
	//	// Multiply the third angle by the sign factor.
	//	angles[2] *= sign;
	//	// Adjust the middle angle by subtracting pi/2.
	//	angles[1] -= XM_PIDIV2;
	//	// Swap the first and third angles (this reversal handles intrinsic rotation order).
	//	std::swap(angles[0], angles[2]);

	//	// 8. Return the Euler angles as an XMFLOAT3 in the order: pitch (X), yaw (Y), roll (Z).
	//	return XMFLOAT3(
	//		clampAngle(angles[1]),  // pitch
	//		clampAngle(angles[0]),  // yaw
	//		clampAngle(angles[2])   // roll
	//	);
	//}

	XMFLOAT3 QuaternionToEulerAngles(XMVECTOR q, EulerOrder order)
	{
		// Optimized lookup table for rotation orders
		static const struct {
			int8_t i, j, k;
			float sign;
		} lookup[6] = {
			{2, 1, 0, -1.0f}, // XYZ
			{1, 2, 0,  1.0f}, // XZY
			{2, 0, 1,  1.0f}, // YXZ
			{0, 2, 1, -1.0f}, // YZX
			{1, 0, 2, -1.0f}, // ZXY
			{0, 1, 2,  1.0f}  // ZYX
		};

		// Direct lookup instead of switch statement
		const auto& data = lookup[static_cast<int>(order)];

		// Extract quaternion components in one operation
		XMFLOAT4 quatFloat;
		XMStoreFloat4(&quatFloat, q);
		const float quat[4] = { quatFloat.x, quatFloat.y, quatFloat.z, quatFloat.w };

		// Compute intermediate values with minimal operations
		const float a = quat[3] - quat[data.j];
		const float b = quat[data.i] + quat[data.k] * data.sign;
		const float c = quat[data.j] + quat[3];
		const float d = quat[data.k] * data.sign - quat[data.i];

		// Pre-compute squares once
		const float a2 = a * a;
		const float b2 = b * b;
		const float sum_ab_squared = a2 + b2;

		const float n2 = sum_ab_squared + c * c + d * d;

		// Calculate middle angle
		float angles[3] = { 0.0f, 0.0f, 0.0f };
		angles[1] = acosf((2.0f * sum_ab_squared / n2) - 1.0f);

		// Use a practical epsilon for numerical stability
		constexpr float eps = 1.0e-6f;
		const bool safe1 = (angles[1] >= eps);
		const bool safe2 = (XM_PI - angles[1] >= eps);

		// Handle normal case and singularities efficiently
		if (safe1 && safe2) {
			// Normal case - no singularities
			const float half_sum = atan2f(b, a);
			const float half_diff = atan2f(-d, c);
			angles[0] = half_sum + half_diff;
			angles[2] = half_sum - half_diff;
		}
		else {
			// Gimbal lock case - flat branch structure for better prediction
			angles[0] = 0.0f;

			if (!safe1) {
				// Singularity at theta = 0
				angles[2] = 2.0f * atan2f(b, a);
			}
			else {
				// Singularity at theta = pi
				angles[2] = -2.0f * atan2f(-d, c);
			}
		}

		// Apply final adjustments
		angles[2] *= data.sign;
		angles[1] -= XM_PIDIV2;

		// Fast swap without std::swap overhead
		const float temp = angles[0];
		angles[0] = angles[2];
		angles[2] = temp;

		// Return result with angle clamping
		return XMFLOAT3(
			clampAngle(angles[1]),  // pitch
			clampAngle(angles[0]),  // yaw
			clampAngle(angles[2])   // roll
		);
	}

	inline float clampAngle(float angle)
	{
		while (angle < -XM_PI)
			angle += XM_2PI;
		while (angle > XM_PI)
			angle -= XM_2PI;
		return angle;
	}

	// Obtains the exe's filename + path and returns that as a path object.
	std::filesystem::path obtainHostExeAndPath()
	{
		char lpBuffer[MAX_PATH];
		GetModuleFileNameA(nullptr, lpBuffer, MAX_PATH);
		return filesystem::path(lpBuffer);
	}
	

	BOOL isMainWindow(HWND handle)
	{
		BOOL toReturn = GetWindow(handle, GW_OWNER) == static_cast<HWND>(0) && IsWindowVisible(handle);
		if (toReturn)
		{
			// check window title as there can be more top windows. 
			int bufsize = GetWindowTextLength(handle) + 1;
			LPWSTR title = new WCHAR[bufsize];
			GetWindowText(handle, title, bufsize);
			// trick to do a 'startswith' compare. Will only return 0 if the string actually starts with the fragment we want to compare with
			toReturn &= (wcsncmp(title, GAME_WINDOW_TITLE, wcslen(GAME_WINDOW_TITLE)) == 0);
			// convert title to char* so we can display it
			_bstr_t titleAsBstr(title);
			const char* titleAsChar = titleAsBstr;		// char conversion copy
			MessageHandler::logDebug("Window found with title: '%s'", titleAsChar);
		}
		return toReturn;
	}


	BOOL CALLBACK enumWindowsCallback(HWND handle, LPARAM lParam)
	{
		handle_data& data = *(handle_data*)lParam;
		unsigned long process_id = 0;
		GetWindowThreadProcessId(handle, &process_id);
		if (data.process_id != process_id || !isMainWindow(handle))
		{
			return TRUE;
		}
		data.best_handle = handle;
		return FALSE;
	}


	MODULEINFO getModuleInfoOfContainingProcess()
	{
		HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());
		HMODULE processModule = nullptr;
		if (nullptr != processHandle)
		{
			DWORD cbNeeded;
			if (!EnumProcessModulesEx(processHandle, &processModule, sizeof(processModule), &cbNeeded, LIST_MODULES_32BIT | LIST_MODULES_64BIT))
			{
				processModule = nullptr;
			}
		}
		MODULEINFO toReturn;
		if (nullptr == processModule)
		{
			toReturn.lpBaseOfDll = nullptr;
		}
		else
		{
			if (!GetModuleInformation(processHandle, processModule, &toReturn, sizeof(MODULEINFO)))
			{
				toReturn.lpBaseOfDll = nullptr;
			}
		}
		if (processHandle != nullptr)
		{
			CloseHandle(processHandle);
		}
		return toReturn;
	}


	MODULEINFO getModuleInfoOfDll(LPCWSTR libraryName)
	{
		HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());
		HMODULE dllModule = GetModuleHandle(libraryName);
		MODULEINFO toReturn;
		if (nullptr == dllModule)
		{
			toReturn.lpBaseOfDll = nullptr;
		}
		else
		{
			if (!GetModuleInformation(processHandle, dllModule, &toReturn, sizeof(MODULEINFO)))
			{
				toReturn.lpBaseOfDll = nullptr;
			}
		}
		CloseHandle(processHandle);
		return toReturn;
	}


	HWND findMainWindow(unsigned long process_id)
	{
		handle_data data;
		data.process_id = process_id;
		data.best_handle = 0;
		EnumWindows(enumWindowsCallback, (LPARAM)&data);
		return data.best_handle;
	}


	uint8_t CharToByte(char c)
	{
		uint8_t b;
		sscanf_s(&c, "%hhx", &b);
		return b;
	}


	//bool DataCompare(LPBYTE image, LPBYTE bytePattern, char* patternMask)
	//{
	//	for (; *patternMask; ++patternMask, ++image, ++bytePattern)
	//	{
	//		if (*patternMask == 'x' && *image != *bytePattern)
	//		{
	//			return false;
	//		}
	//	}
	//	return (*patternMask) == 0;
	//}

	bool DataCompare(LPBYTE image, LPBYTE bytePattern, const char* patternMask)
	{
		// Optimize the comparison loop with early returns and better memory access patterns
		while (*patternMask)
		{
			// Only compare if this position requires an exact match
			if (*patternMask == 'x' && *image != *bytePattern)
				return false;

			++patternMask;
			++image;
			++bytePattern;
		}
		return true;
	}


	//// returns false if not found, true otherwise
	//bool findAOBPattern(LPBYTE imageAddress, DWORD imageSize, AOBBlock* const toScanFor)
	//{
	//	uint8_t firstByte = *(toScanFor->bytePattern());
	//	__int64 length = (__int64)imageAddress + imageSize - toScanFor->patternSize();

	//	LPBYTE startOfScan = imageAddress;
	//	for (int occurrence = 0; occurrence < toScanFor->occurrence(); occurrence++)
	//	{
	//		// reset the pointer found, as we're not interested in this occurrence, we need a following occurrence.
	//		LPBYTE currentAddress = nullptr;
	//		for (__int64 i = (__int64)startOfScan; i < length; i += 4)
	//		{
	//			unsigned x = *(unsigned*)(i);

	//			if ((x & 0xFF) == firstByte)
	//			{
	//				if (DataCompare(reinterpret_cast<uint8_t*>(i), toScanFor->bytePattern(), toScanFor->patternMask()))
	//				{
	//					currentAddress = reinterpret_cast<uint8_t*>(i);
	//					break;
	//				}
	//			}
	//			if ((x & 0xFF00) >> 8 == firstByte)
	//			{
	//				if (DataCompare(reinterpret_cast<uint8_t*>(i + 1), toScanFor->bytePattern(), toScanFor->patternMask()))
	//				{
	//					currentAddress = reinterpret_cast<uint8_t*>(i + 1);
	//					break;
	//				}
	//			}
	//			if ((x & 0xFF0000) >> 16 == firstByte)
	//			{
	//				if (DataCompare(reinterpret_cast<uint8_t*>(i + 2), toScanFor->bytePattern(), toScanFor->patternMask()))
	//				{
	//					currentAddress = reinterpret_cast<uint8_t*>(i + 2);
	//					break;
	//				}
	//			}
	//			if ((x & 0xFF000000) >> 24 == firstByte)
	//			{
	//				if (DataCompare(reinterpret_cast<uint8_t*>(i + 3), toScanFor->bytePattern(), toScanFor->patternMask()))
	//				{
	//					currentAddress = reinterpret_cast<uint8_t*>(i + 3);
	//					break;
	//				}
	//			}
	//		}
	//		if (nullptr == currentAddress)
	//		{
	//			// not found, give up
	//			return false;
	//		}
	//		// found an occurrence, store it
	//		toScanFor->storeFoundLocation(currentAddress);
	//		startOfScan = currentAddress + 1;	// otherwise we'll match ourselves. 
	//	}
	//	return true;
	//}
	bool findAOBPattern(LPBYTE imageAddress, DWORD imageSize, AOBBlock* const toScanFor)
	{
		// Cache frequently accessed values to avoid repeated function calls
		const uint8_t firstByte = *(toScanFor->bytePattern());
		const LPBYTE pattern = toScanFor->bytePattern();
		const char* mask = toScanFor->patternMask();
		const int patternSize = static_cast<int>(strlen(mask));

		// Calculate bounds once to avoid repeated calculations
		const LPBYTE endAddress = imageAddress + imageSize - patternSize;

		LPBYTE startOfScan = imageAddress;
		for (int occurrence = 0; occurrence < toScanFor->occurrence(); occurrence++)
		{
			LPBYTE currentAddress = nullptr;

			// Use a pointer for the scan position and increment it directly
			for (LPBYTE pos = startOfScan; pos <= endAddress; pos += 4)
			{
				// Read a 4-byte chunk at once (optimization for memory access)
				uint32_t chunk = *(uint32_t*)pos;

				// Check all 4 positions in the chunk for the first byte match
				// Optimize by using bit shifting and avoiding redundant calculations

				// Check position 0
				if ((chunk & 0xFF) == firstByte)
				{
					if (DataCompare(pos, pattern, mask))
					{
						currentAddress = pos;
						break;
					}
				}

				// Check position 1
				if (((chunk >> 8) & 0xFF) == firstByte)
				{
					if (DataCompare(pos + 1, pattern, mask))
					{
						currentAddress = pos + 1;
						break;
					}
				}

				// Check position 2
				if (((chunk >> 16) & 0xFF) == firstByte)
				{
					if (DataCompare(pos + 2, pattern, mask))
					{
						currentAddress = pos + 2;
						break;
					}
				}

				// Check position 3
				if (((chunk >> 24) & 0xFF) == firstByte)
				{
					if (DataCompare(pos + 3, pattern, mask))
					{
						currentAddress = pos + 3;
						break;
					}
				}
			}

			if (nullptr == currentAddress)
			{
				// Pattern not found for this occurrence
				return false;
			}

			// Found an occurrence, store it
			toScanFor->storeFoundLocation(currentAddress);
			startOfScan = currentAddress + 1;  // Move past this occurrence for next search
		}

		return true;
	}

	// locationData is the AOB block with the address of the rip relative value to read for the calculation.
	// nextOpCodeOffset is used to calculate the address of the next instruction as that's the address the rip relative value is relative off. In general
	// this is 4 (the size of the int32 for the rip relative value), but sometimes the rip relative value is inside an instruction following one or more bytes before the 
	// next instruction starts.
	LPBYTE calculateAbsoluteAddress(AOBBlock* locationData, int nextOpCodeOffset)
	{
		assert(locationData != nullptr);
		// ripRelativeValueAddress is the absolute address of a DWORD value which is a RIP relative offset. A calculation has to be performed on x64 to
		// calculate from that rip relative value and its location the real absolute address it refers to. That address is returned.
		LPBYTE ripRelativeValueAddress = locationData->locationInImage() + locationData->customOffset();
		return  ripRelativeValueAddress + nextOpCodeOffset + *((__int32*)ripRelativeValueAddress);
	}

	
	string formatString(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		string formattedArgs = formatStringVa(fmt, args);
		va_end(args);
		return formattedArgs;
	}


	string formatStringVa(const char* fmt, va_list args)
	{
		va_list args_copy;
		va_copy(args_copy, args);

		int len = vsnprintf(NULL, 0, fmt, args_copy);
		char* buffer = new char[len + 2];
		vsnprintf(buffer, len + 1, fmt, args_copy);
		string toReturn(buffer, len + 1);
		return toReturn;
	}


	bool stringStartsWith(const char *a, const char *b)
	{
		return strncmp(a, b, strlen(b)) == 0 ? 1 : 0;
	}

	
	bool keyDown(int virtualKeyCode)
	{
		return (GetKeyState(virtualKeyCode) & 0x8000);
	}

	
	bool altPressed()
	{
		return keyDown(VK_LMENU) || keyDown(VK_RMENU);
	}

	
	bool ctrlPressed()
	{
		return keyDown(VK_LCONTROL) || keyDown(VK_RCONTROL);
	}

	
	bool shiftPressed()
	{
		return keyDown(VK_LSHIFT) || keyDown(VK_RSHIFT);
	}

	
	std::string vkCodeToString(int vkCode)
	{
		if (vkCode > 255 || vkCode < 0)
		{
			return "";
		}
		string toReturn = vkCodeToStringLookup[vkCode];
		return toReturn;
	}

	
	float floatFromBytes(uint8_t byteArray[], DWORD arrayLength, int startIndex)
	{
		if(arrayLength < static_cast<DWORD>(startIndex) +4)
		{
			return -1.0f;
		}
		const auto floatInArray = reinterpret_cast<float*>(byteArray + startIndex);
		return *floatInArray;
	}

	double doubleFromBytes(uint8_t byteArray[], DWORD arrayLength, int startIndex)
	{
		if (arrayLength < static_cast<DWORD>(startIndex) + 4)
		{
			return -1.0;
		}
		const auto doubleInArray = reinterpret_cast<double*>(byteArray + startIndex);
		return *doubleInArray;
	}

	
	int intFromBytes(uint8_t byteArray[], DWORD arrayLength, int startIndex)
	{
		if (arrayLength < static_cast<DWORD>(startIndex) + 4)
		{
			return -1;
		}
		const auto intInArray = reinterpret_cast<int*>(byteArray + startIndex);
		return *intInArray;
	}

	uint8_t uintFromBytes(uint8_t byteArray[], DWORD arrayLength, int startIndex)
	{
		if (arrayLength < static_cast<DWORD>(startIndex) + 4)
		{
			return -1;
		}
		const auto uintInArray = (byteArray + startIndex);
		return *uintInArray;
	}
	
	std::string stringFromBytes(uint8_t byteArray[], DWORD arrayLength, int startIndex)
	{
		if (arrayLength < static_cast<DWORD>(startIndex) + 4)
		{
			return "";
		}
		const auto charInArray = reinterpret_cast<char*>(byteArray + startIndex);
		// copy over the bytes in our own array which has a trailing 0
		const auto characters = new char[(arrayLength - startIndex) + 1];
		memcpy(characters, charInArray, arrayLength - startIndex);
		characters[(arrayLength - startIndex)] = '\0';
		string toReturn(characters);
		return toReturn;
	}

	void toggleNOPState(const string& key, const int numberOfBytes, const bool enabled)
	{
		auto& aob = System::instance().getAOBBlock();

		if (!aob.contains(key))
		{
			MessageHandler::logError("No AOB block found with key: %s", key.c_str());
			return;
		}

		auto& hookData = aob[key];

		// Get target address
		const LPBYTE targetAddress = hookData.absoluteAddress();
		if (!targetAddress) {
			MessageHandler::logError("Invalid address for block: %s", hookData.getName().c_str());
			return;
		}

		// Initialize byte storage on first call
		if (hookData.byteStorage == nullptr) {
			hookData.byteStorage = new uint8_t[numberOfBytes];
			IGCS::GameImageHooker::readRange(targetAddress, hookData.byteStorage, numberOfBytes);
		}

		// Current state matches requested state - nothing to do
		if (hookData.nopState == enabled) {
			MessageHandler::logDebug("Block '%s' already in %s state",
				hookData.getName().c_str(),
				enabled ? "NOP" : "original");
			return;
		}

		// Apply requested state
		if (enabled) {
			GameImageHooker::nopRange(targetAddress, numberOfBytes);
			MessageHandler::logDebug("NOPs applied to block: %s", hookData.getName().c_str());
		}
		else {
			GameImageHooker::writeRange(targetAddress, hookData.byteStorage, numberOfBytes);
			MessageHandler::logDebug("Original bytes restored for block: %s", hookData.getName().c_str());
		}

		// Update state
		hookData.nopState = enabled;
	}

	void saveBytesWrite(const string& key, int numberOfBytes, const uint8_t* bytestoWrite, bool enabled)
	{
		auto& aob = System::instance().getAOBBlock();

		if (!aob.contains(key))
		{
			MessageHandler::logError("No AOB block found with key: %s", key.c_str());
			return;
		}

		auto& hookData = aob[key];

		// Get target address
		const LPBYTE targetAddress = hookData.absoluteAddress();
		if (!targetAddress) {
			MessageHandler::logError("Invalid address for block: %s", hookData.getName().c_str());
			return;
		}

		// Initialize byte storage on first call
		if (hookData.byteStorage2 == nullptr) {
			hookData.byteStorage2 = new uint8_t[numberOfBytes];
			GameImageHooker::readRange(targetAddress, hookData.byteStorage2, numberOfBytes);
		}

		// Current state matches requested state - nothing to do
		if (hookData.nopState2 == enabled) {
			MessageHandler::logDebug("Block '%s' already in %s state",
				hookData.getName().c_str(),
				enabled ? "custom" : "original");
			return;
		}

		// Apply requested state
		if (enabled) {
			GameImageHooker::writeRange(targetAddress, bytestoWrite, numberOfBytes);
			MessageHandler::logDebug("Custom bytes applied to block: %s", hookData.getName().c_str());
		}
		else {
			GameImageHooker::writeRange(targetAddress, hookData.byteStorage2, numberOfBytes);
			MessageHandler::logDebug("Original bytes restored for block: %s", hookData.getName().c_str());
		}

		// Update state
		hookData.nopState2 = enabled;
	}

	const char* getExecutableName()
	{
		static char executableName[MAX_PATH] = { 0 };

		// Only calculate this once
		if (executableName[0] == 0)
		{
			// Get the full path of the current executable
			char fullPath[MAX_PATH] = { 0 };
			GetModuleFileNameA(nullptr, fullPath, MAX_PATH);

			// Extract just the filename
			const char* lastSlash = strrchr(fullPath, '\\');
			const char* lastForwardSlash = strrchr(fullPath, '/');

			// Find the last directory separator (could be \ or /)
			const char* lastSeparator = lastSlash;
			if (lastForwardSlash > lastSlash)
				lastSeparator = lastForwardSlash;

			if (lastSeparator)
			{
				strcpy_s(executableName, sizeof(executableName), lastSeparator + 1);
			}
			else
			{
				// Fallback in case there's no separator
				strcpy_s(executableName, sizeof(executableName), fullPath);
			}

			// Convert to lowercase for case-insensitive comparisons
			for (char* p = executableName; *p; ++p) {
				const unsigned char uc = static_cast<unsigned char>(*p);    // 1) promote to unsigned char
				const int lowered = tolower(uc);							// tolower returns int
				*p = static_cast<char>(lowered);							// 2) explicit char narrowing
			}
			MessageHandler::logDebug("Executable name: %s", executableName);
		}

		return executableName;
	}

	// Wide character version (if needed for Unicode paths)
	const wchar_t* getExecutableNameW()
	{
		static wchar_t executableName[MAX_PATH] = { 0 };

		// Only calculate this once
		if (executableName[0] == 0)
		{
			// Get the full path of the current executable
			wchar_t fullPath[MAX_PATH] = { 0 };
			GetModuleFileNameW(nullptr, fullPath, MAX_PATH);

			// Extract just the filename
			const wchar_t* lastSlash = wcsrchr(fullPath, L'\\');
			const wchar_t* lastForwardSlash = wcsrchr(fullPath, L'/');

			// Find the last directory separator (could be \ or /)
			const wchar_t* lastSeparator = lastSlash;
			if (lastForwardSlash > lastSlash)
				lastSeparator = lastForwardSlash;

			if (lastSeparator)
			{
				wcscpy_s(executableName, sizeof(executableName) / sizeof(wchar_t), lastSeparator + 1);
			}
			else
			{
				// Fallback in case there's no separator
				wcscpy_s(executableName, sizeof(executableName) / sizeof(wchar_t), fullPath);
			}

			// Convert to lowercase for case-insensitive comparisons
			for (wchar_t* p = executableName; *p; ++p) {
				const unsigned char uc = static_cast<unsigned char>(*p);    // 1) promote to unsigned char
				const int lowered = tolower(uc);							// tolower returns int
				*p = static_cast<char>(lowered);							// 2) explicit char narrowing
			}
		}

		return executableName;
	}

	std::vector<uint8_t> intToBytes(const int integer)
	{
		std::vector<uint8_t> bytes(sizeof(integer));	// 4 bytes for a 32-bit integer
		std::memcpy(bytes.data(), &integer, sizeof(integer)); // Copy the bytes of the integer into the vector
		return bytes;
	}

	bool determineHandedness()
	{
		using namespace IGCS;
		using namespace DirectX;

		auto t = GameSpecific::CameraManipulator::getCameraStructAddress();
		auto q = reinterpret_cast<XMFLOAT4*>(t + QUATERNION_IN_STRUCT_OFFSET);
		//load the quaternion from memory into an XMVECTOR
		XMVECTOR orientationQuaternion = XMLoadFloat4(q);

		// 1. Derive the RAW, UNCORRECTED basis vectors
		XMVECTOR rightVec = XMVector3Rotate(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), orientationQuaternion);
		XMVECTOR upVec = XMVector3Rotate(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), orientationQuaternion);
		XMVECTOR forwardVec = XMVector3Rotate(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), orientationQuaternion);

		// 2. Calculate the scalar triple product on the RAW vectors
		XMVECTOR crossProduct = XMVector3Cross(upVec, forwardVec);
		XMVECTOR tripleProduct = XMVector3Dot(rightVec, crossProduct);

		// 4. A positive result indicates a Right-Handed system.
		const bool isRightHanded = XMVectorGetX(tripleProduct) > 0.0f;
		MessageHandler::logDebug("Coordinate system detected as: %s", isRightHanded ? "Right-Handed" : "Left-Handed");
		return isRightHanded;
	}
}