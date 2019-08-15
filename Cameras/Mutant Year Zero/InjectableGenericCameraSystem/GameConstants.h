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
	#define GAME_NAME									"Mutant Year Zero"
	#define CAMERA_VERSION								"1.1"
	#define CAMERA_CREDITS								"GHOSTINTHECAMERA & One3rd"
	#define GAME_WINDOW_TITLE							"Mutant Year Zero: Road to Eden "
	#define INITIAL_PITCH_RADIANS						0.0f	// around X axis	(right)
	#define INITIAL_YAW_RADIANS							0.0f	// around Y axis	(out of the screen)
	#define INITIAL_ROLL_RADIANS						0.0f	// around Z axis	(up)
	#define CONTROLLER_Y_INVERT							false
	#define FASTER_MULTIPLIER							5.0f
	#define SLOWER_MULTIPLIER							0.1f
	#define MOUSE_SPEED_CORRECTION						0.2f	// to correct for the mouse-deltas related to normal rotation.
	#define DEFAULT_MOVEMENT_SPEED						6.0f
	#define DEFAULT_ROTATION_SPEED						0.04f
	#define DEFAULT_FOV_SPEED							1.00f
	#define DEFAULT_UP_MOVEMENT_MULTIPLIER				0.7f
	#define RESOLUTION_SCALE_MAX						200.0f
	#define RESOLUTION_SCALE_MIN						25.0f
	// End Mandatory constants

	// AOB Keys for interceptor's AOB scanner
	#define CAMERA_ADDRESS_INTERCEPT_KEY				"AOB_CAMERA_ADDRESS_INTERCEPT"
	#define RESOLUTION_SCALE_INTERCEPT_KEY				"AOB_RESOLUTION_SCALE_INTERCEPT"
	#define TIMESTOP_READ_INTERCEPT_KEY					"AOB_TIMESTOP_READ_INTERCEPT"
	#define HUD_RENDER_INTERCEPT_KEY					"AOB_HUD_RENDER_INTERCEPT"
	#define HUD_RENDER_INTERCEPT_KEY2					"AOB_HUD_RENDER_INTERCEPT2"	
	#define RESOLUTION_ABSADD_INTERCEPT_KEY				"AOB_RESOLUTION_ABSADD_INTERCEPT_KEY"
	

	// Indices in the structures read by interceptors 
	#define COORDS_IN_STRUCT_OFFSET						0x410
	#define QUATERNION_IN_STRUCT_OFFSET					0x41C
	#define FOV_IN_STRUCT_OFFSET						0x428
	#define TIMESTOP_IN_STRUCT_OFFSET					0x4E8
	
}
