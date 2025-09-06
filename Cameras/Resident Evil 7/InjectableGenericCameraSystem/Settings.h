////////////////////////////////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System
// Copyright(c) 2020, Frans Bouma
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

#include "CameraManipulator.h"
#include "D3D12Hook.h"
#include "Gamepad.h"
#include "Utils.h"
#include "GameConstants.h"
#include "Defaults.h"
#include "D3DHook.h"
#include "MemoryPatcher.h"
#include "System.h"

namespace IGCS
{
	struct Settings
	{
		bool invertY;
		float fastMovementMultiplier;
		float slowMovementMultiplier;
		float movementUpMultiplier;
		float movementSpeed;
		float rotationSpeed;
		float fovChangeSpeed;
		int cameraControlDevice;		// 0==keyboard/mouse, 1 == gamepad, 2 == both, see Defaults.h
		float gameSpeed;
		int hsWidth;
		int hsHeight;
		float movementSmoothness;
		float rotationSmoothness;
		float fovSmoothness;
		//PathController Setting
		int pathDurationSetting;
		float easingValueSetting;
		int sampleCountSetting = 256;
		bool easingEnabled;
		uint8_t positionInterpMode;
		uint8_t easingtype;
		float alphaValue;
		int eulerOrder;
		bool uniformParamEnabled;
		uint8_t _rotationMode;
		uint8_t deltaType;
		float fixedDelta;
		bool isCameraShakeEnabled;
		float shakeAmplitude;
		float shakeFrequency;
		bool isHandheldEnabled;
		float handheldIntensity;
		float handheldDriftIntensity;
		float handheldDriftSpeed;
		float handheldRotationDriftSpeed;
		float handheldJitterIntensity;
		float handheldBreathingIntensity;
		float handheldBreathingRate;
		bool handheldPositionToggle;
		bool handheldRotationToggle;
		bool isRelativePlayerEnabled;
		float relativePathSmoothness;
		bool waitBeforePlaying;
		bool unpauseOnPlay;
		bool togglePathVisualisation;
		bool toggleDisplayAllPaths;
		bool d3ddisabled;
		bool lookAtEnabled;
		bool pathLookAtEnabled;
		float pathLookAtOffsetX;
		float pathLookAtOffsetY;
		float pathLookAtOffsetZ;
		float pathLookAtSmoothness;
		// Path speed matching settings
		bool pathSpeedMatchingEnabled;
		float pathSpeedScale;           // Multiplier for how much vehicle speed affects path speed
		float pathSpeedSmoothness;      // How quickly path speed changes (higher = faster response)
		float pathMinSpeed;             // Minimum path speed multiplier (never completely stop)
		float pathMaxSpeed;             // Maximum path speed multiplier (cap for very fast vehicles)
		float pathBaselineSpeed;        // Vehicle speed that corresponds to normal path speed (1.0x)
		// Normal Camera Shake
		bool isCameraShakeEnabledB;
		float shakeAmplitudeB;
		float shakeFrequencyB;
		bool isHandheldEnabledB;
		float handheldIntensityB;
		float handheldDriftIntensityB;
		float handheldDriftSpeedB;
		float handheldRotationDriftSpeedB;
		float handheldJitterIntensityB;
		float handheldBreathingIntensityB;
		float handheldBreathingRateB;
		bool handheldPositionToggleB;
		bool handheldRotationToggleB;
		//game specific
		bool disableFlashlight;
		bool disableVignette;

		


		
		void setValueFromMessage(uint8_t payload[], DWORD payloadLength)
		{
			// byte 1 is the id of the setting. Bytes 2 and further contain the data for the setting.
			if(payloadLength<3)
			{
				return;
			}
			switch(static_cast<SettingType>(payload[1]))
			{
			case SettingType::FastMovementMultiplier:
				fastMovementMultiplier = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::SlowMovementMultiplier:
				slowMovementMultiplier = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::UpMovementMultiplier:
				movementUpMultiplier= Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::MovementSpeed:
				movementSpeed= Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::CameraControlDevice:
				cameraControlDevice = Utils::intFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::RotationSpeed:
				rotationSpeed = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::InvertYLookDirection:
				invertY = payload[2] == static_cast<uint8_t>(1);
				break;
			case SettingType::FoVZoomSpeed:
				fovChangeSpeed= Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::gameSpeed:
				gameSpeed = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::MovementSmoothness:
				movementSmoothness = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::RotationSmoothness:
				rotationSmoothness = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::FOVSmoothness:
				fovSmoothness = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			//PathController
			case SettingType::PathDuration:
				pathDurationSetting = Utils::intFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::PathEasingValue:
				easingValueSetting = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::PathSampleCount:
				sampleCountSetting = Utils::intFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::PathToggleInterpolationMode:
				positionInterpMode = Utils::uintFromBytes(payload, payloadLength, 2);

				if (d3ddisabled)
					return;

				if (!PathManager::instance()._pathManagerState)
					return;

				PathManager::D3DHookChecks();
				break;
			case SettingType::PathEasingType:
				easingtype = Utils::uintFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::EulerOrder:
				eulerOrder = Utils::intFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::UniformParam:
				uniformParamEnabled = payload[2] == static_cast<uint8_t>(1);
				break;
			case SettingType::RotationMode:
				_rotationMode = Utils::uintFromBytes(payload, payloadLength, 2);

				if (d3ddisabled)
					return;

				if (!PathManager::instance()._pathManagerState)
					return;
				
				PathManager::D3DHookChecks();
				break;
			case SettingType::DeltaType:
				deltaType = payload[2] == static_cast<uint8_t>(1);
				break;
			case SettingType::DeltaValue:
				fixedDelta = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::CameraShakeToggle:
				isCameraShakeEnabled = payload[2] == static_cast<uint8_t>(1);
				break;
			case SettingType::Amplitude:
				shakeAmplitude = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::Frequency:
				shakeFrequency = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::HandheldCameraToggle:
				isHandheldEnabled = payload[2] == static_cast<uint8_t>(1);
				break;
			case SettingType::HandheldIntensity:
				handheldIntensity = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::HandheldDriftIntensity:
				handheldDriftIntensity = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::HandheldJitterIntensity:
				handheldJitterIntensity = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::HandheldBreathingIntensity:
				handheldBreathingIntensity = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::HandheldBreatingRate:
				handheldBreathingRate = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::HandheldDriftSpeed:
				handheldDriftSpeed = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::HandheldRotationDriftSpeed:
				handheldRotationDriftSpeed = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::HandheldPositionToggle:
				handheldPositionToggle = payload[2] == static_cast<uint8_t>(1);
				break;
			case SettingType::HandheldRotationToggle:
				handheldRotationToggle = payload[2] == static_cast<uint8_t>(1);
				break;
			case SettingType::CameraShakeToggleB:
				isCameraShakeEnabledB = payload[2] == static_cast<uint8_t>(1);
				break;
			case SettingType::AmplitudeB:
				shakeAmplitudeB = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::FrequencyB:
				shakeFrequencyB = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::HandheldCameraToggleB:
				isHandheldEnabledB = payload[2] == static_cast<uint8_t>(1);
				break;
			case SettingType::HandheldIntensityB:
				handheldIntensityB = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::HandheldDriftIntensityB:
				handheldDriftIntensityB = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::HandheldJitterIntensityB:
				handheldJitterIntensityB = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::HandheldBreathingIntensityB:
				handheldBreathingIntensityB = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::HandheldBreatingRateB:
				handheldBreathingRateB = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::HandheldDriftSpeedB:
				handheldDriftSpeedB = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::HandheldRotationDriftSpeedB:
				handheldRotationDriftSpeedB = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::HandheldPositionToggleB:
				handheldPositionToggleB = payload[2] == static_cast<uint8_t>(1);
				break;
			case SettingType::HandheldRotationToggleB:
				handheldRotationToggleB = payload[2] == static_cast<uint8_t>(1);
				break;
			case SettingType::PlayerRelativeToggle:
				isRelativePlayerEnabled = payload[2] == static_cast<uint8_t>(1);
				break;
			case SettingType::PlayerRelativeSmoothness:
				relativePathSmoothness = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::WaitBeforePlaying:
				waitBeforePlaying = payload[2] == static_cast<uint8_t>(1);
				break;
			case SettingType::UnpauseOnPlay:
				unpauseOnPlay = payload[2] == static_cast<uint8_t>(1);
				break;
			case SettingType::VisualisationToggle:
				//Utils::callOnChange(togglePathVisualisation, payload[2], [](auto t) {D3DHook::instance().setVisualisation(t);});
				Utils::callOnChange(togglePathVisualisation, payload[2], [](auto t) {D3D12Hook::instance().setVisualisation(t); });
				break;
			case SettingType::VisualiseAllPaths:

				//FLIPPED LOGIC because I set up the flow in the core logic the opposite to the UI :D
				//Utils::callOnChange(toggleDisplayAllPaths, !payload[2], [](auto t) {D3DHook::instance().setRenderPathOnly(t); });
				Utils::callOnChange(toggleDisplayAllPaths, !payload[2], [](auto t) {D3D12Hook::instance().setRenderPathOnly(t); });
				break;
			case SettingType::ScrubbingProgress:
				if (payloadLength >= 6) // need at least 6 bytes (2 header + 4 float)
				{
					float scrubberPosition = Utils::floatFromBytes(payload, payloadLength, 2);

					if (PathManager::instance()._pathManagerStatus == Scrubbing)
						PathManager::instance()._targetScrubbingProgress = scrubberPosition;
				}
				break;
			case SettingType::D3DDisabled:
				d3ddisabled = payload[2] == static_cast<uint8_t>(1);
				break;
			case SettingType::LookAtPlayer:
				lookAtEnabled = payload[2] == static_cast<uint8_t>(1);
				break;
			case SettingType::PathLookAtEnabled:
				pathLookAtEnabled = payload[2] == static_cast<uint8_t>(1);
				//PathManager::instance().updateOffsetforVisualisation();
				break;
			case SettingType::PathLookAtOffsetX:
				pathLookAtOffsetX = Utils::floatFromBytes(payload, payloadLength, 2);
				//PathManager::instance().updateOffsetforVisualisation();
				break;
			case SettingType::PathLookAtOffsetY:
				pathLookAtOffsetY = Utils::floatFromBytes(payload, payloadLength, 2);
				//PathManager::instance().updateOffsetforVisualisation();
				break;
			case SettingType::PathLookAtOffsetZ:
				pathLookAtOffsetZ = Utils::floatFromBytes(payload, payloadLength, 2);
				//PathManager::instance().updateOffsetforVisualisation();
				break;
			case SettingType::PathLookAtSmoothness:
				pathLookAtSmoothness = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::PathSpeedMatchingEnabled:
				pathSpeedMatchingEnabled = payload[2] == static_cast<uint8_t>(1);
				break;
			case SettingType::PathSpeedScale:
				pathSpeedScale = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::PathSpeedSmoothness:
				pathSpeedSmoothness = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::PathMinSpeed:
				pathMinSpeed = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::PathMaxSpeed:
				pathMaxSpeed = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::PathBaselineSpeed:
				pathBaselineSpeed = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			//gamespecific
			case SettingType::DisableFlashlight:
				Utils::callOnChange(disableFlashlight, payload[2],
					[](auto t) {GameSpecific::CameraManipulator::toggleFlashlight(t); });
				break;
			case SettingType::DisableVignette:
				Utils::callOnChange(disableVignette, payload[2],
					[](auto t) {GameSpecific::CameraManipulator::toggleVignette(t); });
				break;
			default:
				// nothing
				break;
			}
		}

