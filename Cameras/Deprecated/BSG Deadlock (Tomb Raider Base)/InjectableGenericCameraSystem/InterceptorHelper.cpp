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
// Game specific code. The interface is defined in InterceptorHelper.h, these methods have to be implemented, that's it.
// Use asm functions to inject the hooks
/////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "InterceptorHelper.h"

using namespace std;
//--------------------------------------------------------------------------------------------------------------------------------
// external asm functions

extern "C" {
	void cameraAddressInterceptor();
	void cameraQuaternionWrite();		// create as much interceptors for write interception as needed. In the example game, there are 4.
	/*void cameraWriteInterceptor2();
	void fovWriteInterceptor1();
	void fovWriteInterceptor2();
	void gamespeedAddressInterceptor();*/
}

extern "C" {
	// The continue address for continuing execution after camera values address interception. 
	LPBYTE _cameraStructInterceptionContinue = 0;
	// Secondary camera struct, for alternative timestop. No FoV in this case.
	LPBYTE _cameraQuaternionWriteInterceptionContinue = 0;
	//// the continue address for continuing execution after interception of the first block of code which writes to the camera values. 
	//LPBYTE _cameraStructAddressInterceptor1 = 0;
	//// the continue address for continuing execution after interception of the second block of code which writes to the camera values. 
	//LPBYTE _cameraWriteInterceptionContinue2 = 0;
	//// the continue address for continuing execution after interception of the first block of code which writes to the fov values. 
	//LPBYTE _fovWriteInterceptionContinue1 = 0;
	//// the continue address for continuing execution after interception of the second block of code which writes to the fov values. 
	//LPBYTE _fovWriteInterceptionContinue2 = 0;
	//// the continue address for the continuing execution after interception of the gamespeed block of code. 
	//LPBYTE _gamespeedInterceptionContinue = 0;
}

static LPBYTE _cameraStructInterceptionStart = 0;
static LPBYTE _cameraQuaternionWriteStart = 0;
//static LPBYTE _cameraWriteInterceptionStart2 = 0;
//static LPBYTE _fovWriteInterceptionStart1 = 0;
//static LPBYTE _fovWriteInterceptionStart2 = 0;
//static LPBYTE _gamespeedInterceptionStart = 0;


void SetCameraStructInterceptorHook(LPBYTE hostImageAddress)
{
	SetHook(hostImageAddress, CAMERA_ADDRESS_INTERCEPT_START_OFFSET, CAMERA_ADDRESS_INTERCEPT_CONTINUE_OFFSET, &_cameraStructInterceptionContinue, &cameraAddressInterceptor);
}


void SetCameraWriteInterceptorHooks(LPBYTE hostImageAddress)
{
	// for each block of code that writes to the camera values we're manipulating we need an interception to block it. For the example game there are 3. 
	SetHook(hostImageAddress, CAMERA_WRITE_QAUTERNION_START_OFFSET, CAMERA_WRITE_QUATERNION_CONTINUE_OFFSET, &_cameraQuaternionWriteInterceptionContinue, &cameraQuaternionWrite);
	//SetHook(hostImageAddress, CAMERA_WRITE_INTERCEPT2_START_OFFSET, CAMERA_WRITE_INTERCEPT2_CONTINUE_OFFSET, &_cameraWriteInterceptionContinue2, &cameraWriteInterceptor2);
}

//void SetTimestopInterceptorHook(LPBYTE hostImageAddress)
//{
//	SetHook(hostImageAddress, GAMESPEED_ADDRESS_INTERCEPT_START_OFFSET, GAMESPEED_ADDRESS_INTERCEPT_CONTINUE_OFFSET, &_gamespeedInterceptionContinue, &gamespeedAddressInterceptor);
//}


//void SetFoVWriteInterceptorHooks(LPBYTE hostImageAddress)
//{
//	SetHook(hostImageAddress, FOV_WRITE_INTERCEPT1_START_OFFSET, FOV_WRITE_INTERCEPT1_CONTINUE_OFFSET, &_fovWriteInterceptionContinue1, &fovWriteInterceptor1);
//	SetHook(hostImageAddress, FOV_WRITE_INTERCEPT2_START_OFFSET, FOV_WRITE_INTERCEPT2_CONTINUE_OFFSET, &_fovWriteInterceptionContinue2, &fovWriteInterceptor2);
//}

