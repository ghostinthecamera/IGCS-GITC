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
#include "stdafx.h"
#include "Globals.h"
#include "GameConstants.h"

//--------------------------------------------------------------------------------------------------------------------------------
// data shared with asm functions. This is allocated here, 'C' style and not in some datastructure as passing that to 
// MASM is rather tedious. 
extern "C" {
	BYTE g_cameraEnabled = 0;
}


namespace IGCS
{
	Globals::Globals()
	{
		initializeKeyBindings();
		_settings.init(false);
		_settings.loadFromFile(_keyBindingPerActionType);
	}


	Globals::~Globals()
	{
	}


	Globals &Globals::instance()
	{
		static Globals theInstance;
		return theInstance;
	}


	void Globals::saveSettingsIfRequired(float delta)
	{
		if (_settingsDirtyTimer <= 0.0f)
		{
			// nothing to do
			return;
		}
		_settingsDirtyTimer -= delta;
		if (_settingsDirtyTimer <= 0)
		{
			_settings.saveToFile(_keyBindingPerActionType);
			_settingsDirtyTimer = 0.0f;
		}
	}


	void Globals::updateActionDataForAction(ActionType type)
	{
		if (!_keyCollectorData.isValid())
		{
			// not valid, no need to set it.
			return;
		}
		ActionData* toUpdate = getActionData(type);
		if (nullptr == toUpdate)
		{
			return;
		}
		toUpdate->update(_keyCollectorData);
		// mark settings as dirty, so it's persisted, as we potentially set a new keybinding.
		markSettingsDirty();
	}


	void Globals::markSettingsDirty()
	{
		if (_settingsDirtyTimer <= 0)
		{
			_settingsDirtyTimer = IGCS_SPLASH_DURATION;
		}
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
		_keyBindingPerActionType[ActionType::BlockInput] = new ActionData("BlockInput", "Block input to game", IGCS_KEY_BLOCK_INPUT, false, false, false);
		_keyBindingPerActionType[ActionType::CameraEnable] = new ActionData("CameraEnable", "Enable / Disable camera", IGCS_KEY_CAMERA_ENABLE, false, false, false);
		_keyBindingPerActionType[ActionType::CameraLock] = new ActionData("CameraLock", "Lock / unlock camera movement", IGCS_KEY_CAMERA_LOCK, false, false, false);
		_keyBindingPerActionType[ActionType::FovDecrease] = new ActionData("FovDecrease", "Decrease FoV", IGCS_KEY_FOV_DECREASE, false, false, false);
		_keyBindingPerActionType[ActionType::FovIncrease] = new ActionData("FovIncrease", "Increase FoV", IGCS_KEY_FOV_INCREASE, false, false, false);
		_keyBindingPerActionType[ActionType::FovReset] = new ActionData("FovReset", "Reset FoV", IGCS_KEY_FOV_RESET, false, false, false);
		_keyBindingPerActionType[ActionType::MoveBackward] = new ActionData("MoveBackward", "Move camera backward", IGCS_KEY_MOVE_BACKWARD, false, false, false);
		_keyBindingPerActionType[ActionType::MoveDown] = new ActionData("MoveDown", "Move camera down", IGCS_KEY_MOVE_DOWN, false, false, false);
		_keyBindingPerActionType[ActionType::MoveForward] = new ActionData("MoveForward", "Move camera forward", IGCS_KEY_MOVE_FORWARD, false, false, false);
		_keyBindingPerActionType[ActionType::MoveLeft] = new ActionData("MoveLeft", "Move camera left", IGCS_KEY_MOVE_LEFT, false, false, false);
		_keyBindingPerActionType[ActionType::MoveRight] = new ActionData("MoveRight", "Move camera right", IGCS_KEY_MOVE_RIGHT, false, false, false);
		_keyBindingPerActionType[ActionType::MoveUp] = new ActionData("MoveUp", "Move camera up", IGCS_KEY_MOVE_UP, false, false, false);
		_keyBindingPerActionType[ActionType::RotateDown] = new ActionData("RotateDown", "Rotate camera down", IGCS_KEY_ROTATE_DOWN, false, false, false);
		_keyBindingPerActionType[ActionType::RotateLeft] = new ActionData("RotateLeft", "Rotate camera left", IGCS_KEY_ROTATE_LEFT, false, false, false);
		_keyBindingPerActionType[ActionType::RotateRight] = new ActionData("RotateRight", "Rotate camera right", IGCS_KEY_ROTATE_RIGHT, false, false, false);
		_keyBindingPerActionType[ActionType::RotateUp] = new ActionData("RotateUp", "Rotate camera up", IGCS_KEY_ROTATE_UP, false, false, false);
		_keyBindingPerActionType[ActionType::TiltLeft] = new ActionData("TiltLeft", "Tilt camera left", IGCS_KEY_TILT_LEFT, false, false, false);
		_keyBindingPerActionType[ActionType::TiltRight] = new ActionData("TiltRight", "Tilt camera right", IGCS_KEY_TILT_RIGHT, false, false, false);
		_keyBindingPerActionType[ActionType::ToggleOverlay] = new ActionData("ToggleOverlay", "Show / hide camera tools main window", IGCS_KEY_TOGGLE_OVERLAY, false, true, false);

		// Bindings which are often optional. Specify 'false' for available to disable it if the binding should be hidden. 
		_keyBindingPerActionType[ActionType::HudToggle] = new ActionData("HudToggle", "Toggle HUD", IGCS_KEY_HUD_TOGGLE, false, false, false);
		_keyBindingPerActionType[ActionType::Timestop] = new ActionData("Timestop", "Pause / unpause the game", IGCS_KEY_TIMESTOP, false, false, false);
	}
}