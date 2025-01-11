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

namespace IGCS::GameSpecific
{
	// Mandatory constants to define for a game
	#define GAME_NAME									"Lightning Returns: Final Fantasy XIII"
	#define CAMERA_VERSION								"0.1"
	#define CAMERA_CREDITS								"ghostinthecamera"
	#define GAME_WINDOW_TITLE							"LIGHTNING RETURNS: FINAL FANTASY XIII"
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
	#define MATRIX_SIZE									12
	#define COORD_SIZE									3
	#define DEFAULT_IGCS_TYPE							6
	#define BYTE_PAUSE									0x00
	#define BYTE_RESUME									0x01
	// End Mandatory constants
	#define DEFAULT_WIDTH								1280
	#define DEFAULT_HEIGHT								720
	

	// AOB Keys for interceptor's AOB scanner
	#define CAMERA_ADDRESS_INTERCEPT_KEY				"AOB_CAMERA_ADDRESS_INTERCEPT"
	#define CAMERA_WRITE1_INTERCEPT_KEY					"AOB_CAMERA_WRITE1_INTERCEPT"
	#define CAMERA_WRITE2_INTERCEPT_KEY					"AOB_CAMERA_WRITE2_INTERCEPT"

	#define FOV_WRITE_KEY								"FOV_WRITE_KEY"
	#define FOV_WRITE_KEY_CUTSCENE						"FOV_WRITE_KEY_CUTSCENE"
	#define NEAR_PLANE_KEY_CUTSCENE						"NEAR_PLANE_KEY_CUTSCENE"
	#define TIMESTOP_INTERCEPT_KEY						"TIMESTOP_INTERCEPT_KEY"
	#define TIMESTOP_WRITE_KEY							"TIMESTOP_WRITE_KEY"
	#define DOF_KEY										"DOF_KEY"
	#define BLOOM_KEY									"BLOOM_KEY"
	#define HUD_KEY										"HUD_KEY"
	#define RESOLUTION_INTERCEPT_KEY					"RESOLUTION_INTERCEPT_KEY"
	#define AR_INTERCEPT_KEY							"AR_INTERCEPT_KEY"
	#define BATTLE_AR_INTERCEPT_KEY						"BATTLE_AR_INTERCEPT_KEY"
	#define CUTSCENE_AR_INTERCEPT_KEY					"CUTSCENE_AR_INTERCEPT_KEY"
	#define BATTLE_AR_NOP_KEY							"BATTLE_AR_NOP_KEY"	//also for cutscenes
	
	// Indices in the structures read by interceptors 
	#define COORDS_IN_STRUCT_OFFSET						0xA8
	#define MATRIX_IN_STRUCT_OFFSET						0x78
	#define FOV_IN_STRUCT_OFFSET						0x2A0
	#define NEAR_PLANE									0x2A8
	#define TIMESCALE_OFFSET							0x1C		// set float to 1.0 to proceed normally, 0.00001 to pause
	#define WIDTH_OFFSET								0x10
	#define HEIGHT_OFFSET								0x14
	#define AR_OFFSET									0xA54
	#define BATTLE_AR_OFFSET							0x2B0  //also fot cutscenes
	#define JMP_BYTE									0xEB
}
