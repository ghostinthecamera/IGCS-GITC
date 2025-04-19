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
#include "OverlayControl.h"
#include "GameImageHooker.h"
#include "Camera.h"

using namespace DirectX;
using namespace std;

extern "C" {
	LPBYTE g_cameraStructAddress = nullptr;
	LPBYTE g_activecamAddress = nullptr;
	LPBYTE g_timescaleAddress = nullptr;
	LPBYTE g_HUDaddress = nullptr;
}



namespace IGCS::GameSpecific::CameraManipulator
{
	static float _originalCoords[3];
	static float _originalquaternion[4];
	static float _originalMatrix[12];
	static float _originalFoV;
	static float _originalNearPlane;

	void toggleHUD()
	{
		if (nullptr == g_HUDaddress)
		{
			return;
		}

		BYTE* HUDinMemory = reinterpret_cast<BYTE*>(g_HUDaddress + HUD_OFFSET);
		*HUDinMemory = *HUDinMemory == (BYTE)0 ? (BYTE)1 : (BYTE)0;
	}

	void timeStop(bool enabled) 
	{
		if (nullptr == g_timescaleAddress)
		{
			return;
		}

		float* timestopinmemory = reinterpret_cast<float*>(g_timescaleAddress + TIMESTOP_OFFSET);
		*timestopinmemory = (enabled) ? 0.00001f :1.0f;
	}

