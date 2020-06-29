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
	#define GAME_NAME									"GRID (2019) v1.0+"
	#define CAMERA_VERSION								"0.1"
	#define CAMERA_CREDITS								"ghostinthecamera"
	#define GAME_WINDOW_TITLE							"GRID (2019) (DirectX 12)"
	#define INITIAL_PITCH_RADIANS						0.0f	// around X axis	(right)
	#define INITIAL_YAW_RADIANS							0.0f	// around Y axis	(out of the screen)
	#define INITIAL_ROLL_RADIANS						0.0f	// around Z axis	(up)
	#define CONTROLLER_Y_INVERT							false
	// These will be overwritten by settings sent by the client. These defines are for initial usage. 
	#define FASTER_MULTIPLIER							5.0f
	#define SLOWER_MULTIPLIER							0.07f
	#define MOUSE_SPEED_CORRECTION						0.2f	// to correct for the mouse-deltas related to normal rotation.
	#define DEFAULT_MOVEMENT_SPEED						0.05f
	#define DEFAULT_ROTATION_SPEED						0.01f
	#define DEFAULT_FOV_SPEED							0.05f
	#define DEFAULT_UP_MOVEMENT_MULTIPLIER				0.7f
	#define DEFAULT_FOV									1.31f	// fov in degrees
	// End Mandatory constants

	// AOB Keys for interceptor's AOB scanner
	#define CAMERA_ADDRESS_INTERCEPT_KEY				"AOB_CAMERA_ADDRESS_INTERCEPT"
	#define CAMERA_WRITE1_RET_KEY						"AOB_CAMERA_WRITE_RET"
	#define CAMERA_COORD_WRITE_NOP_KEY					"AOB_CAMERA_COORD_WRITE_NOP"
	#define CAMERA_QUAT_WRITE_NOP_KEY					"AOB_CAMERA_QUAT_WRITE_NOP"
	#define CAMERA_MOVE_WRITE_NOP_KEY					"AOB_CAMERA_MOVE_WRITE_NOP"
	#define CAMERA_MOVE_WRITE_NOP_KEY2					"AOB_CAMERA_MOVE_WRITE_NOP2"
	#define CAMERA_MOVE_WRITE_NOP_KEY3					"AOB_CAMERA_MOVE_WRITE_NOP3"
	#define CAMERA_MOVE_WRITE_NOP_KEY4					"AOB_CAMERA_MOVE_WRITE_NOP4"
	#define TIMESTOP_READ_INTERCEPT_KEY				    "AOB_TIMESTOP_INTERCEPT"
	#define ABS1_TIMESCALE_INTERCEPT_KEY				"AOB_TIMESCALE_ABSOLUTE_KEY"
	#define FOV_WRITE_NOP_KEY							"AOB_FOV_WRITE_NOP"
	
	// Indices in the structures read by interceptors 
	#define COORDS_IN_STRUCT_OFFSET						0x10
	#define QUATERNION_IN_STRUCT_OFFSET					0x00
	#define FOV_IN_STRUCT_OFFSET						0x70
	#define TIMESTOP_FLOAT_OFFSET						0x54		// set float to 1.0 to proceed normally, 0.000001 to pause
	
}
