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
#pragma once
#include "stdafx.h"
#include "Gamepad.h"
#include "GameConstants.h"
#include "Defaults.h"
#include "CDataFile.h"

extern "C" BYTE g_cameraEnabled;
//extern "C" float g_fovValue;
extern "C" BYTE g_gamePaused;

namespace IGCS
{
	struct Settings
	{
		bool invertY;
		bool allowCameraMovementWhenMenuIsUp;
		bool hudandtimestop;
		float fastMovementMultiplier;
		float slowMovementMultiplier;
		float movementUpMultiplier;
		float movementSpeed;
		float rotationSpeed;
		float fovChangeSpeed;
		float resolutionScale;		//50.0-200.0
		int frameskip;
		float clampFloat(float value, float min, float default)
		{
			return value < min ? default : value;
		}

		void loadFromFile()
		{
			CDataFile iniFile;
			if (!iniFile.Load(IGCS_SETTINGS_INI_FILENAME))
			{
				// doesn't exist
				return;
			}
			invertY = iniFile.GetBool("invertY", "CameraSettings");
			hudandtimestop = iniFile.GetBool("hudandtimestop", "CameraSettings");
			allowCameraMovementWhenMenuIsUp = iniFile.GetBool("allowCameraMovementWhenMenuIsUp", "CameraSettings");
			fastMovementMultiplier = clampFloat(iniFile.GetFloat("fastMovementMultiplier", "CameraSettings"), 0.0f, FASTER_MULTIPLIER);
			slowMovementMultiplier = clampFloat(iniFile.GetFloat("slowMovementMultiplier", "CameraSettings"), 0.0f, SLOWER_MULTIPLIER);
			movementUpMultiplier = clampFloat(iniFile.GetFloat("movementUpMultiplier", "CameraSettings"), 0.0f, DEFAULT_UP_MOVEMENT_MULTIPLIER);
			movementSpeed = clampFloat(iniFile.GetFloat("movementSpeed", "CameraSettings"), 0.0f, DEFAULT_MOVEMENT_SPEED);
			rotationSpeed = clampFloat(iniFile.GetFloat("rotationSpeed", "CameraSettings"), 0.0f, DEFAULT_ROTATION_SPEED);
			fovChangeSpeed = clampFloat(iniFile.GetFloat("fovChangeSpeed", "CameraSettings"), 0.0f, DEFAULT_FOV_SPEED);
		}

		void saveToFile()
		{
			CDataFile iniFile;
			iniFile.SetBool("invertY", invertY, "", "CameraSettings");
			iniFile.SetBool("allowCameraMovementWhenMenuIsUp", allowCameraMovementWhenMenuIsUp, "", "CameraSettings");
			iniFile.SetBool("hudandtimestop", hudandtimestop, "", "CameraSettings");
			iniFile.SetFloat("fastMovementMultiplier", fastMovementMultiplier, "", "CameraSettings");
			iniFile.SetFloat("slowMovementMultiplier", slowMovementMultiplier, "", "CameraSettings");
			iniFile.SetFloat("movementUpMultiplier", movementUpMultiplier, "", "CameraSettings");
			iniFile.SetFloat("movementSpeed", movementSpeed, "", "CameraSettings");
			iniFile.SetFloat("rotationSpeed", rotationSpeed, "", "CameraSettings");
			iniFile.SetFloat("fovChangeSpeed", fovChangeSpeed, "", "CameraSettings");
			iniFile.SetFileName(IGCS_SETTINGS_INI_FILENAME);
			iniFile.Save();
		}

		void init()
		{
			invertY = CONTROLLER_Y_INVERT;
			fastMovementMultiplier = FASTER_MULTIPLIER;
			slowMovementMultiplier = SLOWER_MULTIPLIER;
			movementUpMultiplier = DEFAULT_UP_MOVEMENT_MULTIPLIER;
			movementSpeed = DEFAULT_MOVEMENT_SPEED;
			rotationSpeed = DEFAULT_ROTATION_SPEED;
			fovChangeSpeed = DEFAULT_FOV_SPEED;
			resolutionScale = 100.0f;
			frameskip = 15;
			allowCameraMovementWhenMenuIsUp = false;
			hudandtimestop = true;
		}
	};


	class Globals
	{
	public:
		Globals();
		~Globals();

		static Globals& instance();

		void saveSettingsIfRequired(float delta);
		void markSettingsDirty();

		bool inputBlocked() const { return _inputBlocked; }
		void inputBlocked(bool value) { _inputBlocked = value; }
		bool systemActive() const { return _systemActive; }
		void systemActive(bool value) { _systemActive = value; }
		HWND mainWindowHandle() const { return _mainWindowHandle; }
		void mainWindowHandle(HWND handle) { _mainWindowHandle = handle; }
		Gamepad& gamePad() { return _gamePad; }
		Settings& settings() { return _settings; }

	private:
		bool _inputBlocked = false;
		volatile bool _systemActive = false;
		Gamepad _gamePad;
		HWND _mainWindowHandle;
		Settings _settings;
		float _settingsDirtyTimer=0.0f;			// when settings are marked dirty, this is set with a value > 0 and decremented each frame. If 0, settings are saved. In seconds.
	};
}