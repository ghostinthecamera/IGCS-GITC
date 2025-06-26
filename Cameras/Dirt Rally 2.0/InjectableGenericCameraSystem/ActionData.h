////////////////////////////////////////////////////////////////////////////////////////////////////////
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
#include "stdafx.h"

namespace IGCS
{
	enum class ActionType : uint8_t
	{
		BlockInput = 0,
		CameraEnable = 1,
		FovDecrease = 2,
		FovIncrease = 3,
		FovReset = 4,
		CameraLock = 5,
		MoveLeft = 6,
		MoveRight = 7,
		MoveForward = 8,
		MoveBackward = 9,
		MoveUp = 10,
		MoveDown = 11,
		RotateUp = 12,
		RotateDown = 13,
		RotateLeft = 14,
		RotateRight = 15,
		TiltLeft = 16,
		TiltRight = 17,
		Timestop = 18,
		SkipFrames = 19,
		SlowMo = 20,
		HUDtoggle = 21,
		PathVisualizationToggle = 22,
		PathPlayStop = 23,
		PathAddNode = 24,
		PathCreate = 25,
		PathDelete = 26,
		PathCycle = 27,
		PathDeleteLastNode = 28,
		CycleDepthBuffers = 29,
		ToggleDepthBuffer = 30,
		GamepadSlowModifier = 31,
		GamepadFastModifier = 32,
		ResetLookAtOffsets = 33,
		ToggleOffsetMode = 34,
		ToggleHeightLockedMovement = 35,
		ToggleFixedCameraMount = 36,
		ToggleLookAtVisualization = 37,
		MoveUpTarget = 38,
		MoveDownTarget = 39,

		Amount,
	};

	enum class InputSource : uint8_t
	{
		Keyboard,
		Gamepad
	};

	class ActionData
	{
	public:
		ActionData(std::string name, int keyCode, bool altRequired, bool ctrlRequired, bool shiftRequired)
			: ActionData(name, keyCode, altRequired, ctrlRequired, shiftRequired, true, InputSource::Keyboard) {
		};

		ActionData(std::string name, int keyCode, bool altRequired, bool ctrlRequired, bool shiftRequired, bool available)
			: ActionData(name, keyCode, altRequired, ctrlRequired, shiftRequired, available, InputSource::Keyboard) {
		};

		// Constructor with input source
		ActionData(std::string name, int keyCode, bool altRequired, bool ctrlRequired, bool shiftRequired, bool available, InputSource source)
			: _name{ name }, _keyCode{ keyCode }, _altRequired{ altRequired }, _ctrlRequired{ ctrlRequired }, _shiftRequired{ shiftRequired },
			_available{ available }, _inputSource{ source }
		{
		}

		~ActionData();

		bool isActive(bool ignoreAltCtrl);
		void clear();
		void update(int newKeyCode, bool altRequired, bool ctrlRequired, bool shiftRequired, bool isGamepad);
		void setKeyCode(int newKeyCode);

		std::string getName() { return _name; }
		int getKeyCode() const { return _keyCode; }
		// If false, the action is ignored to be edited / in help. Code isn't anticipating on it either, as it's not supported in this particular camera. 
		bool getAvailable() { return _available; }
		bool isValid() { return _keyCode > 0; }
		void setAltRequired() { _altRequired = true; }
		void setCtrlRequired() { _ctrlRequired = true; }
		void setShiftRequired() { _shiftRequired = true; }

		InputSource getInputSource() const { return _inputSource; }
		void setInputSource(InputSource source) { _inputSource = source; }

	private:
		std::string _name;
		int _keyCode;
		bool _altRequired;
		bool _ctrlRequired;
		bool _shiftRequired;
		bool _available;
		InputSource _inputSource;
	};
}