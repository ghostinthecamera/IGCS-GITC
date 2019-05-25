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
#include "InterceptorHelper.h"
#include "Globals.h"
#include "OverlayConsole.h"

using namespace DirectX;
using namespace std;

extern "C" {
	LPBYTE g_cameraStructAddress = nullptr;
	LPBYTE g_timestopStructAddress = nullptr;
	LPBYTE g_resolutionscaleStructAddress = nullptr;
}

namespace IGCS::GameSpecific::CameraManipulator
{
	static LPBYTE _hostImageAddress = nullptr;
	static float _originalCoords[3];
	static float _originalAngles[3];
	static float _currentCameraCoords[3];
	static float _resolutionScaleCache;

	void setImageAddress(LPBYTE hostImageAddress)
	{
		_hostImageAddress = hostImageAddress;
	}
	
	// Resets the FOV to the one it got when we enabled the camera
	void resetFoV()
	{
		if (nullptr == g_cameraStructAddress)
		{
			return;
		}
		float* fovInMemory = reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET);
		*fovInMemory = DEFAULT_FOV_DEGREES;
	}


	// changes the FoV with the specified amount
	void changeFoV(float amount)
	{
		if (nullptr == g_cameraStructAddress)
		{
			return;
		}
		float* fovInMemory = reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET);
		float newValue = *fovInMemory + amount;
		if (newValue < 0.001f)
		{
			// clamp. 
			newValue = 0.001f;
		}
		*fovInMemory = newValue;

	}


	// timestop
	void timeStop()
	{
		if (nullptr == g_timestopStructAddress)
		{
			return;
		}
		float* timescaleInMemory = reinterpret_cast<float*>(g_timestopStructAddress + TIMESTOP_IN_STRUCT_OFFSET);
		*timescaleInMemory = *timescaleInMemory > 0.04f ? 0.0f : 1.0f;
	}

	void sloMoFunc(float amount)
	{
		if (nullptr == g_timestopStructAddress)
		{
			return;
		}
		if (!g_gamePaused)
		{
			float* timescaleInMemory = reinterpret_cast<float*>(g_timestopStructAddress + TIMESTOP_IN_STRUCT_OFFSET);
			*timescaleInMemory = *timescaleInMemory > 0.96f ? amount : 1.0f;
		}
	}

	void plusFrame(int amount)
	{
		float* timescaleInMemory = reinterpret_cast<float*>(g_timestopStructAddress + TIMESTOP_IN_STRUCT_OFFSET);
		if (g_gamePaused)
		{
			*timescaleInMemory = 1.0f;
			Sleep(amount);
			*timescaleInMemory = 0.0f;
		}
	}

	void hudToggle()
	{
		float* hud1InMemory = reinterpret_cast<float*>(_hostImageAddress + HUD_TOGGLE_1);
		//OverlayConsole::instance().logDebug("HUD Toggle 1 Address: %p", (void*)hud1InMemory);
		//OverlayConsole::instance().logDebug("HUD Toggle 1 value: %f", (float)*hud1InMemory);
		*hud1InMemory = *hud1InMemory > 0.1f ? 0.0f : 1.0f;
		//OverlayConsole::instance().logDebug("HUD Toggle 1 value: %f", (float)*hud1InMemory);

		BYTE* hud2InMemory = reinterpret_cast<BYTE*>(_hostImageAddress + HUD_TOGGLE_2);
		//OverlayConsole::instance().logDebug("HUD Toggle 2 Address: %p", (void*)hud2InMemory);
		//OverlayConsole::instance().logDebug("HUD Toggle 2 Value: %x", (BYTE)*hud2InMemory);
		*hud2InMemory = *hud2InMemory == (BYTE)1 ? (BYTE)0 : (BYTE)1;
		//OverlayConsole::instance().logDebug("HUD Toggle 1 value: %f", (float)*hud1InMemory);
	}

	void hudOn()
	{
		float* hud1InMemory = reinterpret_cast<float*>(_hostImageAddress + HUD_TOGGLE_1);
		*hud1InMemory = 1.0f;

		BYTE* hud2InMemory = reinterpret_cast<BYTE*>(_hostImageAddress + HUD_TOGGLE_2);
		*hud2InMemory = (BYTE)1;
	}

	void hudOff()
	{
		float* hud1InMemory = reinterpret_cast<float*>(_hostImageAddress + HUD_TOGGLE_1);
		*hud1InMemory = 0.0f;

		BYTE* hud2InMemory = reinterpret_cast<BYTE*>(_hostImageAddress + HUD_TOGGLE_2);
		*hud2InMemory = (BYTE)0;
	}


	void getSettingsFromGameState()
	{
		Settings& currentSettings = Globals::instance().settings();
		if (nullptr != g_resolutionscaleStructAddress)
		{
			float* resolutionScaleInMemory = reinterpret_cast<float*>(g_resolutionscaleStructAddress);
			currentSettings.resolutionScale = *resolutionScaleInMemory;
		}
	}

	void applySettingsToGameState()
	{
		Settings& currentSettings = Globals::instance().settings();
		if (nullptr != g_resolutionscaleStructAddress)
		{
			float* resolutionScaleInMemory = reinterpret_cast<float*>(g_resolutionscaleStructAddress);
			*resolutionScaleInMemory = Utils::clamp(currentSettings.resolutionScale, RESOLUTION_SCALE_MIN, RESOLUTION_SCALE_MAX, 1.0f);
		}
	}

	XMFLOAT3 getCurrentCameraCoords()
	{
		return XMFLOAT3(_currentCameraCoords[0], _currentCameraCoords[1], _currentCameraCoords[2]);
	}
	

	// newCoords are the new coordinates for the camera in worldspace.
	// yaw, pitch, roll are the angles used for the rotation as they're now defined in the camera. These angles have to be written as floats (in degrees)
	// pitch, yaw, roll (they store it as 'x rotation', 'y rotation', 'z rotation'). 
	void writeNewCameraValuesToGameData(XMFLOAT3 newCoords, float pitch, float yaw, float roll)
	{
		if (nullptr == g_cameraStructAddress)
		{
			return;
		}
		// this camera is stored as 3 coords (floats) and 3 angles (floats). 
		float* coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		coordsInMemory[0] = newCoords.x;
		coordsInMemory[1] = newCoords.y;
		coordsInMemory[2] = newCoords.z;
		float* anglesInMemory = reinterpret_cast<float*>(g_cameraStructAddress + ANGLES_IN_STRUCT_OFFSET);
		anglesInMemory[0] = pitch;
		anglesInMemory[1] = yaw;
		anglesInMemory[2] = roll;

		_currentCameraCoords[0] = newCoords.x;
		_currentCameraCoords[1] = newCoords.y;
		_currentCameraCoords[2] = newCoords.z;
	}


	bool isCameraFound()
	{
		return nullptr != g_cameraStructAddress;
	}


	void displayCameraStructAddress()
	{
		LPBYTE hudoffset = _hostImageAddress + HUD_TOGGLE_1;
		OverlayConsole::instance().logDebug("Camera struct address: %p", (void*)g_cameraStructAddress);
		OverlayConsole::instance().logDebug("HUD Toggle address: %p", (void*)hudoffset);
		OverlayConsole::instance().logDebug("HUD Toggle value: %f", (void*)*hudoffset);
	}
	

	// should restore the camera values in the camera structures to the cached values. This assures the free camera is always enabled at the original camera location.
	void restoreOriginalCameraValues()
	{
		float* coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress+ COORDS_IN_STRUCT_OFFSET);
		memcpy(coordsInMemory, _originalCoords, 3 * sizeof(float));
		float* anglesInMemory = reinterpret_cast<float*>(g_cameraStructAddress + ANGLES_IN_STRUCT_OFFSET);
		memcpy(anglesInMemory, _originalAngles, 3 * sizeof(float));

		if (nullptr != g_resolutionscaleStructAddress);
		{
			float* resolutionScaleInMemory = reinterpret_cast<float*>(g_resolutionscaleStructAddress);
			*resolutionScaleInMemory = _resolutionScaleCache;
		}
	}


	void cacheOriginalCameraValues()
	{
		float* coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		memcpy(_originalCoords, coordsInMemory, 3 * sizeof(float));
		_currentCameraCoords[0] = _originalCoords[0];		// x
		_currentCameraCoords[1] = _originalCoords[1];		// y
		_currentCameraCoords[2] = _originalCoords[2];		// z

		float* anglesInMemory = reinterpret_cast<float*>(g_cameraStructAddress + ANGLES_IN_STRUCT_OFFSET);
		memcpy(_originalAngles, anglesInMemory, 3 * sizeof(float));

		if (nullptr != g_resolutionscaleStructAddress);
		{
			float* resolutionScaleInMemory = reinterpret_cast<float*>(g_resolutionscaleStructAddress);
			_resolutionScaleCache = *resolutionScaleInMemory;
		}
	}
}