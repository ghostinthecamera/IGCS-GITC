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
#include "stdafx.h"
#include "Globals.h"
#include "GameConstants.h"
#include "NamedPipeManager.h"

//--------------------------------------------------------------------------------------------------------------------------------
// data shared with asm functions. This is allocated here, 'C' style and not in some datastructure as passing that to 
// MASM is rather tedious. 
extern "C" {
	uint8_t g_cameraEnabled = 0;
}


namespace IGCS
{
	Globals::Globals()
	{
		initializeKeyBindings();
		_settings.init(false);
	}


	Globals::~Globals()
	{
	}


	Globals &Globals::instance()
	{
		static Globals theInstance;
		return theInstance;
	}

	
	void Globals::handleSettingMessage(uint8_t payload[], DWORD payloadLength)
	{
		_settings.setValueFromMessage(payload, payloadLength);
		
	}

	
	void Globals::handleKeybindingMessage(uint8_t payload[], DWORD payloadLength)
	{
		if(payloadLength<6)
		{
			return;
		}
		// byte 1 is the keybinding id, byte 2-5 are the keybinding data:
		//payload[2] = _keyCode;
		//payload[3] = _altPressed;
		//payload[4] = _ctrlPressed;
		//payload[5] = _shiftPressed;
		ActionType idOfBinding = static_cast<ActionType>(payload[1]);
		ActionData* toUpdate = getActionData(idOfBinding);
		if (nullptr == toUpdate)
		{
			return;
		}
		toUpdate->update(payload[2], payload[3] == 0x01, payload[4] == 0x01, payload[5] == 0x01);
	}


	ActionData* Globals::getActionData(ActionType type)
	{
		if (_keyBindingPerActionType.count(type) != 1)
		{
			return nullptr;
		}
		return _keyBindingPerActionType.at(type);
	}
	

	void Globals::initializeKeyBindings()
	{
		// initialize the bindings with defaults. First the features which are always supported.
		_keyBindingPerActionType[ActionType::BlockInput] = new ActionData("BlockInput", IGCS_KEY_BLOCK_INPUT, false, false, false);
		_keyBindingPerActionType[ActionType::CameraEnable] = new ActionData("CameraEnable", IGCS_KEY_CAMERA_ENABLE, false, false, false);
		_keyBindingPerActionType[ActionType::CameraLock] = new ActionData("CameraLock", IGCS_KEY_CAMERA_LOCK, false, false, false);
		_keyBindingPerActionType[ActionType::FovDecrease] = new ActionData("FovDecrease", IGCS_KEY_FOV_DECREASE, false, false, false);
		_keyBindingPerActionType[ActionType::FovIncrease] = new ActionData("FovIncrease", IGCS_KEY_FOV_INCREASE, false, false, false);
		_keyBindingPerActionType[ActionType::FovReset] = new ActionData("FovReset", IGCS_KEY_FOV_RESET, false, false, false);
		_keyBindingPerActionType[ActionType::MoveBackward] = new ActionData("MoveBackward", IGCS_KEY_MOVE_BACKWARD, false, false, false);
		_keyBindingPerActionType[ActionType::MoveDown] = new ActionData("MoveDown", IGCS_KEY_MOVE_DOWN, false, false, false);
		_keyBindingPerActionType[ActionType::MoveForward] = new ActionData("MoveForward", IGCS_KEY_MOVE_FORWARD, false, false, false);
		_keyBindingPerActionType[ActionType::MoveLeft] = new ActionData("MoveLeft", IGCS_KEY_MOVE_LEFT, false, false, false);
		_keyBindingPerActionType[ActionType::MoveRight] = new ActionData("MoveRight", IGCS_KEY_MOVE_RIGHT, false, false, false);
		_keyBindingPerActionType[ActionType::MoveUp] = new ActionData("MoveUp", IGCS_KEY_MOVE_UP, false, false, false);
		_keyBindingPerActionType[ActionType::RotateDown] = new ActionData("RotateDown", IGCS_KEY_ROTATE_DOWN, false, false, false);
		_keyBindingPerActionType[ActionType::RotateLeft] = new ActionData("RotateLeft", IGCS_KEY_ROTATE_LEFT, false, false, false);
		_keyBindingPerActionType[ActionType::RotateRight] = new ActionData("RotateRight", IGCS_KEY_ROTATE_RIGHT, false, false, false);
		_keyBindingPerActionType[ActionType::RotateUp] = new ActionData("RotateUp", IGCS_KEY_ROTATE_UP, false, false, false);
		_keyBindingPerActionType[ActionType::TiltLeft] = new ActionData("TiltLeft", IGCS_KEY_TILT_LEFT, false, false, false);
		_keyBindingPerActionType[ActionType::TiltRight] = new ActionData("TiltRight", IGCS_KEY_TILT_RIGHT, false, false, false);
		_keyBindingPerActionType[ActionType::HudToggle] = new ActionData("HudToggle", IGCS_KEY_HUD_TOGGLE, false, false, false);
		_keyBindingPerActionType[ActionType::Timestop] = new ActionData("Timestop", IGCS_KEY_TIMESTOP, false, false, false);
		_keyBindingPerActionType[ActionType::SkipFrames] = new ActionData("SkipFrames", IGCS_KEY_SKIP_FRAMES, false, false, false);
	}
}