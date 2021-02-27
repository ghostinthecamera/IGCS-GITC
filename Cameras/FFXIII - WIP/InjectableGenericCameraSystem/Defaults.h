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

namespace IGCS
{
	// System defaults
	#define FRAME_SLEEP								8		// in milliseconds
	#define IGCS_OVERLAY_INI_FILENAME				"IGCS_overlay.ini"
	#define IGCS_SETTINGS_INI_FILENAME				"IGCS_settings.ini"
	#define IGCS_SETTINGS_SAVE_DELAY				5.0f	// in seconds
	#define IGCS_SPLASH_DURATION					8.0f	// in seconds
	#define IGCS_SUPPORT_RAWKEYBOARDINPUT			true	// if set to false, raw keyboard input is ignored.
	#define IGCS_MAX_MESSAGE_SIZE					4*1024	// in bytes

	// Keyboard system control
	#define IGCS_KEY_TOGGLE_OVERLAY					VK_INSERT		// With control
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
	#define IGCS_KEY_MOVE_UP						VK_NUMPAD7
	#define IGCS_KEY_MOVE_DOWN						VK_NUMPAD9
	#define IGCS_KEY_TILT_LEFT						VK_NUMPAD1		// roll
	#define IGCS_KEY_TILT_RIGHT						VK_NUMPAD3
	#define IGCS_KEY_FOV_RESET						VK_MULTIPLY
	#define IGCS_KEY_FOV_INCREASE					VK_ADD
	#define IGCS_KEY_FOV_DECREASE					VK_SUBTRACT
	#define IGCS_KEY_BLOCK_INPUT					VK_DECIMAL
	#define IGCS_KEY_HUD_TOGGLE						VK_DELETE
	#define IGCS_KEY_TIMESTOP						VK_NUMPAD0

	#define IGCS_BUTTON_FOV_DECREASE	Gamepad::button_t::UP
	#define IGCS_BUTTON_FOV_INCREASE	Gamepad::button_t::DOWN
	#define IGCS_BUTTON_RESET_FOV		Gamepad::button_t::B
	#define IGCS_BUTTON_TILT_LEFT		Gamepad::button_t::LEFT
	#define IGCS_BUTTON_TILT_RIGHT		Gamepad::button_t::RIGHT
	#define IGCS_BUTTON_FASTER			Gamepad::button_t::Y
	#define IGCS_BUTTON_SLOWER			Gamepad::button_t::X

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
		
		// add more here
	};

	
	enum class MessageType : uint8_t
	{
		Setting = 1,
		KeyBinding = 2,
		Notification = 3,
		NormalTextMessage = 4,
		ErrorTextMessage = 5,
		DebugTextMessage= 6,
		Action = 7,
	};

	enum class ActionMessageType : uint8_t
	{
		RehookXInput = 1,
	};
}