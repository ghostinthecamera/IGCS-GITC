//////////////////////////	//////////////////////////////////////////////////////////////////////////////
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

namespace IGCS
{
	// Mandatory constants to define for a game
	#define GAME_NAME									"DS3"
	#define CAMERA_VERSION								"1.2.1"
	#define CAMERA_CREDITS								"ghostinthecamera"
	#define GAME_WINDOW_TITLE							"DARK SOULS III"
	#define GAME_EXE_NAME								"DarkSoulsIII.exe"
	#define INITIAL_PITCH_RADIANS						0.0f	// around X axis	(right)
	#define INITIAL_YAW_RADIANS							0.0f	// around Y axis	(up)
	#define INITIAL_ROLL_RADIANS						0.0f	// around Z axis	(out of the screen)
	#define CONTROLLER_Y_INVERT							false
	// These will be overwritten by settings sent by the client. These defines are for initial usage. 
	#define FASTER_MULTIPLIER							5.0f
	#define SLOWER_MULTIPLIER							0.07f
	#define MOUSE_SPEED_CORRECTION						0.2f	// to correct for the mouse-deltas related to normal rotation.
	#define DEFAULT_MOVEMENT_SPEED						0.05f
	#define DEFAULT_ROTATION_SPEED						0.01f
	#define DEFAULT_FOV_SPEED							0.03f
	#define DEFAULT_UP_MOVEMENT_MULTIPLIER				0.7f
	#define DEFAULT_FOV									0.79f	// fov in degrees
	#define DEFAULT_GAMESPEED							0.5f
	#define DEFAULT_MOVEMENT_SMOOTHNESS					5.0f
	#define DEFAULT_ROTATION_SMOOTHNESS					5.0f
	//System default defines
	#define MATRIX_SIZE									12
	#define COORD_SIZE									3
	#define DEFAULT_IGCS_TYPE							6
	#define BYTE_PAUSE									0x00
	#define BYTE_RESUME									0x01
	#define MULTIPLICATION_ORDER						Utils::EulerOrder::YXZ
	#define NEGATE_PITCH								true
	#define NEGATE_YAW									false
	#define NEGATE_ROLL									true
	#define RAD_TO_DEG_FACTOR							(180.0f / XM_PI)
	#define DEG_TO_RAD_FACTOR							(XM_PI / 180.0f)

	// Other Compile?time constants
	inline constexpr auto PATH_MANAGER_MAXPATHS = 5;
	inline constexpr auto PATH_MAX_NODES = 15;
	inline constexpr auto RUN_IN_HOOKED_PRESENT = true;
	// Modify these constants (?1.f or 1.f) to match your engine’s forward/right/up directions. 1.f == current behaviour.
	static constexpr float kForwardSign = 1.0f; // set to ?1.f if engine uses negative forward
	static constexpr float kRightSign = 1.0f; // set to ?1.f if engine uses negative right
	static constexpr float kUpSign = 1.0f; // set to ?1.f if engine uses negative up
	// D3DLogging Constant
	static constexpr auto D3DLOGGING = true;
	static constexpr auto VERBOSE = false;
	static constexpr auto USE_WINDOWFOREGROUND_OVERRIDE = false;
	// End Mandatory constants
	

	// AOB Keys for interceptor's AOB scanner
	inline constexpr auto CAMERA_ADDRESS_INTERCEPT_KEY = "AOB_CAMERA_ADDRESS_INTERCEPT";
	inline constexpr auto CAM_BLOCK1 = "CAM_BLOCK1";
	inline constexpr auto CAM_BLOCK2 = "CAM_BLOCK2";
	inline constexpr auto HUDTOGGLE = "HUDTOGGLE";
	inline constexpr auto TIMESTOP_KEY = "TIMESTOP_KEY";
	inline constexpr auto FOV_INTERCEPT_KEY = "FOV_INTERCEPT_KEY"; //2nops;
	inline constexpr auto FOV_INTERCEPT_DEBUG = "FOV_INTERCEPT_DEBUG";
	inline constexpr auto DOF_KEY = "DOF_KEY";
	inline constexpr auto PLAYERPOINTER_KEY = "PLAYERPOINTER_KEY";
	inline constexpr auto ENTITY_KEY = "ENTITY_KEY	";
	inline constexpr auto PAUSE_BYTE = "PAUSE_BYTE";
	inline constexpr auto TIMESTOP_KEY_NOP = "TIMESTOP_KEY_NOP";
	inline constexpr auto VIGNETTE_KEY = "VIGNETTE_KEY";
	inline constexpr auto BLOOM_KEY = "BLOOM_KEY";
	inline constexpr auto OPACITY_KEY = "OPACITY_KEY";
	inline constexpr auto TIMESTOP_CORRECT_KEY = "TIMESTOP_CORRECT_KEY";


	// Indices in the structures read by interceptors 
	#define COORDS_IN_STRUCT_OFFSET	0x40
	#define MATRIX_IN_STRUCT_OFFSET	0x10
	#define FOV_IN_STRUCT_OFFSET	0x50
	#define DOF_OFFSET				0x28
	#define TIMESCALE_OFFSET		0x264
	#define PLAYER_POSITION_OFFSET	0x14C0
	
}
