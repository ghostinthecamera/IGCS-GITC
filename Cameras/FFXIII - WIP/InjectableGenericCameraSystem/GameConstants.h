//////////////////////////	//////////////////////////////////////////////////////////////////////////////
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

namespace IGCS::GameSpecific
{
	// Mandatory constants to define for a game
	#define GAME_NAME									"Final Fantasy XIII"
	#define CAMERA_VERSION								"0.1"
	#define CAMERA_CREDITS								"SKall feat. GHOSTINTHECAMERA"
	#define GAME_WINDOW_TITLE							"FINAL FANTASY XIII"
	#define INITIAL_PITCH_RADIANS						0.0f	// around X axis	(right)
	#define INITIAL_YAW_RADIANS							0.0f	// around Y axis	(out of the screen)
	#define INITIAL_ROLL_RADIANS						0.0f	// around Z axis	(up)
	#define CONTROLLER_Y_INVERT							false
	// These will be overwritten by settings sent by the client. These defines are for initial usage. 
	#define FASTER_MULTIPLIER							5.0f
	#define SLOWER_MULTIPLIER							0.1f
	#define MOUSE_SPEED_CORRECTION						0.2f	// to correct for the mouse-deltas related to normal rotation.
	#define DEFAULT_MOVEMENT_SPEED						1.0f
	#define DEFAULT_ROTATION_SPEED						0.001f
	#define DEFAULT_FOV_SPEED							0.01f
	#define DEFAULT_UP_MOVEMENT_MULTIPLIER				0.07f
	// End Mandatory constants

	// AOB Keys for interceptor's AOB scanner
	#define CAMERA_ADDRESS_INTERCEPT_KEY				"AOB_CAMERA_ADDRESS_INTERCEPT"
	#define HUDTOGGLE									"HUDTOGGLE"
	#define TIMESTOP_KEY								"TIMESTOP_KEY"
	#define HFOV_INTERCEPT_KEY							"HFOV_INTERCEPT_KEY" //2nops
	#define VFOV_INTERCEPT_KEY							"VFOV_INTERCEPT_KEY" //3nops
	#define DOF_KEY										"DOF_KEY"
	#define AR_KEY										"AR_KEY"


	// Indices in the structures read by interceptors 
	#define COORDS_IN_STRUCT_OFFSET						0x30
	#define HFOV_IN_STRUCT_OFFSET						0x40
	#define VFOV_IN_STRUCT_OFFSET						0x54
	#define DOF_NEAR_AMOUNT								0x4C
	#define DOF_FAR_AMOUNT								0x50
	#define AR_OFFSET									0x3FC
	#define FOV_NOP1_OFFSET								0x0C  //2 nops
	#define FOV_NOP2_OFFSET								0x39  //3 nops
}
