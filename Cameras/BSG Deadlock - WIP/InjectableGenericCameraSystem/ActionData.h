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
		BlockInput=0,
		CameraEnable,
		FovDecrease,
		FovIncrease,
		FovReset,
		CameraLock,
		MoveLeft,
		MoveRight,
		MoveForward,
		MoveBackward,
		MoveUp,
		MoveDown,
		RotateUp,
		RotateDown,
		RotateLeft,
		RotateRight,
		TiltLeft,
		TiltRight,
		HudToggle,
		Timestop,

		Amount,
	};


	class ActionData
	{
	public:
		ActionData(std::string name, std::string description, int keyCode, bool altRequired, bool ctrlRequired, bool shiftRequired)
			: ActionData(name, description, keyCode, altRequired, ctrlRequired, shiftRequired, true) { };
		ActionData(std::string name, std::string description, int keyCode, bool altRequired, bool ctrlRequired, bool shiftRequired, bool available);
		~ActionData();

		bool isActive(bool ignoreAltCtrl);
		int getIniFileValue();
		void setValuesFromIniFileValue(int iniFileValue);
		void clear();
		void update(uint8_t newKeyCode, bool altRequired, bool ctrlRequired, bool shiftRequired);
		void setKeyCode(int newKeyCode);

		std::string getName() { return _name; }
		std::string getDescription() { return _description; }
		// If false, the action is ignored to be edited / in help. Code isn't anticipating on it either, as it's not supported in this particular camera. 
		bool getAvailable() { return _available; }
		bool isValid() { return _keyCode > 0; }
		void setAltRequired() { _altRequired = true; }
		void setCtrlRequired() { _ctrlRequired = true; }
		void setShiftRequired() { _shiftRequired = true; }

	private:
		bool ctrlPressed();
		bool shiftPressed();

		std::string _name;
		std::string _description;
		int _keyCode;
		bool _altRequired;
		bool _ctrlRequired;
		bool _shiftRequired;
		bool _available;
	};
}