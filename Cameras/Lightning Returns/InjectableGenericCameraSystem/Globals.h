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
#include "Gamepad.h"
#include "Defaults.h"
#include <map>
#include <atomic>
#include "ActionData.h"
#include <map>
#include "Settings.h"
#include "System.h"

extern "C" uint8_t g_cameraEnabled;

namespace IGCS
{
	class Globals
	{
	public:
		Globals();
		~Globals();

		static Globals& instance();

		bool inputBlocked() const { return _inputBlocked; }
		void inputBlocked(bool newValue) { _inputBlocked = newValue; }
		bool cameraMovementLocked() const { return _cameraMovementLocked; }
		void cameraMovementLocked(bool newValue) { _cameraMovementLocked = newValue; }
		bool systemActive() const { return _systemActive; }
		void systemActive(bool value) { _systemActive = value; }
		bool gamePaused() const { return _gamePaused; }
		bool* gamePausedptr() { return &_gamePaused; }
		void gamePaused(bool state) { _gamePaused = state; }
		bool sloMo() const { return _slowMo; }
		bool* sloMoptr() { return &_slowMo; }
		void sloMo(bool status) { _slowMo = status; }
		HWND mainWindowHandle() const { return _mainWindowHandle; }
		void mainWindowHandle(HWND handle) { _mainWindowHandle = handle; }
		Gamepad& gamePad() { return _gamePad; }
		Settings& settings() { return _settings; }
		bool toggleCameraMovementLocked()
		{
			_cameraMovementLocked = !_cameraMovementLocked;
			return _cameraMovementLocked;
		}
		bool toggleHudVisible()
		{
			_hudVisible = !_hudVisible;
			return _hudVisible;
		}
		bool toggleGamePaused()
		{
			_gamePaused = !_gamePaused;
			return _gamePaused;
		}
		bool toggleSlowMo()
		{
			_slowMo = !_slowMo;
			return _slowMo;
		}
		bool toggleInputBlocked()
		{
			_inputBlocked = !_inputBlocked;
			return _inputBlocked;
		}
		bool keyboardMouseControlCamera() const { return _settings.cameraControlDevice == DEVICE_ID_KEYBOARD_MOUSE || _settings.cameraControlDevice == DEVICE_ID_ALL; }
		bool controllerControlsCamera() const { return _settings.cameraControlDevice == DEVICE_ID_GAMEPAD || _settings.cameraControlDevice == DEVICE_ID_ALL; }
		ActionData* getActionData(ActionType type);
		void handleSettingMessage(uint8_t payload[], DWORD payloadLength);
		void handleKeybindingMessage(uint8_t payload[], DWORD payloadLength);
		void handleActionPayload(uint8_t payload[], DWORD payloadLength); //for action payload
		// gamespecific items
		void storeCurrentaob(map<string, AOBBlock*> aobblock) { currentAOBBlock = aobblock; }
		map<string, AOBBlock*> getCurrentAOB() { return currentAOBBlock; }

		//IGCSDOF
		void storeCurrentSystem(System* system) { currentSystem = system; }
		System* getCurrentSystem() { return currentSystem; }

	private:
		void initializeKeyBindings();

		
		IGCS::System* currentSystem; //IGCSDOF
		map<string, AOBBlock*> currentAOBBlock;
		bool _inputBlocked = true;
		bool _cameraMovementLocked = false;
		atomic_bool _systemActive = false;
		Gamepad _gamePad;
		HWND _mainWindowHandle;
		Settings _settings;
		map<ActionType, ActionData*> _keyBindingPerActionType;
		bool _hudVisible = true;
		bool _gamePaused = false;
		bool _slowMo = false;
	};
}