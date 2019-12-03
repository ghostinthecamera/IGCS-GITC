////////////////////////////////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System
// Copyright(c) 2017, Frans Bouma
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
#include "CameraManipulator.h"
#include "GameConstants.h"
#include "Console.h"
#include "InterceptorHelper.h"
#include "Globals.h"

using namespace DirectX;
using namespace std;

extern "C" {
	LPBYTE g_cameraStructAddress = nullptr;
	LPBYTE g_fovConstructAddress = nullptr;
}

namespace IGCS::GameSpecific::CameraManipulator
{
	static float _originalCoordsData[3], _originalLookData[4],_currentCameraCoords[3];

	// Resets the FOV to the default
	void resetFoV()
	{
		if (nullptr == g_fovConstructAddress)
		{
			return;
		}
		float* fovInMemory = reinterpret_cast<float*>(g_fovConstructAddress + FOV_IN_STRUCT_OFFSET);
		*fovInMemory = DEFAULT_FOV_DEGREES;
	}


	// changes the FoV with the specified amount
	void changeFoV(float amount)
	{
		if (nullptr == g_fovConstructAddress)
		{
			return;
		}
		float* fovInMemory = reinterpret_cast<float*>(g_fovConstructAddress + FOV_IN_STRUCT_OFFSET);
		*fovInMemory += amount;
	}


	XMFLOAT3 getCurrentCameraCoords()
	{
		return XMFLOAT3(_currentCameraCoords[0], _currentCameraCoords[1], _currentCameraCoords[2]);
	}
	

	void writeNewCameraValuesToGameData(XMFLOAT3 newCoords, XMVECTOR newLookQuaternion)
	{
		if (nullptr == g_cameraStructAddress)
		{
			return;
		}

		XMFLOAT4 qAsFloat4;
		XMStoreFloat4(&qAsFloat4, newLookQuaternion);

		float* lookQInMemory = reinterpret_cast<float*>(g_cameraStructAddress + LOOK_DATA_IN_CAMERA_STRUCT_OFFSET);
		float* lookQInMemoryFourth = reinterpret_cast<float*>(g_cameraStructAddress + LOOK_DATA_SECOND_QAUTERNION);
		float* coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + CAMERA_COORDS_IN_CAMERA_STRUCT_OFFSET);

		coordsInMemory[0] = newCoords.x;
		coordsInMemory[1] = newCoords.y;
		coordsInMemory[2] = newCoords.z;

		lookQInMemoryFourth[0] = qAsFloat4.x;
		lookQInMemoryFourth[1] = qAsFloat4.y;
		lookQInMemoryFourth[2] = qAsFloat4.z;
		lookQInMemoryFourth[3] = qAsFloat4.w;

		lookQInMemory[0] = qAsFloat4.x;
		lookQInMemory[1] = qAsFloat4.y;
		lookQInMemory[2] = qAsFloat4.z;
		lookQInMemory[3] = qAsFloat4.w;

		_currentCameraCoords[0] = newCoords.x;
		_currentCameraCoords[1] = newCoords.y;
		_currentCameraCoords[2] = newCoords.z;
	}


	// Waits for the interceptor to pick up the camera struct address. Should only return if address is found 
	void waitForCameraStructAddresses(LPBYTE hostImageAddress)
	{
		Console::WriteLine("Waiting for camera struct interception...");
		while (nullptr == g_cameraStructAddress)
		{
			Sleep(100);
		}
		Console::WriteLine("Camera found.");

#ifdef _DEBUG
		cout << "Camera struct address: " << hex << (void*)g_cameraStructAddress << endl;
//		cout << "Second quaternion address: " << hex << (void*)(g_secondQuaternion + LOOK_DATA_SECOND_QAUTERNION) << endl;
#endif
	}


	// should restore the camera values in the camera structures to the cached values. This assures the free camera is always enabled at the original camera location.
	void restoreOriginalCameraValues()
	{
		float* coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + CAMERA_COORDS_IN_CAMERA_STRUCT_OFFSET);
		memcpy(coordsInMemory, _originalCoordsData, 3 * sizeof(float));
		float* lookQInMemory = reinterpret_cast<float*>(g_cameraStructAddress + LOOK_DATA_IN_CAMERA_STRUCT_OFFSET);
		memcpy(lookQInMemory, _originalLookData, 4 * sizeof(float));
	}


	void cacheOriginalCameraValues()
	{
		float* coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + CAMERA_COORDS_IN_CAMERA_STRUCT_OFFSET);
		memcpy(_originalCoordsData, coordsInMemory, 3 * sizeof(float));
		memcpy(_currentCameraCoords, coordsInMemory, 3 * sizeof(float));
		float* lookQInMemory = reinterpret_cast<float*>(g_cameraStructAddress + LOOK_DATA_IN_CAMERA_STRUCT_OFFSET);
		memcpy(_originalLookData, lookQInMemory, 4 * sizeof(float));
	}
}