////////////////////////////////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System
// Copyright(c) 2016, Frans Bouma
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

// Mandatory constants to define for a game
#define GAME_NAME									"Battlestar Galactica"
#define CAMERA_VERSION								"1.0"
#define CAMERA_CREDITS								"ghostinthecamera"
#define GAME_WINDOW_TITLE							"BattlestarGalacticaDeadlock"
#define INITIAL_PITCH_RADIANS						0.0f
#define INITIAL_YAW_RADIANS							0.0f
#define INITIAL_ROLL_RADIANS						0.0f
#define CONTROLLER_Y_INVERT							true
// End Mandatory constants

// Offsets for camera intercept code. Used in interceptor.
#define CAMERA_ADDRESS_INTERCEPT_START_OFFSET		0x0923F6
#define CAMERA_ADDRESS_INTERCEPT_CONTINUE_OFFSET	0x0923FC 

#define	CAMERA_WRITE_QAUTERNION_START_OFFSET		0x092A09  
#define CAMERA_WRITE_QUATERNION_CONTINUE_OFFSET		0x092A10

//#define	CAMERA_WRITE_INTERCEPT2_START_OFFSET	0x5D44B9
//#define CAMERA_WRITE_INTERCEPT2_CONTINUE_OFFSET	0x5D44CE

/*#define GAMESPEED_ADDRESS_INTERCEPT_START_OFFSET	0x2E6284
#define GAMESPEED_ADDRESS_INTERCEPT_CONTINUE_OFFSET 0x2E628A*/		

//#define FOV_WRITE_INTERCEPT1_START_OFFSET			0x5D44E1
//#define FOV_WRITE_INTERCEPT1_CONTINUE_OFFSET		0x5D44E7
//#define FOV_WRITE_INTERCEPT2_START_OFFSET			0x5D005A
//#define FOV_WRITE_INTERCEPT2_CONTINUE_OFFSET		0x5D0062

// Indices in the structures read by interceptors 
#define LOOK_QUATERNION_IN_CAMERA_STRUCT_OFFSET		0x70			
#define CAMERA_COORDS_IN_CAMERA_STRUCT_OFFSET		0x60			
/*#define FOV_IN_CAMERA_STRUCT_OFFSET				-0x78
#define GAMESPEED_IN_STRUCT_OFFSET					0x104	*/		

