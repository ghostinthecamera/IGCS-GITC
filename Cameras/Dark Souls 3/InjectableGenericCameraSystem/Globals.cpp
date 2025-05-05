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

#include "Console.h"
#include "GameConstants.h"
#include "NamedPipeManager.h"
#include "Input.h"
#include "Gamepad.h"

//--------------------------------------------------------------------------------------------------------------------------------
// data shared with asm functions. This is allocated here, 'C' style and not in some datastructure as passing that to 
// MASM is rather tedious. 
extern "C" {
	uint8_t g_cameraEnabled = 0;
	uint8_t g_playersonly = 0;
	uint8_t g_hideplayer = 0;
	uint8_t g_hideNPC = 0;
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

	//for handling actions with a payload (like hotsampling)
	void Globals::handleActionPayload(uint8_t payload[], DWORD payloadLength)
	{
		_settings.setValueFromAction(payload, payloadLength);

	}

	void Globals::handleKeybindingMessage(uint8_t payload[], DWORD payloadLength)
	{
		if (payloadLength < 7)
		{
			IGCS::Console::WriteError("Error: payloadLength < 7, returning early");
			return;
		}

		ActionType idOfBinding = static_cast<ActionType>(payload[1]);
		uint8_t keyCodeByte = payload[2];
		bool altPressed = payload[3] == 0x01;
		bool ctrlPressed = payload[4] == 0x01;
		bool shiftPressed = payload[5] == 0x01;
		bool isGamepadButton = payload[6] == 0x01;

		// Determine which ActionData to update
		const InputSource inputType = isGamepadButton ? InputSource::Gamepad : InputSource::Keyboard;

		ActionData* toUpdate = (inputType == InputSource::Keyboard) ?
			getActionData(idOfBinding) : getGamePadActionData(idOfBinding);

		if (nullptr == toUpdate)
		{
			MessageHandler::logError("Key Binding Action Data could not be found - binding was not changed");
			return;
		}

		// Get key code to use - start with current value as default
		int keyCode = toUpdate->getKeyCode();

		// For gamepad buttons, convert ID to XInput mask if valid
		if (isGamepadButton)
		{
			if (const auto xinputMask = Gamepad::idToXInputMask(keyCodeByte); xinputMask.has_value()) keyCode = xinputMask.value();
			else MessageHandler::logError("Invalid gamepad button ID provided, no update to keyCode");
		}
		else keyCode = keyCodeByte;

		toUpdate->update(keyCode, altPressed, ctrlPressed, shiftPressed, isGamepadButton);
	}


	// Keep the original method unchanged
	ActionData* Globals::getActionData(ActionType type)
	{
		if (_keyBindingPerActionType.count(type) != 1)
		{
			return nullptr;
		}
		return _keyBindingPerActionType.at(type);
	}

	// New method to get alternate binding
	ActionData* Globals::getGamePadActionData(ActionType type)
	{
		if (_gamepadKeyBindingPerActionType.count(type) != 1)
		{
			return nullptr;
		}
		return _gamepadKeyBindingPerActionType.at(type);
	}

	// Check if any binding for this action is active
	bool Globals::isAnyBindingActivated(ActionType type, bool altCtrlOptional)
	{
		ActionData* primaryBinding = getActionData(type);
		ActionData* altBinding = getGamePadActionData(type);

		// Check primary binding
		if (primaryBinding && primaryBinding->isActive(altCtrlOptional))
		{
			return true;
		}

		// Check alternate binding
		if (altBinding && altBinding->isActive(altCtrlOptional))
		{
			return true;
		}

		return false;
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
		_keyBindingPerActionType[ActionType::Timestop] = new ActionData("Timestop", IGCS_KEY_TIMESTOP, false, false, false);
		_keyBindingPerActionType[ActionType::SkipFrames] = new ActionData("SkipFrames", IGCS_KEY_SKIP_FRAMES, false, false, false);
		_keyBindingPerActionType[ActionType::SlowMo] = new ActionData("SlowMo", IGCS_KEY_SLOWMO, false, false, false);
		_keyBindingPerActionType[ActionType::HUDtoggle] = new ActionData("HUDtoggle", IGCS_KEY_HUD, false, false, false);
		_keyBindingPerActionType[ActionType::PlayerOnly] = new ActionData("PlayerOnly", IGCS_KEY_PLAYERONLY, false, false, false);
		_keyBindingPerActionType[ActionType::CycleDepthBuffers] = new ActionData("CycleDepthBuffers", IGCS_KEY_CYCLEDEPTH, false, false, false);
		_keyBindingPerActionType[ActionType::ToggleDepthBuffer] = new ActionData("ToggleDepthBuffer", IGCS_KEY_PATH_TOGGLE_DEPTH, false, false, false);

		// Path manager keyboard bindings
		_keyBindingPerActionType[ActionType::PathVisualizationToggle] = new ActionData("PathVisualizationToggle", IGCS_KEY_PATH_VISUALIZATION_TOGGLE, false, false, false);
		_keyBindingPerActionType[ActionType::PathPlayStop] = new ActionData("PathPlayStop", IGCS_KEY_PATH_PLAY_STOP, false, false, false);
		_keyBindingPerActionType[ActionType::PathCreate] = new ActionData("PathCreate", IGCS_KEY_PATH_CREATE, false, false, false);
		_keyBindingPerActionType[ActionType::PathAddNode] = new ActionData("PathAddNode", IGCS_KEY_PATH_ADD_NODE, false, false, false);
		_keyBindingPerActionType[ActionType::PathDelete] = new ActionData("PathDelete", IGCS_KEY_PATH_DELETE, false, false, false);
		_keyBindingPerActionType[ActionType::PathCycle] = new ActionData("PathCycle", IGCS_KEY_PATH_CYCLE, false, false, false);
		_keyBindingPerActionType[ActionType::PathDeleteLastNode] = new ActionData("PathDeleteLastNode", IGCS_KEY_PATH_DELETE_LAST_NODE, false, false, false);


		// Path manager alternate gamepad bindings
		_gamepadKeyBindingPerActionType[ActionType::FovDecrease] = new ActionData("FovDecrease", IGCS_BUTTON_FOV_DECREASE, false, false, false, true, InputSource::Gamepad);
		_gamepadKeyBindingPerActionType[ActionType::FovIncrease] = new ActionData("FovIncrease", IGCS_BUTTON_FOV_INCREASE, false, false, false, true, InputSource::Gamepad);
		_gamepadKeyBindingPerActionType[ActionType::GamepadFastModifier] = new ActionData("GPFast", IGCS_BUTTON_FASTER, false, false, false, true, InputSource::Gamepad);
		_gamepadKeyBindingPerActionType[ActionType::GamepadSlowModifier] = new ActionData("GPSlow", IGCS_BUTTON_SLOWER, false, false, false, true, InputSource::Gamepad);
		_gamepadKeyBindingPerActionType[ActionType::TiltLeft] = new ActionData("GPTiltLeft", IGCS_BUTTON_TILT_LEFT, false, false, false, true, InputSource::Gamepad);
		_gamepadKeyBindingPerActionType[ActionType::TiltRight] = new ActionData("GPTiltRight", IGCS_BUTTON_TILT_RIGHT, false, false, false, true, InputSource::Gamepad);
		_gamepadKeyBindingPerActionType[ActionType::FovReset] = new ActionData("GPFovReset", IGCS_BUTTON_RESET_FOV, false, false, false, true, InputSource::Gamepad);
		_gamepadKeyBindingPerActionType[ActionType::PathVisualizationToggle] = new ActionData("GPadPathVisualizationToggle", IGCS_BUTTON_PATH_VISUALIZATION_TOGGLE, false, true, false, true, InputSource::Gamepad);
		_gamepadKeyBindingPerActionType[ActionType::PathPlayStop] = new ActionData("GPadPathPlay", IGCS_BUTTON_PATH_PLAY_STOP, false, true, false, true, InputSource::Gamepad);
		_gamepadKeyBindingPerActionType[ActionType::PathCreate] = new ActionData("GPadPathAddPath", IGCS_BUTTON_PATH_CREATE, false, true, false, true, InputSource::Gamepad);
		_gamepadKeyBindingPerActionType[ActionType::PathAddNode] = new ActionData("GPaDPathAddNode", IGCS_BUTTON_PATH_ADD_NODE, false, true, false, true, InputSource::Gamepad);
		_gamepadKeyBindingPerActionType[ActionType::PathDelete] = new ActionData("GPPathDelete", IGCS_BUTTON_PATH_DELETE, false, true, false, true ,InputSource::Gamepad);
		_gamepadKeyBindingPerActionType[ActionType::PathCycle] = new ActionData("GPPathCycle", IGCS_BUTTON_PATH_CYCLE, false, true, false, true, InputSource::Gamepad);
		_gamepadKeyBindingPerActionType[ActionType::PathDeleteLastNode] = new ActionData("GPPathDeleteLastNode", IGCS_BUTTON_PATH_DELETE_LAST_NODE, false, true, false,true, InputSource::Gamepad);

	}
}