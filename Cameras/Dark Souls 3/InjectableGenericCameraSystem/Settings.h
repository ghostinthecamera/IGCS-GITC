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
#include "Utils.h"
#include "GameConstants.h"
#include "Defaults.h"
#include "D3DHook.h"
#include "PathManager.h"
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
		//game specific
		bool bloomToggle;
		bool vignetteToggle;
		bool playerhideToggle;
		bool npchideToggle;
		bool timestopType;

		


		
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
			//game specific
			case SettingType::BloomToggle:
				bloomToggle = payload[2] == static_cast<uint8_t>(1);
				Utils::toggleNOPState(System::instance().getAOBBlock().at(BLOOM_KEY),5, bloomToggle);
				break;
			case SettingType::VignetteToggle:
				vignetteToggle = payload[2] == static_cast<uint8_t>(1);
				Utils::toggleNOPState(System::instance().getAOBBlock().at(VIGNETTE_KEY),5, vignetteToggle);
				break;
			case SettingType::HidePlayer:
				playerhideToggle = payload[2] == static_cast<uint8_t>(1);
				break;
			case SettingType::HideNPC:
				npchideToggle = payload[2] == static_cast<uint8_t>(1);
				break;
			case SettingType::TimestopType:
				timestopType = payload[2] == static_cast<uint8_t>(1);
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
			case SettingType::PathToggleEasing:
				easingEnabled = payload[2] == static_cast<uint8_t>(1);
				break;
			case SettingType::PathToggleInterpolationMode:
				positionInterpMode = Utils::uintFromBytes(payload, payloadLength, 2);

				if (d3ddisabled)
					return;

				if (!CameraPathManager::instance()._pathManagerEnabled)
					return;

				D3DHook::instance().safeInterpolationModeChange();
				break;
			case SettingType::PathEasingType:
				easingtype = Utils::uintFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::PathAlphaValue:
				alphaValue = Utils::floatFromBytes(payload, payloadLength, 2);
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

				if (!CameraPathManager::instance()._pathManagerEnabled)
					return;
				
				D3DHook::instance().safeInterpolationModeChange();
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
			case SettingType::HanheldRotationDriftSpeed:
				handheldRotationDriftSpeed = Utils::floatFromBytes(payload, payloadLength, 2);
				break;
			case SettingType::HandheldPositionToggle:
				handheldPositionToggle = payload[2] == static_cast<uint8_t>(1);
				break;
			case SettingType::HandheldRotationToggle:
				handheldRotationToggle = payload[2] == static_cast<uint8_t>(1);
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

				Utils::onChange(togglePathVisualisation, payload[2], [](auto t) {D3DHook::instance().setVisualization(t);});
				break;
			case SettingType::VisualiseAllPaths:

				//FLIPPED LOGIC because I set up the flow in the core logic the opposite to the UI :D
				Utils::onChange(toggleDisplayAllPaths, !payload[2], [](auto t) {D3DHook::instance().setRenderPathOnly(t); });
				break;
			case SettingType::ScrubbingProgress:
				if (payloadLength >= 6) // need at least 6 bytes (2 header + 4 float)
				{
					float scrubberPosition = Utils::floatFromBytes(payload, payloadLength, 2);

					if (CameraPathManager::instance()._pathManagerState == Scrubbing)
						CameraPathManager::instance()._targetScrubbingProgress = scrubberPosition;
				}
				break;
			case SettingType::D3DDisabled:
				d3ddisabled = payload[2];
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
		}
	};
}
