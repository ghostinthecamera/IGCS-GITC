//////////////////////////	//////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System
// Copyright(c) 2019, Frans Bouma
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
	#define GAME_NAME									"Ni No Kuni 2"
	#define CAMERA_VERSION								"0.1"
	#define CAMERA_CREDITS								"GHOSTINTHECAMERA & Hattiwatti"
	#define GAME_WINDOW_TITLE							"Ni no Kuni II: Revenant Kingdom"
	#define INITIAL_PITCH_RADIANS						0.0f	// around X axis	(right)
	#define INITIAL_YAW_RADIANS							0.0f	// around Y axis	(up)
	#define INITIAL_ROLL_RADIANS						0.0f	// around Z axis	(out of the screen)
	#define CONTROLLER_Y_INVERT							false
	#define FASTER_MULTIPLIER							5.0f
	#define SLOWER_MULTIPLIER							0.1f
	#define MOUSE_SPEED_CORRECTION						0.2f	// to correct for the mouse-deltas related to normal rotation.
	#define DEFAULT_MOVEMENT_SPEED						0.1f
	#define DEFAULT_ROTATION_SPEED						0.01f
	#define DEFAULT_FOV_SPEED							0.10f
	#define DEFAULT_UP_MOVEMENT_MULTIPLIER				0.7f
	#define RESOLUTION_SCALE_MAX						4.0f
	#define RESOLUTION_SCALE_MIN						0.5f
	// End Mandatory constants

	// AOB Keys for interceptor's AOB scanner
	#define CAMERA_ADDRESS_INTERCEPT_KEY				"AOB_CAMERA_ADDRESS_INTERCEPT"
	#define HUD_RENDER_INTERCEPT_KEY					"AOB_HUD_RENDER_INTERCEPT_KEY"
	#define FOV_INTERCEPT_KEY							"AOB_FOV_INTERCEPT_KEY"
	#define TIMESCALE_INTERCEPT_KEY						"AOB_TIMESCALE_INTERCEPT_KEY"
	#define ABS1_TIMESCALE_INTERCEPT_KEY				"AOB_ABS1_TIMESCALE_INTERCEPT_KEY"
	#define ABS2_TIMESCALE_INTERCEPT_KEY				"AOB_ABS2_TIMESCALE_INTERCEPT_KEY"
	#define QUATERNION_WRITE							"AOB_QUATERNION_WRITE"
	#define QUATERNION_COORD_WRITE						"AOB_QUATERNION_COORD_WRITE"
	#define QUATERNION_CUTSCENE_COORD_WRITE				"AOB_QUATERNION_CUTSCENE_COORD_WRITE"
	//#define TIMESTOP_KEY								"AOB_TIMESTOP_KEY"
	

	// Indices in the structures read by interceptors 
	#define MATRIX_IN_STRUCT_OFFSET					    0x000
	#define HFOV_IN_STRUCT_OFFSET						0x160
	#define VFOV_IN_STRUCT_OFFSET						0x174
	#define TIMESTOP_OFFSET								0x018
	#define HUD_OFFSET									0x6144
	#define QUATERNION_OFFSET						   -0x1C0
	#define QUATERNION_COORD_OFFSET					   -0x1B0

}
