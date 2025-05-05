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
#include "PathManager.h"
#include "D3DHook.h"

namespace IGCS
{

	class System
	{
	public:
		// Singleton access method
		static System& instance()
		{
			static System instance;
			return instance;
		}

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
		static void clearMovementData();
		void toggleGamePause(bool displayNotification = true);
		GameAddressData getAddressData() { return _addressData; }
		uint8_t IGCSsuccess = 0;
		uint8_t IGCScamenabled = 1;
		uint8_t IGCSsessionactive = 3;
		bool pathRun = false;
		void updateFrame();
		void timestop();
		void frameSkip();
		float getDT() const { return _deltaTime; }
        map<string, AOBBlock>& getAOBBlock() { return _aobBlocks; }
		bool sysReady = false;

	private:
		System();
		~System();

		// Delete copy constructor and assignment operator
		System(const System&) = delete;
		System& operator=(const System&) = delete;

		static void mainLoop();
		void initialize();
		
		void cameraStateProcessor();
		[[nodiscard]] float getDeltaTime() const;
		static bool checkIfGamehasFocus();
		void handlePathManagerInput();	
		void handleUserInput();
		static void displayCameraState();
		static void toggleCameraMovementLockState();
		static void handleKeyboardCameraMovement(float multiplier);
		void checkDXHookRequired();
		static void handleMouseCameraMovement(float multiplier);
		void handleGamePadMovement();
		void waitForCameraStructAddresses();
		static void toggleInputBlockState();
		void toggleHud();
		
		void toggleSlowMo(bool displaynotification = true);
		void handleSkipFrames();
		//game specific
		void togglePlayerOnly();
		void toggleBytePause(bool displayNotification = true);
		void updateDeltaTime();
		bool _blocksInit = false;
		bool _cameraStructInit = false;
		bool _postCameraStructInit = false;


		/// <summary>
		/// IGCSDOF
		/// </summary>
		void setIGCSsession(bool status, uint8_t type) { _IGCSConnectorSessionActive = status, _IGCSConnecterSessionType = type; }
		IGCSSessionCacheData _igcscacheData; 
		bool _IGCSConnectorSessionActive = false;
		uint8_t _IGCSConnecterSessionType = DEFAULT_IGCS_TYPE;
		//

		//Camera _camera;
		GameCameraData _originalData;
		GameAddressData _addressData;
		LPBYTE _hostImageAddress;
		DWORD _hostImageSize;
		bool _cameraStructFound = false;

		map<string, AOBBlock> _aobBlocks;
		bool _applyHammerPrevention = false;	// set to true by a keyboard action and which triggers a sleep before keyboard handling is performed.
		std::filesystem::path _hostExePath;
		std::filesystem::path _hostExeFilename;

		//interpolation stuff
		LARGE_INTEGER _lastFrameTime;
		LARGE_INTEGER _frequency;
		float _deltaTime = 0.0f; // In seconds
		float _smoothness = 5.0f; // Adjustable smoothness factor
		// Timing configuration
		bool _useFixedDeltaTime;
		float _fixedDeltaValue;

		bool _visualizationEnabled = false;
		static void toggledepthBufferUsage();

		//D3DPresent frame skip
		bool  _skipActive = false;   // true while we’re “stepping”
		int   _skipFramesLeft = 0;       // how many Present calls to wait
		float _skipTimeLeft = 0.0f;    // (optional) time?based skipping
	};
}

