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
#include "ActionData.h"

#include "Globals.h"
#include "Utils.h"

using namespace std;

namespace IGCS
{
	ActionData::~ActionData() {}

	// altCtrlOptional is only effective for actions which don't have alt/ctrl as a required key. Actions which do have one or more of these
	// keys as required, will ignore altCtrlOptional and always test for these keys. 
    bool ActionData::isActive(bool altCtrlOptional)
    {
        if (!_available) {
            return false;
        }

        // Handle based on input source
        if (_inputSource == InputSource::Keyboard)
        {
            bool toReturn = (Utils::keyDown(_keyCode)) && (_shiftRequired == Utils::shiftPressed());
            if ((_altRequired || _ctrlRequired) || !altCtrlOptional)
            {
                toReturn = toReturn && (Utils::altPressed() == _altRequired) && (Utils::ctrlPressed() == _ctrlRequired);
            }
            return toReturn;
        }
        else if (_inputSource == InputSource::Gamepad)
        {
            // For gamepad, the _keyCode corresponds to a button code defined in Gamepad::button_t
            auto& gamepad = Globals::instance().gamePad();
            if (!gamepad.isConnected()) {
                return false;
            }

            // Check if the main button is pressed
            bool mainButtonPressed = gamepad.isButtonPressed(static_cast<Gamepad::button_t>(_keyCode));
            if (!mainButtonPressed) {
                return false;
            }

            // Check modifiers
            bool rbPressed = gamepad.isButtonPressed(Gamepad::button_t::RB);
            bool lbPressed = gamepad.isButtonPressed(Gamepad::button_t::LB);

            // ALWAYS do exact modifier matching for gamepad inputs
            // This ensures bindings with no modifiers won't activate when modifiers are pressed
            return (rbPressed == _altRequired) && (lbPressed == _ctrlRequired);
        }

        return false;
    }

	void ActionData::setKeyCode(int newKeyCode)
	{
		// if we have a keycode set and this is a different one, we will reset alt/ctrl/shift key requirements as it's a different key altogether.
		if (_keyCode > 0 && newKeyCode > 0 && newKeyCode != _keyCode)
		{
			clear();
		}
		_keyCode = newKeyCode;
	}

	void ActionData::clear()
	{
		_altRequired = false;
		_ctrlRequired = false;
		_shiftRequired = false;
		_keyCode = 0;
	}

	void ActionData::update(int newKeyCode, bool altRequired, bool ctrlRequired, bool shiftRequired, bool isGamepad)
	{
		if (isGamepad)
		{
			_keyCode = newKeyCode;
		}
		else
		{
			_keyCode = static_cast<uint8_t>(newKeyCode);
		}
		
		_altRequired = altRequired;
		_ctrlRequired = ctrlRequired;
		_shiftRequired = shiftRequired;


	}
}