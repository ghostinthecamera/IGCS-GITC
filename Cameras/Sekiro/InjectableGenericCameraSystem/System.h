////////////////////////////////////////////////////////////////////////////////////////////////////////
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
#include "stdafx.h"
#include "Camera.h"
#include "InputHooker.h"
#include "Gamepad.h"
#include <map>
#include "AOBBlock.h"
#include "GameCameraData.h" //IGCSDOF

namespace IGCS
{

	class System
	{
	public:
		System();

		~System();

		void start(LPBYTE hostBaseAddress, DWORD hostImageSize);

		/// <summary>
		/// IGCSDOF
		/// </summary>
		/// <param name="type"></param>
		/// <returns></returns>
		uint8_t startIGCSsession(uint8_t type);
		void endIGCSsession();
		void stepCameraforIGCSSession(float stepAngle);
		void stepCameraforIGCSSession(float stepLeftRight, float stepUpDown, float fovDegrees, bool fromStartPosition);
		void clearMovementData();
		GameAddressData getAddressData() { return _addressData; }
		uint8_t IGCSsuccess = 0;
		uint8_t IGCScamenabled = 1;
		uint8_t IGCSsessionactive = 3;

	private:
		void mainLoop();
		void initialize();
		void updateFrame();
		bool checkIfGamehasFocus();
		void handleUserInput();
		void displayCameraState();
		void toggleCameraMovementLockState();
		void handleKeyboardCameraMovement(float multiplier);
		void handleMouseCameraMovement(float multiplier);
		void handleGamePadMovement(float multiplierBase);
		void waitForCameraStructAddresses();
		void toggleInputBlockState();
		void toggleHud();
		void toggleGamePause(bool displayNotification = true);
		void toggleSlowMo(bool displaynotification = true);
		void handleSkipFrames();
		//game specific
		void togglePlayerOnly();
		void toggleBytePause(bool displayNotification = true);

		/// <summary>
		/// IGCSDOF
		/// </summary>
		void setIGCSsession(bool status, uint8_t type) { _IGCSConnectorSessionActive = status, _IGCSConnecterSessionType = type; }
		igcsSessionCacheData _igcscacheData; 
		bool _IGCSConnectorSessionActive = false;
		uint8_t _IGCSConnecterSessionType = DEFAULT_IGCS_TYPE;
		//

		Camera _camera;
		GameCameraData _originalData;
		GameAddressData _addressData;
		LPBYTE _hostImageAddress;
		DWORD _hostImageSize;
		bool _cameraStructFound = false;

		map<string, AOBBlock*> _aobBlocks;
		bool _applyHammerPrevention = false;	// set to true by a keyboard action and which triggers a sleep before keyboard handling is performed.
		std::filesystem::path _hostExePath;
		std::filesystem::path _hostExeFilename;
	};
}

