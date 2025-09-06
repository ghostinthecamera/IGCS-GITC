//////////////////////////	//////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System
// Copyright(c) 2020, Frans Bouma
// All rights reserved.
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
#include "Defaults.h"

namespace IGCS
{
	// Mandatory constants to define for a game
	inline constexpr auto GAME_NAME = "Resident Evil 7 Tools";
	inline constexpr auto CAMERA_VERSION = "1.0.0";
	inline constexpr auto CAMERA_CREDITS = "ghostinthecamera";
	inline constexpr auto GAME_WINDOW_TITLE = L"RESIDENT EVIL 7 biohazard";
	inline constexpr auto GAME_EXE_NAME = "re7.exe";
	inline constexpr auto INITIAL_PITCH_RADIANS = 0.0f;	// around X axis	(right)
	inline constexpr auto INITIAL_YAW_RADIANS = 0.0f;	// around Y axis	(up)
	inline constexpr auto INITIAL_ROLL_RADIANS = 0.0f;	// around Z axis	(out of the screen)
	inline constexpr auto CONTROLLER_Y_INVERT = false;
	// These will be overwritten by settings sent by the client. These defines are for initial usage. 
	inline constexpr auto FASTER_MULTIPLIER = 5.0f;
	inline constexpr auto SLOWER_MULTIPLIER = 0.07f;
	inline constexpr auto MOUSE_SPEED_CORRECTION = 0.2f;	// to correct for the mouse-deltas related to normal rotation.
	inline constexpr auto DEFAULT_MOVEMENT_SPEED = 0.05f;
	inline constexpr auto DEFAULT_ROTATION_SPEED = 0.01f;
	inline constexpr auto DEFAULT_FOV_SPEED = 0.03f;
	inline constexpr auto DEFAULT_UP_MOVEMENT_MULTIPLIER = 0.7f;
	inline constexpr auto DEFAULT_FOV = 0.79f;	// fov in degrees
	inline constexpr auto DEFAULT_GAMESPEED = 0.5f;
	inline constexpr auto DEFAULT_MOVEMENT_SMOOTHNESS = 5.0f;
	inline constexpr auto DEFAULT_ROTATION_SMOOTHNESS = 5.0f;
	inline constexpr auto DEFAULT_LOOKAT = false;

	//System default defines
	inline constexpr auto DEFAULT_IGCS_TYPE = 6;

	// Game specific tool settings
	inline constexpr auto MULTIPLICATION_ORDER = EulerOrder::YXZ;
	inline constexpr auto D3DMODE = D3DMODE::DX12; // Set to DX11 or DX12 based on the game
	inline constexpr auto RUN_IN_HOOKED_PRESENT = true;
	inline constexpr auto USE_WINDOWFOREGROUND_OVERRIDE = false;
	inline constexpr auto XINPUT_VERSION = L"XINPUT1_3"; // XInput DLL to hook, usually XINPUT1_3 or XINPUT9_1_0

	// Other Compile time constants
	inline constexpr auto PATH_MANAGER_MAXPATHS = 5;
	inline constexpr auto PATH_MAX_NODES = 8;
	inline constexpr float ASPECT_RATIO = 1.7777777777777777777777777777778f; // Used to calculate vertical FOV from horizontal FOV

	// Modify these constants (?1.f or 1.f) to match your engine’s forward/right/up directions. 1.f == current behaviour.
	inline constexpr float kForwardSign = -1.0f; // set to ?1.f if engine uses negative forward
	inline constexpr float kRightSign = 1.0f; // set to ?1.f if engine uses negative right
	inline constexpr float kUpSign = 1.0f; // set to ?1.f if engine uses negative up
	inline constexpr auto NEGATE_PITCH = false;
	inline constexpr auto NEGATE_YAW = true;
	inline constexpr auto NEGATE_ROLL = true;

	// D3DLogging Constant
	inline constexpr auto D3DLOGGING = true;
	inline constexpr auto VERBOSE = false;

	// AOB Keys for interceptor's AOB scanner
	inline constexpr auto ACTIVE_CAMERA_ADDRESS_INTERCEPT = "ACTIVE_CAMERA_ADDRESS_INTERCEPT";
	inline constexpr auto FOV_WRITE = "FOV_WRITE";
	inline constexpr auto CAMERA_POSITION_WRITE = "CAMERA_POSITION_WRITE";
	inline constexpr auto CAMERA_ROTATION_WRITE = "CAMERA_ROTATION_WRITE";
	inline constexpr auto DISABLE_FLASHLIGHT = "DISABLE_FLASHLIGHT";
	inline constexpr auto HUD_TOGGLE = "HUD_TOGGLE";
	inline constexpr auto VIGNETTE_TOGGLE = "VIGNETTE_TOGGLE";
	inline constexpr auto TIMESCALE_ADDRESS = "TIMESCALE_ADDRESS";
	inline constexpr auto PLAYER_ADDRESS = "PLAYER_ADDRESS";
	inline constexpr auto PLAYER_POSITION_ZERO = "PLAYER_POSITION_ZERO";
	inline constexpr auto DISABLE_PLAYER_LIGHT_CHECK = "DISABLE_PLAYER_LIGHT_CHECK";

	// Indices in the structures read by interceptors 
	inline constexpr auto COORDS_IN_STRUCT_OFFSET = 0x30;
	inline constexpr auto QUATERNION_IN_STRUCT_OFFSET = 0x40;
	inline constexpr auto MATRIX_IN_STRUCT_OFFSET = 0x80;
	inline constexpr auto SECOND_COORDS_IN_STRUCT_OFFSET = 0xB0;
	inline constexpr auto FOV_IN_STRUCT_OFFSET = 0x158;
	inline constexpr auto NEARZ_IN_STRUCT_OFFSET = 0x150;
	inline constexpr auto FARZ_IN_STRUCT_OFFSET = 0x154;
	inline constexpr auto VIGNETTE_OFFSET = 0x20C;
	inline constexpr auto TIMESCALE_OFFSET = 0x348;
	inline constexpr auto PLAYER_POSITION_OFFSET = 0x30;
	inline constexpr auto PLAYER_ROTATION_OFFSET = 0x40; //quaternion


	// Patches for the game
	inline constexpr uint8_t hudPatch[] = { 0xEB, 0x5B }; // changes 74 to EB
	inline constexpr uint8_t playerPositionPatch[] = {0xEB};
}
