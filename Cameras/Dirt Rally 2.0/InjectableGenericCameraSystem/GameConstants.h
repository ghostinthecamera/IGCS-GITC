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
	#define GAME_NAME									"Dirt Rally 2.0"
	#define CAMERA_VERSION								"2.0.1"
	#define CAMERA_CREDITS								"ghostinthecamera"
	#define GAME_WINDOW_TITLE							"DiRT Rally 2.0"
	#define GAME_EXE_NAME								"dirtrally2.exe"
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
	#define DEFAULT_LOOKAT								true
	//System default defines
	#define MATRIX_SIZE									12
	#define COORD_SIZE									3
	#define DEFAULT_IGCS_TYPE							6
	#define BYTE_PAUSE									0x00
	#define BYTE_RESUME									0x01
	#define MULTIPLICATION_ORDER						Utils::EulerOrder::YXZ
	#define NEGATE_PITCH								true
	#define NEGATE_YAW									true
	#define NEGATE_ROLL									false
	#define RAD_TO_DEG_FACTOR							(180.0f / XM_PI)
	#define DEG_TO_RAD_FACTOR							(XM_PI / 180.0f)


	// Other Compile?time constants
	inline constexpr auto PATH_MANAGER_MAXPATHS = 5;
	inline constexpr auto PATH_MAX_NODES = 8;
	inline constexpr auto RUN_IN_HOOKED_PRESENT = true;
	// Modify these constants (?1.f or 1.f) to match your engine’s forward/right/up directions. 1.f == current behaviour.
	static constexpr float kForwardSign = 1.0f; // set to ?1.f if engine uses negative forward
	static constexpr float kRightSign = -1.0f; // set to ?1.f if engine uses negative right
	static constexpr float kUpSign = 1.0f; // set to ?1.f if engine uses negative up
	// D3DLogging Constant
	static constexpr auto D3DLOGGING = true;
	static constexpr auto VERBOSE = false;
	static constexpr auto USE_WINDOWFOREGROUND_OVERRIDE = false;
	

	// AOB Keys for interceptor's AOB scanner
	inline constexpr auto ACTIVE_CAMERA_ADDRESS_INTERCEPT = "ACTIVE_CAMERA_ADDRESS_INTERCEPT";
	inline constexpr auto CAM_WRITE1 = "CAM_WRITE1";
	inline constexpr auto CAM_WRITE2 = "CAM_WRITE2";
	inline constexpr auto CAM_WRITE3 = "CAM_WRITE3";
	inline constexpr auto CAM_WRITE4 = "CAM_WRITE4";
	inline constexpr auto CAM_WRITE5 = "CAM_WRITE5";
	inline constexpr auto REPLAY_NOP1 = "REPLAY_NOP1";
	inline constexpr auto REPLAY_NOP2 = "REPLAY_NOP2";
	inline constexpr auto REPLAY_NOP3 = "REPLAY_NOP3";
	inline constexpr auto REPLAY_NOP4 = "REPLAY_NOP4";
	inline constexpr auto REPLAY_NOP5 = "REPLAY_NOP5";
	inline constexpr auto REPLAY_NOP6 = "REPLAY_NOP6";
	inline constexpr auto REPLAY_NOP7 = "REPLAY_NOP7";
	inline constexpr auto REPLAY_NOP8 = "REPLAY_NOP8";
	inline constexpr auto REPLAY_NOP9 = "REPLAY_NOP9";
	inline constexpr auto REPLAY_NOP10 = "REPLAY_NOP10";
	inline constexpr auto REPLAY_NOP11 = "REPLAY_NOP11";
	inline constexpr auto REPLAY_NOP12 = "REPLAY_NOP12";
	inline constexpr auto REPLAY_NOP13 = "REPLAY_NOP13";
	inline constexpr auto GAMEPLAY_NOP1 = "GAMEPLAY_NOP1";
	inline constexpr auto GAMEPLAY_NOP2 = "GAMEPLAY_NOP2";
	inline constexpr auto GAMEPLAY_NOP3 = "GAMEPLAY_NOP3";
	inline constexpr auto GAMEPLAY_NOP4 = "GAMEPLAY_NOP4";
	inline constexpr auto GAMEPLAY_NOP5 = "GAMEPLAY_NOP5";
	inline constexpr auto FOV_ABS = "FOV_ABS";
	inline constexpr auto FOV_WRITE1 = "FOV_WRITE1";
	inline constexpr auto FOV_WRITE2 = "FOV_WRITE2";
	inline constexpr auto FOV_WRITE3 = "FOV_WRITE3";
	inline constexpr auto FOV_WRITE4 = "FOV_WRITE4";
	inline constexpr auto FOV_WRITE_NOP = "FOV_WRITE_NOP";
	inline constexpr auto FOV_WRITE_NOP2 = "FOV_WRITE_NOP2";
	inline constexpr auto COLLISION_NOP1 = "COLLISION_NOP1";
	inline constexpr auto COLLISION_NOP2 = "COLLISION_NOP2";
	inline constexpr auto CAR_POSITION_INJECTION = "CAR_POSITION_INJECTION";
	inline constexpr auto FOCUS_LOSS_NOP = "FOCUS_LOSS_NOP";
	inline constexpr auto HUD_TOGGLE_INJECTION = "HUD_TOGGLE_INJECTION";
	inline constexpr auto TIMESCALE_RELATIVE_OFFSET = "TIMESCALE_RELATIVE_OFFSET";
	inline constexpr auto TIMESCALE_INJECTION = "TIMESCALE_INJECTION";
	inline constexpr auto TIMESCALE_NOP = "TIMESCALE_NOP";
	inline constexpr auto DOF_INJECTION = "DOF_INJECTION";




	// Indices in the structures read by interceptors 
	#define QUATERNION_IN_STRUCT_OFFSET									0x130
	#define COORDS_IN_STRUCT_OFFSET										0x140
	#define FOV_IN_STRUCT_OFFSET										0x1A0
	#define PLAYER_POSITION_IN_STRUCT_OFFSET							0x2D0
	#define PLAYER_ROTATION_IN_STRUCT_OFFSET							0x2E0
	#define TIMESCALE_OFFSET											0x290
	#define DOF_STRENGTH_OFFSET											0x7AC
	#define NEAR_Z_FROM_QUATERNION_OFFSET								0x78
	#define MOTION_BLUR_STRENGTH_FROM_CAMQUATERNION_OFFSET				0x5C


}
