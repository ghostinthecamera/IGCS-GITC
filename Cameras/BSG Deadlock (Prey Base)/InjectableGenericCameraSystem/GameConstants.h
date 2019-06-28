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
	#define GAME_NAME									"BSG"
	#define CAMERA_VERSION								"1.0"
	#define CAMERA_CREDITS								"IGCS by Otis_Inf, adapted by ghostinthecamera"
	#define GAME_WINDOW_TITLE							"BattlestarGalacticaDeadlock"
	#define INITIAL_PITCH_RADIANS						0.0f	// around X axis	(right)
	#define INITIAL_YAW_RADIANS							0.0f	// around Y axis	(into the screen)
	#define INITIAL_ROLL_RADIANS						0.0f	// aruond Z axis	(up)
	#define CONTROLLER_Y_INVERT							false
	#define FASTER_MULTIPLIER							10.0f
	#define SLOWER_MULTIPLIER							0.1f
	#define MOUSE_SPEED_CORRECTION						0.2f	// to correct for the mouse-deltas related to normal rotation.
	#define DEFAULT_MOVEMENT_SPEED						10.0f
	#define DEFAULT_ROTATION_SPEED						0.01f
	#define DEFAULT_FOV_DEGREES							80.0f
	#define DEFAULT_FOV_SPEED							0.1f
	#define DEFAULT_UP_MOVEMENT_MULTIPLIER				0.7f
	#define HOTSAMPLE_FACTOR_MAX						5
	// End Mandatory constants

	// AOB Keys for interceptor's AOB scanner
	#define CAMERA_ADDRESS_INTERCEPT_KEY				"AOB_CAMERA_ADDRESS_INTERCEPT"
	#define CAMERA_WRITE1_INTERCEPT_KEY					"AOB_CAMERA_WRITE1_INTERCEPT"
	#define CAMERA_WRITE2_INTERCEPT_KEY					"AOB_CAMERA_WRITE2_INTERCEPT"
	//#define CAMERA_WRITE3_INTERCEPT_KEY					"AOB_CAMERA_WRITE3_INTERCEPT"
	//#define FOV_INTERCEPT_KEY							"AOB_FOV_INTERCEPT"
	//#define TIMESTOP_INTERCEPT_KEY						"AOB_TIMESTOP_INTERCEPT"
	//#define SUPERSAMPLING_KEY							"AOB_SUPERSAMPLING_ADDRESS"
	//#define HUD_TOGGLE_KEY								"AOB_HUD_TOGGLE_ADDRESS"

	// Indices in the structures read by interceptors 
	#define CAMERA_COORDS_IN_CAMERA_STRUCT_OFFSET		0x00
	#define LOOK_DATA_IN_CAMERA_STRUCT_OFFSET			-0x60
	#define LOOK_DATA2_IN_CAMERA_STRUCT_OFFSET			-0x80
	#define LOOK_DATA3_IN_CAMERA_STRUCT_OFFSET			0x10
	#define LOOK_DATA_SECOND_QAUTERNION					-0x50
	//#define FOV_IN_STRUCT_OFFSET						0x8
	//#define TIMESTOP_IN_STRUCT_OFFSET					0x8
}
