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
#include "GameImageHooker.h"

using namespace DirectX;
using namespace std;

extern "C" {
	LPBYTE g_cameraStructAddress = nullptr;
	//LPBYTE g_resolutionScaleAddress = nullptr;
	//LPBYTE g_todStructAddress = nullptr;
	LPBYTE g_timestopStructAddress = nullptr;
	LPBYTE _effectivecamstruct = nullptr;
	bool _timeStopped;
}

namespace IGCS::GameSpecific::CameraManipulator
{
	static float _originalCoords[3];
	static float _originalQuaternion[4];
	static float _originalFoV;
	//static LPBYTE g_resolutionScaleMenuValueAddress = nullptr;

	void getEffectiveCam()
	{
		if (g_cameraStructAddress == nullptr)
		{
			return;
		}
		_effectivecamstruct = g_cameraStructAddress + CAM_STRUCT_BASE;
	}



	void writeEnableBytes()
	{
		BYTE statementbytes[4] = { 0x01, 0x01, 0x01, 0x01 };
		GameImageHooker::writeRange(g_cameraStructAddress + 0x324, statementbytes, 4);
	}
	//disabled - use ingame timestop
	/*void sloMoFunc(float amount)
	{
		if (nullptr == g_timestopStructAddress)
		{
			return;
		}
		if (!_timeStopped)
		{
			float* timescaleInMemory = reinterpret_cast<float*>(g_timestopStructAddress + TIMESTOP_OFFSET);
			*timescaleInMemory = *timescaleInMemory > 0.96f ? amount : 1.0f;
		}
	}

	void timeStop()
	{
		if (nullptr == g_timestopStructAddress)
		{
			return;
		}

		float* timescaleInMemory = reinterpret_cast<float*>(g_timestopStructAddress + TIMESTOP_OFFSET);
		*timescaleInMemory = *timescaleInMemory > 0.04f ? 0.0f : 1.0f;
	}*/


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
		float newValue = *fovAddress + amount;
		if (newValue < 0.001f)
		{
			// clamp. Game will crash with negative fov
			newValue = 0.001f;
		}
		*fovAddress = newValue;
	}
	

	XMFLOAT3 getCurrentCameraCoords()
	{
		// we write to both cameras at once, so we just grab one of the coords, it always works. Photomode does inherit its coords from the 
		// gameplay / current cam anyway. 
		float* coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		XMFLOAT3 currentCoords = XMFLOAT3(coordsInMemory[0], coordsInMemory[1], coordsInMemory[2]);
		return currentCoords;
	}


	// newLookQuaternion: newly calculated quaternion of camera view space. Can be used to construct a 4x4 matrix if the game uses a matrix instead of a quaternion
	// newCoords are the new coordinates for the camera in worldspace.
	void writeNewCameraValuesToGameData(XMFLOAT3 newCoords, XMVECTOR newLookQuaternion)
	{
		if (!isCameraFound())
		{
			return;
		}

		XMFLOAT4 qAsFloat4;
		XMStoreFloat4(&qAsFloat4, newLookQuaternion);

		float* coordsInMemory = nullptr;
		float* quaternionInMemory = nullptr;

		// only the gameplay camera. Photomode coords aren't updated.
		coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + +0x19C0);
		coordsInMemory[0] = newCoords.x;
		coordsInMemory[1] = newCoords.y;
		coordsInMemory[2] = newCoords.z;

		quaternionInMemory = reinterpret_cast<float*>(g_cameraStructAddress + QUATERNION_IN_STRUCT_OFFSET);
		quaternionInMemory[0] = qAsFloat4.x;
		quaternionInMemory[1] = qAsFloat4.y;
		quaternionInMemory[2] = qAsFloat4.z;
		quaternionInMemory[3] = qAsFloat4.w;
	}


	bool isCameraFound()
	{
		return nullptr != g_cameraStructAddress;
	}


	void displayCameraStructAddress()
	{
		OverlayConsole::instance().logDebug("Camera struct address: %p", (void*)g_cameraStructAddress);
		LPBYTE g_structaddress = (g_cameraStructAddress + CAM_STRUCT_BASE);
		OverlayConsole::instance().logDebug("Camera struct address: %p", (void*)g_structaddress);
	}
	

	// should restore the camera values in the camera structures to the cached values. This assures the free camera is always enabled at the original camera location.
	void restoreOriginalValuesAfterCameraDisable()
	{
		float* quaternionInMemory = nullptr;
		float* coordsInMemory = nullptr;
		float *fovInMemory = nullptr;

		if (!isCameraFound())
		{
			return;
		}
		// gameplay / cutscene cam
		quaternionInMemory = reinterpret_cast<float*>(g_cameraStructAddress + QUATERNION_IN_STRUCT_OFFSET);
		coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		memcpy(quaternionInMemory, _originalQuaternion, 4 * sizeof(float));
		memcpy(coordsInMemory, _originalCoords, 3 * sizeof(float));

		fovInMemory = reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET);
		*fovInMemory = _originalFoV;
	/*	if (nullptr != g_resolutionScaleMenuValueAddress && nullptr!=g_resolutionScaleAddress)
		{
			float* resolutionScaleInMemory = reinterpret_cast<float*>(g_resolutionScaleAddress + RESOLUTION_SCALE_IN_STRUCT_OFFSET);
			float* resolutionScaleMenuInMemory = reinterpret_cast<float*>(g_resolutionScaleMenuValueAddress);
			*resolutionScaleInMemory = *resolutionScaleMenuInMemory;
		}*/
	}


	void cacheOriginalValuesBeforeCameraEnable()
	{
		float* quaternionInMemory = nullptr;
		float* coordsInMemory = nullptr;
		float *fovInMemory = nullptr;

		if (!isCameraFound())
		{
			return;
		}
		// gameplay/cutscene cam
		quaternionInMemory = reinterpret_cast<float*>(g_cameraStructAddress + QUATERNION_IN_STRUCT_OFFSET);
		coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		memcpy(_originalQuaternion, quaternionInMemory, 4 * sizeof(float));
		memcpy(_originalCoords, coordsInMemory, 3 * sizeof(float));

		fovInMemory = reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET);
		_originalFoV = *fovInMemory;
	}
}