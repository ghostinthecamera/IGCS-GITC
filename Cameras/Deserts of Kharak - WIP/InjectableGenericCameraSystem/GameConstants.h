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
	#define GAME_NAME									"Deserts of Kharak - WIP"
	#define CAMERA_VERSION								"1.0"
	#define CAMERA_CREDITS								"GHOSTINTHECAMERA & IDK31"
	#define GAME_WINDOW_TITLE							"DESERTS OF KHARAK"
	#define INITIAL_PITCH_RADIANS						0.0f	// around X axis	(right)
	#define INITIAL_YAW_RADIANS							0.0f	// around Y axis	(out of the screen)
	#define INITIAL_ROLL_RADIANS						0.0f	// around Z axis	(up)
	#define CONTROLLER_Y_INVERT							false
	#define FASTER_MULTIPLIER							5.0f
	#define SLOWER_MULTIPLIER							0.1f
	#define MOUSE_SPEED_CORRECTION						0.2f	// to correct for the mouse-deltas related to normal rotation.
	#define DEFAULT_MOVEMENT_SPEED						0.07f
	#define DEFAULT_ROTATION_SPEED						0.01f
	#define DEFAULT_FOV_SPEED							0.0005f
	#define DEFAULT_UP_MOVEMENT_MULTIPLIER				0.7f
	#define RESOLUTION_SCALE_MAX						4.0f
	#define RESOLUTION_SCALE_MIN						0.5f
	// End Mandatory constants

	// AOB Keys for interceptor's AOB scanner
	#define CAMERA_ADDRESS_INTERCEPT_KEY				"AOB_CAMERA_ADDRESS_INTERCEPT"
	#define CAMERA_WRITE1_INTERCEPT_KEY					"AOB_CAMERA_WRITE1_INTERCEPT"
	#define CAMERA_WRITE2_INTERCEPT_KEY					"AOB_CAMERA_WRITE2_INTERCEPT"
	/*#define PHOTOMODE_RANGE_DISABLE_KEY					"AOB_PHOTOMODE_RANGE_DISABLE_INTERCEPT"
	#define RESOLUTION_SCALE_INTERCEPT_KEY				"AOB_RESOLUTION_SCALE_INTERCEPT"
	#define TOD_WRITE_INTERCEPT_KEY						"AOB_TOD_WRITE_INTERCEPT"
	#define TIMESTOP_READ_INTERCEPT_KEY					"AOB_TIMESTOP_READ_INTERCEPT"
	#define HUD_RENDER_INTERCEPT_KEY					"AOB_HUD_RENDER_INTERCEPT"
	#define PAUSE_FUNCTION_LOCATION_KEY					"AOB_PAUSE_FUNCTION"
	#define UNPAUSE_FUNCTION_LOCATION_KEY				"AOB_UNPAUSE_FUNCTION"
	#define DOF_ENABLE_WRITE_LOCATION_KEY				"AOB_DOF_WRITE_LOCATION"
	#define AR_LIMIT_LOCATION_KEY						"AOB_AR_LIMIT_LOCATION"
	#define FOG_READ_INTERCEPT_KEY						"AOB_FOG_READ_INTERCEPT"*/
	

	// Indices in the structures read by interceptors 
	#define COORDS_IN_STRUCT_OFFSET						-0xC20
	#define QUATERNION_IN_STRUCT_OFFSET					-0xC30
	#define FOV_IN_STRUCT_OFFSET						0x284
	//#define RESOLUTION_SCALE_IN_STRUCT_OFFSET			0xA4
	//#define TIMESTOP_BYTE_ONE_OFFSET					0x1790		// set byte one to 1, then byte two to 1 => stop. Set byte three to 0 and byte two to FF and things proceed again.
	//#define TIMESTOP_BYTE_TWO_OFFSET					0x1868
	//#define TIMESTOP_BYTE_THREE_OFFSET					0x1788
	//#define DOF_ENABLE1_IN_STRUCT_OFFSET				0x111		// in camera struct. 1 is enable dof, 0 is disable dof. Writes have to be disabled first. See notes for dof parameters (pretty low-quality, not added to menu).
	//#define DOF_ENABLE2_IN_STRUCT_OFFSET				0x111		// in camera struct. 1 is enable dof, 0 is disable dof. Writes have to be disabled first. See notes for dof parameters (pretty low-quality, not added to menu).
	//#define FOG_STRENGTH_IN_STRUCT_OFFSET				0x20
	//#define FOG_START_CURVE_IN_STRUCT_OFFSET			0x60
}
