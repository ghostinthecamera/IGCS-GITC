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
#include "CameraManipulator.h"
#include "GameConstants.h"
#include "InterceptorHelper.h"
#include "Globals.h"
#include "OverlayConsole.h"

using namespace DirectX;
using namespace std;

extern "C" {
	LPBYTE g_cameraStructAddress = nullptr;
	LPBYTE g_cameraStructAddress2 = nullptr;
}

namespace IGCS::GameSpecific::CameraManipulator
{
	static float _originalCoords[3];
	static float _originalCoords2[3];
	static float _originalMatrix[12];
	static float _originalQuaternion[4];
	static float _originalFoV = DEFAULT_FOV;

	void getSettingsFromGameState()
	{
		Settings& currentSettings = Globals::instance().settings();
	}


	void applySettingsToGameState()
	{
		Settings& currentSettings = Globals::instance().settings();
	}


	// Resets the FOV to the one it got when we enabled the camera
	void resetFoV()
	{
		if (g_cameraStructAddress == nullptr)
		{
			return;
		}
		float* fovAddress = reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET);
		*fovAddress = _originalFoV;
	}


	// changes the FoV with the specified amount
	void changeFoV(float amount)
	{
		if (g_cameraStructAddress == nullptr)
		{
			return;
		}
		float* fovAddress = reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET);
		float newValue = *fovAddress*(180 / XM_PI) + amount; //convert to degrees as we use degrees here but games uses radians
		if (newValue < 0.001f)
		{
			// lower clamp. 
			newValue = 0.001f;
		}
		if (newValue > 120.0f) {
			// upper clamp.
			newValue = 120.0f;
		}
		*fovAddress = newValue*(XM_PI/180);  //convert back to radians and write to game memory
	}
	

	XMFLOAT3 getCurrentCameraCoords()
	{
		float* coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		XMFLOAT3 currentCoords = XMFLOAT3(coordsInMemory[0], coordsInMemory[1], coordsInMemory[2]);
		return currentCoords;
	}

	XMFLOAT3 getCurrentDustCameraCoords()
	{
		float* dustcoordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress2 + COORDS_IN_STRUCT2_OFFSET);
		XMFLOAT3 currentDustCoords = XMFLOAT3(dustcoordsInMemory[0], dustcoordsInMemory[1], dustcoordsInMemory[2]);
		return currentDustCoords;
	}


	// newLookQuaternion: newly calculated quaternion of camera view space. Can be used to construct a 4x4 matrix if the game uses a matrix instead of a quaternion
	// newCoords are the new coordinates for the camera in worldspace.
	void writeNewCameraValuesToGameData(XMFLOAT3 newCoords, XMVECTOR newLookQuaternion)
	{
		if (!isCameraFound())
		{
			return;
		}

		XMMATRIX rotationMatrixPacked = XMMatrixRotationQuaternion(newLookQuaternion);
		XMFLOAT4X4 rotationMatrix;
		XMStoreFloat4x4(&rotationMatrix, rotationMatrixPacked);

		float* coordsInMemory = nullptr;
		float* matrixInMemory = nullptr;

		coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		coordsInMemory[0] = newCoords.x;
		coordsInMemory[1] = newCoords.y;
		coordsInMemory[2] = newCoords.z;

		//Game calculates a 3x3 matrix from internal quaternions and uses the matrix for the current replay camera.
		//Matrix is ordered y,x,z
		matrixInMemory = reinterpret_cast<float*>(g_cameraStructAddress + MATRIX_IN_STRUCT_OFFSET);
		matrixInMemory[0] = rotationMatrix._12;
		matrixInMemory[1] = rotationMatrix._22;
		matrixInMemory[2] = rotationMatrix._32;
		matrixInMemory[3] = 0.0f;	//boundary has a zero rather than 1
		matrixInMemory[4] = rotationMatrix._11;
		matrixInMemory[5] = rotationMatrix._21;
		matrixInMemory[6] = rotationMatrix._31;
		matrixInMemory[7] = 0.0f;
		matrixInMemory[8] = rotationMatrix._13;
		matrixInMemory[9] = rotationMatrix._23;
		matrixInMemory[10] = rotationMatrix._33;
		matrixInMemory[11] = 0.0f;
	}

	void writeNewDustCameraValuesToGameData(XMFLOAT3 newCoords, XMVECTOR newLookQuaternion)
	{
		if (!isCameraFound())
		{
			return;
		}

		XMFLOAT4 qAsFloat4;
		XMStoreFloat4(&qAsFloat4, newLookQuaternion);

		float* coords2InMemory = nullptr;
		float* quaternion2InMemory = nullptr;

		coords2InMemory = reinterpret_cast<float*>(g_cameraStructAddress2 + COORDS_IN_STRUCT2_OFFSET);
		coords2InMemory[0] = newCoords.x;
		coords2InMemory[1] = newCoords.y;
		coords2InMemory[2] = newCoords.z;

		quaternion2InMemory = reinterpret_cast<float*>(g_cameraStructAddress2 + QUATERNION_IN_STRUCT2_OFFSET);
		quaternion2InMemory[0] = qAsFloat4.x;
		quaternion2InMemory[1] = qAsFloat4.y;
		quaternion2InMemory[2] = qAsFloat4.z;
		quaternion2InMemory[3] = qAsFloat4.w;
	}


	bool isCameraFound()
	{
		return nullptr != g_cameraStructAddress;
	}


	void displayCameraStructAddress()
	{
		OverlayConsole::instance().logDebug("Camera struct address: %p", (void*)g_cameraStructAddress);
		OverlayConsole::instance().logDebug("Camera struct address: %p", (void*)g_cameraStructAddress2);
	}
	

	// should restore the camera values in the camera structures to the cached values. This assures the free camera is always enabled at the original camera location.
	void restoreOriginalValuesAfterCameraDisable()
	{
		float* matrixInMemory = nullptr;
		float* coordsInMemory = nullptr;
		float *fovInMemory = nullptr;

		if (!isCameraFound())
		{
			return;
		}
		matrixInMemory = reinterpret_cast<float*>(g_cameraStructAddress + MATRIX_IN_STRUCT_OFFSET);
		coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		memcpy(matrixInMemory, _originalMatrix, 12 * sizeof(float));
		memcpy(coordsInMemory, _originalCoords, 3 * sizeof(float));

		fovInMemory = reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET);
		*fovInMemory = _originalFoV;

		float* quaternion2InMemory = nullptr;
		float* coords2InMemory = nullptr;

		if (!isCameraFound())
		{
			return;
		}
		quaternion2InMemory = reinterpret_cast<float*>(g_cameraStructAddress2 + QUATERNION_IN_STRUCT2_OFFSET);
		coords2InMemory = reinterpret_cast<float*>(g_cameraStructAddress2 + COORDS_IN_STRUCT2_OFFSET);
		memcpy(quaternion2InMemory, _originalQuaternion, 4 * sizeof(float));
		memcpy(coords2InMemory, _originalCoords2, 3 * sizeof(float));

	}


	void cacheOriginalValuesBeforeCameraEnable()
	{
		float* matrixInMemory = nullptr;
		float* coordsInMemory = nullptr;
		float *fovInMemory = nullptr;

		if (!isCameraFound())
		{
			return;
		}
		matrixInMemory = reinterpret_cast<float*>(g_cameraStructAddress + MATRIX_IN_STRUCT_OFFSET);
		coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		memcpy(_originalMatrix, matrixInMemory, 12 * sizeof(float));
		memcpy(_originalCoords, coordsInMemory, 3 * sizeof(float));

		fovInMemory = reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET);
		_originalFoV = *fovInMemory;

		float* quaternion2InMemory = nullptr;
		float* coords2InMemory = nullptr;

		if (!isCameraFound())
		{
			return;
		}
		quaternion2InMemory = reinterpret_cast<float*>(g_cameraStructAddress2 + QUATERNION_IN_STRUCT2_OFFSET);
		coords2InMemory = reinterpret_cast<float*>(g_cameraStructAddress2 + COORDS_IN_STRUCT2_OFFSET);
		memcpy(_originalQuaternion, quaternion2InMemory, 4 * sizeof(float));
		memcpy(_originalCoords2, coords2InMemory, 3 * sizeof(float));
	}
}