		void setValueFromAction(uint8_t payload[], DWORD payloadLength)
		{
			// byte 1 is the id of the setting. Bytes 2 and further contain the data for the setting.
			if (payloadLength < 3)
			{
				return;
			}
			switch (static_cast<ActionMessageType>(payload[1]))
			{
			case ActionMessageType::setWidth:
				hsWidth = Utils::intFromBytes(payload, payloadLength, 2);
				break;
			case ActionMessageType::setHeight:
				hsHeight = Utils::intFromBytes(payload, payloadLength, 2);
				break;
			default:
				break;
			}
		}

		void init(bool persistedOnly)
		{
			invertY = CONTROLLER_Y_INVERT;
			fastMovementMultiplier = FASTER_MULTIPLIER;
			slowMovementMultiplier = SLOWER_MULTIPLIER;
			movementUpMultiplier = DEFAULT_UP_MOVEMENT_MULTIPLIER;
			movementSpeed = DEFAULT_MOVEMENT_SPEED;
			rotationSpeed = DEFAULT_ROTATION_SPEED;
			fovChangeSpeed = DEFAULT_FOV_SPEED;
			cameraControlDevice = DEVICE_ID_ALL;
			gameSpeed = DEFAULT_GAMESPEED;
			movementSmoothness = DEFAULT_MOVEMENT_SMOOTHNESS;
			rotationSmoothness = DEFAULT_ROTATION_SMOOTHNESS;
			lookAtEnabled = DEFAULT_LOOKAT;
			pathLookAtEnabled = false;
			pathLookAtOffsetX = 0.0f;
			pathLookAtOffsetY = 0.0f;
			pathLookAtOffsetZ = 0.0f;
			// Initialize path speed matching settings
			pathSpeedMatchingEnabled = false;
			pathSpeedScale = 1.0f;              // 1:1 scaling by default
			pathSpeedSmoothness = 3.0f;         // Moderate smoothing
			pathMinSpeed = 0.1f;                // Never go below 10% speed
			pathMaxSpeed = 3.0f;                // Cap at 300% speed
			pathBaselineSpeed = 50.0f;          // Baseline speed in game units
		}
	};
}