	void enableFOV()
	{
		if (g_activecamAddress == nullptr)
		{

		}
		BYTE* fovByte = reinterpret_cast<BYTE*>(g_activecamAddress + FOVBYTE_ACTIVECAM_OFFSET);
		*fovByte = *fovByte == (BYTE)0 ? (BYTE)1 : (BYTE)0;
	}


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
		if (g_activecamAddress == nullptr)
		{
			return;
		}
		float* fovAddress = reinterpret_cast<float*>(g_activecamAddress + FOV_ACTIVECAM_OFFSET);
		*fovAddress = _originalFoV;
	}

	float getCurrentFoV()
	{
		if (nullptr == g_activecamAddress)
		{
			return DEFAULT_FOV;
		}
		float* fovAddress = reinterpret_cast<float*>(g_activecamAddress + FOV_ACTIVECAM_OFFSET);
		return *fovAddress;
	}

	void setNearPlane()
	{
		if (g_activecamAddress == nullptr)
		{
			return;
		}

		float* nearPlaneInMemory = reinterpret_cast<float*>(g_activecamAddress + NEARPLANE);
		*nearPlaneInMemory = _originalNearPlane ? 0.01f : _originalNearPlane;
	}

	// changes the FoV with the specified amount
	void changeFoV(float amount)
	{
		if (g_activecamAddress == nullptr)
		{
			return;
		}
		float* fovAddress = reinterpret_cast<float*>(g_activecamAddress + FOV_ACTIVECAM_OFFSET);
		float newValue = *fovAddress;
		float multiplier = Utils::clamp(abs(newValue / _originalFoV), 0.01f,1.0f,1.0f);
	    newValue = Utils::clampSimple((newValue + (amount*multiplier)),0.02f,2.2f);
		*fovAddress = newValue;
	}


	XMFLOAT3 currentCoords()
	{
		//float* coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress);
		//XMFLOAT3 currentCoords = XMFLOAT3(coordsInMemory[3], coordsInMemory[7], coordsInMemory[11]);
		//return currentCoords;

		float* coordsInMemory = reinterpret_cast<float*>(g_activecamAddress+0x30);
		XMFLOAT3 currentCoords = XMFLOAT3(coordsInMemory[0], coordsInMemory[1], coordsInMemory[2]);
		return currentCoords;
	}

	XMFLOAT3 quatcoords()
	{
		float* quatinMemory = reinterpret_cast<float*>(g_activecamAddress + 0x20);

		float pitchinmem, yawinmem, rollinmem, q1, q2, q3, q4;
		q1 = quatinMemory[1];
		q2 = quatinMemory[2];
		q3 = quatinMemory[3];
		q4 = quatinMemory[0];

		pitchinmem = -asin(2 * (q1 * q4 - q3 * q2));
		yawinmem = -atan2(2 * (q1 * q3 + q2 * q4), 1 - 2 * (q4 * q4 + q3 * q3));
		rollinmem = -asin(2 * (q1 * q3 - q4 * q2));

		XMFLOAT3 coordstoreturn = XMFLOAT3(pitchinmem, yawinmem, rollinmem);
		return coordstoreturn;
	}


	void writeNewCameraValuesToGameData(XMFLOAT3 newCoords, XMVECTOR newLookQuaternion)
	{
		if (!isCameraFound())
		{
			return;
		}
		//XMFLOAT4X4 rotationMatrix, inversedmatrix;

		//XMMATRIX rotationMatrixPacked = DirectX::XMMatrixRotationQuaternion(newLookQuaternion);
		//XMStoreFloat4x4(&rotationMatrix, rotationMatrixPacked);

		//float* matrixInMemory = reinterpret_cast<float*>(g_cameraStructAddress);
		//matrixInMemory[0] = rotationMatrix._11; matrixInMemory[1] = rotationMatrix._21; matrixInMemory[2] = rotationMatrix._31; matrixInMemory[3] = newCoords.x;
		//matrixInMemory[4] = rotationMatrix._12; matrixInMemory[5] = rotationMatrix._22; matrixInMemory[6] = rotationMatrix._32; matrixInMemory[7] = newCoords.y;
		//matrixInMemory[8] = rotationMatrix._13; matrixInMemory[9] = rotationMatrix._23; matrixInMemory[10] = rotationMatrix._33; matrixInMemory[11] = newCoords.z;
	
		float Xvec, Yvec, Zvec, qx, qy, qz, qw;
		qx = XMVectorGetX(newLookQuaternion);
		qy = XMVectorGetY(newLookQuaternion);
		qz = XMVectorGetZ(newLookQuaternion);
		qw = XMVectorGetW(newLookQuaternion);

		//Xvec = 10*(2 * (qx * qz - qw * qy));
		//Yvec = 10*(2 * (qy * qz + qw * qx));
		//Zvec = 10*(1-2 * (qx * qx + qy * qy));

		//XMMATRIX inversem = DirectX::XMMatrixInverse(nullptr, rotationMatrixPacked);
		//XMStoreFloat4x4(&inversedmatrix, inversem);
		float* coordsinmemory = reinterpret_cast<float*>(g_activecamAddress + 0x30);
		float* quaternioninmemory = reinterpret_cast<float*>(g_activecamAddress + 0x20);

		/*memcpy(quaternioninmemory, &newLookQuaternion, (sizeof(float) * 4));*/
		quaternioninmemory[0] = qx;
		quaternioninmemory[1] = qy;
		quaternioninmemory[2] = qz;
		quaternioninmemory[3] = qw;

		memcpy(coordsinmemory, &newCoords, sizeof(XMFLOAT3));

	}


	bool isCameraFound()
	{
		return nullptr != g_activecamAddress;
	}


	void displayCameraStructAddress()
	{
		OverlayConsole::instance().logDebug("Camera struct address: %p", (void*)g_activecamAddress);
		OverlayConsole::instance().logDebug("Timescale struct address: %p", (void*)g_timescaleAddress);
		OverlayConsole::instance().logDebug("HUD struct address: %p", (void*)g_HUDaddress);
		OverlayConsole::instance().logDebug("ActiveCam struct address: %p", (void*)g_activecamAddress);
	}
	

	// should restore the camera values in the camera structures to the cached values. This assures the free camera is always enabled at the original camera location.
	void restoreOriginalValuesAfterCameraDisable()
	{
		float* matrixInMemory = nullptr;
		float *fovInMemory = nullptr;
		float* nearPlane = nullptr;

		if (!isCameraFound())
		{
			return;
		}
		// gameplay / cutscene cam
		matrixInMemory = reinterpret_cast<float*>(g_cameraStructAddress);
		memcpy(matrixInMemory, _originalMatrix, 12 * sizeof(float));

		fovInMemory = reinterpret_cast<float*>(g_activecamAddress + FOV_ACTIVECAM_OFFSET);
		*fovInMemory = _originalFoV;

		nearPlane = reinterpret_cast<float*>(g_activecamAddress + NEARPLANE);
		*nearPlane = _originalNearPlane;
	}


	void cacheOriginalValuesBeforeCameraEnable()
	{
		float* matrixInMemory = nullptr;
		float* quaternionInMemory = nullptr;
		float* coordsInMemory = nullptr;
		float* fovInMemory = nullptr;
		float* nearPlane = nullptr;


		if (!isCameraFound())
		{
			return;
		}
		//// gameplay/cutscene cam
		//matrixInMemory = reinterpret_cast<float*>(g_cameraStructAddress);
		//memcpy(_originalMatrix, matrixInMemory, 12 * sizeof(float));

		//fovInMemory = reinterpret_cast<float*>(g_activecamAddress + FOV_ACTIVECAM_OFFSET);
		//_originalFoV = *fovInMemory;

		//nearPlane = reinterpret_cast<float*>(g_activecamAddress + NEARPLANE);
		//_originalNearPlane = *nearPlane;

		// gameplay/cutscene cam
		quaternionInMemory = reinterpret_cast<float*>(g_activecamAddress+0x20);
		memcpy(_originalquaternion, quaternionInMemory, 4 * sizeof(float));

		coordsInMemory = reinterpret_cast<float*>(g_activecamAddress + 0x30);
		memcpy(_originalCoords, coordsInMemory, 3 * sizeof(float));

		fovInMemory = reinterpret_cast<float*>(g_activecamAddress + FOV_ACTIVECAM_OFFSET);
		_originalFoV = *fovInMemory;

		nearPlane = reinterpret_cast<float*>(g_activecamAddress + NEARPLANE);
		_originalNearPlane = *nearPlane;
	}

}