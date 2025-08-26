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
#include "Gamepad.h"

namespace IGCS
{
	// System defaults
	#define FRAME_SLEEP								8		// in milliseconds
	#define IGCS_SUPPORT_RAWKEYBOARDINPUT			true	// if set to false, raw keyboard input is ignored.
	#define IGCS_MAX_MESSAGE_SIZE					10*1024	// in bytes

	// Keyboard system control
	#define IGCS_KEY_CAMERA_ENABLE					VK_INSERT
	#define IGCS_KEY_CAMERA_LOCK					VK_HOME
	#define IGCS_KEY_ROTATE_RIGHT					VK_RIGHT		// yaw
	#define IGCS_KEY_ROTATE_LEFT					VK_LEFT
	#define IGCS_KEY_ROTATE_UP						VK_UP			// pitch
	#define IGCS_KEY_ROTATE_DOWN					VK_DOWN
	#define IGCS_KEY_MOVE_FORWARD					VK_NUMPAD8
	#define IGCS_KEY_MOVE_BACKWARD					VK_NUMPAD5
	#define IGCS_KEY_MOVE_LEFT						VK_NUMPAD4
	#define IGCS_KEY_MOVE_RIGHT						VK_NUMPAD6
	#define IGCS_KEY_MOVE_UP						VK_NUMPAD9
	#define IGCS_KEY_MOVE_DOWN						VK_NUMPAD7
	#define IGCS_KEY_TILT_LEFT						VK_NUMPAD1		// roll
	#define IGCS_KEY_TILT_RIGHT						VK_NUMPAD3
	#define IGCS_KEY_FOV_RESET						VK_MULTIPLY
	#define IGCS_KEY_FOV_INCREASE					VK_ADD
	#define IGCS_KEY_FOV_DECREASE					VK_SUBTRACT
	#define IGCS_KEY_BLOCK_INPUT					VK_DECIMAL
	#define IGCS_KEY_SLOWMO						    VK_PRIOR
	#define IGCS_KEY_TIMESTOP						VK_NUMPAD0
	#define IGCS_KEY_SKIP_FRAMES					VK_NEXT
	#define IGCS_KEY_HUD							VK_DELETE
	#define IGCS_KEY_PLAYERONLY						VK_END
	#define IGCS_KEY_BYTE_PAUSE						VK_PAUSE
	#define IGCS_KEY_CYCLEDEPTH						VK_TAB
	#define IGCS_KEY_PATH_CREATE					VK_F2
	#define IGCS_KEY_PATH_PLAY_STOP				    VK_F3
    #define IGCS_KEY_PATH_VISUALIZATION_TOGGLE		VK_F12
	#define IGCS_KEY_PATH_ADD_NODE					VK_F4
	#define IGCS_KEY_PATH_DELETE					VK_F10
	#define IGCS_KEY_PATH_CYCLE						VK_OEM_6 // ']' key
	#define IGCS_KEY_PATH_DELETE_LAST_NODE			VK_F11
	#define IGCS_KEY_PATH_TOGGLE_DEPTH				VK_TAB
	#define IGCS_KEY_RESETOFFSETS					VK_F5
	#define IGCS_KEY_TOGGLE_OFFSET_MODE				VK_F6
	#define IGCS_KEY_TOGGLE_HEIGHT_LOCKED			VK_F7
	#define IGCS_KEY_TOGGLE_FIXED_CAMERA			VK_F8
	#define IGCS_KEY_TOGGLE_LOOKAT_VIS 				VK_F9
	#define IGCS_KEY_MOVE_UP_TARGET					0x55 // 'U' key
	#define IGCS_KEY_MOVE_DOWN_TARGET				0x4F // 'O' key

	#define IGCS_BUTTON_FOV_DECREASE				Gamepad::button_t::UP
	#define IGCS_BUTTON_FOV_INCREASE				Gamepad::button_t::DOWN
	#define IGCS_BUTTON_RESET_FOV					Gamepad::button_t::B
	#define IGCS_BUTTON_TILT_LEFT					Gamepad::button_t::LEFT
	#define IGCS_BUTTON_TILT_RIGHT					Gamepad::button_t::RIGHT
	#define IGCS_BUTTON_FASTER						Gamepad::button_t::Y
	#define IGCS_BUTTON_SLOWER						Gamepad::button_t::X
	#define IGCS_BUTTON_PATHCONTROLLER_MODIFIER		Gamepad::button_t::LB
    #define IGCS_BUTTON_PATH_PLAY_STOP				Gamepad::button_t::X
	#define IGCS_BUTTON_PATH_VISUALIZATION_TOGGLE 	Gamepad::button_t::LSTICK
	#define IGCS_BUTTON_PATH_ADD_NODE 				Gamepad::button_t::A
	#define IGCS_BUTTON_PATH_CREATE 				Gamepad::button_t::Y
	#define IGCS_BUTTON_PATH_DELETE 				Gamepad::button_t::B
	#define IGCS_BUTTON_PATH_CYCLE 					Gamepad::button_t::RIGHT
	#define IGCS_BUTTON_PATH_DELETE_LAST_NODE 		Gamepad::button_t::LEFT
	#define IGCS_BUTTON_RESETOFFSETS 				Gamepad::button_t::B
	#define IGCS_BUTTON_TOGGLE_OFFSET_MODE			Gamepad::button_t::Y
	#define IGCS_BUTTON_TOGGLE_HEIGHT_LOCKED		Gamepad::button_t::X
	#define IGCS_BUTTON_TOGGLE_FIXED_CAMERA			Gamepad::button_t::A
	#define IGCS_BUTTON_TOGGLE_LOOKAT_VIS 			Gamepad::button_t::RSTICK

	static const uint8_t jmpFarInstructionBytes[6] = { 0xff, 0x25, 0, 0, 0, 0 };	// instruction bytes for jmp qword ptr [0000]

	#define DEVICE_ID_KEYBOARD_MOUSE			0
	#define DEVICE_ID_GAMEPAD					1
	#define DEVICE_ID_ALL						2

	#define IGCS_PIPENAME_DLL_TO_CLIENT				"\\\\.\\pipe\\IgcsDllToClient"
	#define IGCS_PIPENAME_CLIENT_TO_DLL				"\\\\.\\pipe\\IgcsClientToDll"
	

	enum class SettingType : uint8_t
	{
		FastMovementMultiplier = 0,
		SlowMovementMultiplier = 1,
		UpMovementMultiplier = 2,
		MovementSpeed = 3,
		CameraControlDevice = 4,
		RotationSpeed = 5,
		InvertYLookDirection = 6,
		FoVZoomSpeed = 7,
		gameSpeed = 8,
		MovementSmoothness = 9,
		RotationSmoothness = 10,
		FOVSmoothness = 11,
		PathDuration = 12,
		PathEasingValue = 13,
		PathSampleCount = 14,
		PathToggleInterpolationMode = 15,
		PathEasingType = 16,
		EulerOrder = 17,
		UniformParam = 18,
		RotationMode = 19,
		DeltaType = 20,
		DeltaValue = 21,
		CameraShakeToggle = 22,
		Amplitude = 23,
		Frequency = 24,
		HandheldCameraToggle = 25,
		HandheldIntensity = 26,
		HandheldDriftIntensity = 27,
		HandheldJitterIntensity = 28,
		HandheldBreathingIntensity = 29,
		HandheldBreatingRate = 30,
		HandheldRotationToggle = 31,
		HandheldPositionToggle = 32,
		PlayerRelativeToggle = 33,
		PlayerRelativeSmoothness = 34,
		WaitBeforePlaying = 35,
		UnpauseOnPlay = 36,
		HandheldDriftSpeed = 37,
		HandheldRotationDriftSpeed = 38,
		VisualisationToggle = 39,
		VisualiseAllPaths = 40,
		ScrubbingProgress = 41,
		D3DDisabled = 42,
		LookAtPlayer = 43,
		PathLookAtEnabled = 44,
		PathLookAtOffsetX = 45,
		PathLookAtOffsetY = 46,
		PathLookAtOffsetZ = 47,
		PathLookAtSmoothness = 48,
		PathSpeedMatchingEnabled = 49,
		PathSpeedScale = 50,
		PathSpeedSmoothness = 51,
		PathMinSpeed = 52,
		PathMaxSpeed = 53,
		PathBaselineSpeed = 54,
		CameraShakeToggleB = 55,
		AmplitudeB = 56,
		FrequencyB = 57,
		HandheldCameraToggleB = 58,
		HandheldIntensityB = 59,
		HandheldDriftIntensityB = 60,
		HandheldJitterIntensityB = 61,
		HandheldBreathingIntensityB = 62,
		HandheldBreatingRateB = 63,
		HandheldRotationToggleB = 64,
		HandheldPositionToggleB = 65,
		HandheldDriftSpeedB = 66,
		HandheldRotationDriftSpeedB = 67,
		AltPlayerTracking = 68,
		IncreaseShadowRes = 69
	};


	
	enum class MessageType : uint8_t
	{
		Setting = 1,
		KeyBinding = 2,
		Notification = 3,
		NormalTextMessage = 4,
		ErrorTextMessage = 5,
		DebugTextMessage = 6,
		Action = 7,
		CameraPathData = 8,
		UpdatePathPlaying = 9,
		CameraEnabled = 10,
		CameraPathBinaryData = 11,
		UpdateVisualisation = 12,
		UpdateSelectedPath = 13,
		CyclePath = 14,
		PathProgress = 15,
		NotificationOnly = 16,
		MotionBlurUpdate = 17,

	};

	enum class ActionMessageType : uint8_t
	{
		RehookXInput = 1,
		setWidth = 2,
		setHeight = 3,
		setResolution = 4,
	};

	enum class PathActionType : uint8_t
	{
		addPath = 1,
		deletePath = 2,
		addNode = 3,
		playPath = 4,
		stopPath = 5,
		gotoNode = 6,
		updateNode = 7,
		pausePathPlayBack = 8,
		deleteNode = 9,
		refreshPath = 10,
		selectedPathUpdate = 11,
		insertNode = 12,
		selectedNodeIndexUpdate = 13,
		pathManagerStatus = 14,
		pathScrubbing = 15,
		pathScrubPosition = 16,
	};

	enum class EulerOrder {
		XYZ,  // corresponds to: i = 2, j = 1, k = 0
		XZY,  // i = 1, j = 2, k = 0
		YXZ,  // i = 2, j = 0, k = 1
		YZX,  // i = 0, j = 2, k = 1
		ZXY,  // i = 1, j = 0, k = 2
		ZYX   // i = 0, j = 1, k = 2
	};

	enum class D3DMODE : uint8_t
	{
		DX11 = 0,
		DX12 = 1,
	};
}