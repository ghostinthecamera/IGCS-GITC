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
#include "Utils.h"

using namespace std;

namespace IGCS
{
	ActionData::ActionData(string name, string description, int keyCode, bool altRequired, bool ctrlRequired, bool shiftRequired, bool available)
		: _name{ name }, _description{ description }, _keyCode{ keyCode }, _altRequired{ altRequired }, _ctrlRequired{ ctrlRequired }, _shiftRequired{ shiftRequired },
		  _available {available}
	{
		fillStringVariant();
	}
	ActionData::~ActionData() {}

	// altCtrlOptional is only effective for actions which don't have alt/ctrl as a required key. Actions which do have one or more of these
	// keys as required, will ignore altCtrlOptional and always test for these keys. 
	bool ActionData::isActive(bool altCtrlOptional)
	{
		bool toReturn = (_available && Utils::keyDown(_keyCode)) && (_shiftRequired==shiftPressed());
		if((_altRequired || _ctrlRequired) || !altCtrlOptional)
		{
			toReturn = toReturn && (Utils::altPressed() == _altRequired) && (ctrlPressed() == _ctrlRequired);
		}
		return toReturn;
	}

	void ActionData::setKeyCode(int newKeyCode)
	{
		// if we have a keycode set and this is a different one, we will reset alt/ctrl/shift key requirements as it's a different key altogether.
		if (_keyCode > 0 && newKeyCode > 0 && newKeyCode != _keyCode)
		{
			clear();
		}
		_keyCode = newKeyCode;
		fillStringVariant();
	}

	void ActionData::clear()
	{
		_altRequired = false;
		_ctrlRequired = false;
		_shiftRequired = false;
		_keyCode = 0;
		fillStringVariant();
	}

	void ActionData::update(ActionData& source)
	{
		if (!source.isValid())
		{
			return;
		}
		_altRequired = source._altRequired;
		_ctrlRequired = source._ctrlRequired;
		_shiftRequired = source._shiftRequired;
		_keyCode = source._keyCode;
		fillStringVariant();
	}

	bool ActionData::ctrlPressed()
	{
		return Utils::keyDown(VK_RCONTROL) || Utils::keyDown(VK_LCONTROL);
	}
	
	bool ActionData::shiftPressed()
	{
		return Utils::keyDown(VK_LSHIFT) || Utils::keyDown(VK_RSHIFT);
	}

	// Returns 4 bytes: first byte is the keycode, second byte is alt required, third byte is cntrl required, forth byte is shift required
	int ActionData::getIniFileValue()
	{
		return (_keyCode & 0xFF) << 24 | ((_altRequired ? 1 : 0) << 16) | ((_ctrlRequired ? 1 : 0) << 8) | ((_shiftRequired ? 1 : 0));
	}

	// Receives 4 bytes: first byte is the keycode, second byte is alt required, third byte is cntrl required, forth byte is shift required
	void ActionData::setValuesFromIniFileValue(int iniFileValue)
	{
		_keyCode = ((iniFileValue >> 24) & 0xFF);
		_altRequired = ((iniFileValue >> 16) & 0xFF) == 0x01;
		_ctrlRequired = ((iniFileValue >> 8) & 0xFF) == 0x01;
		_shiftRequired = (iniFileValue & 0xFF) == 0x01;
		fillStringVariant();
	}
	
	void ActionData::fillStringVariant()
	{
		if (!_altRequired && !_ctrlRequired && !_shiftRequired && (_keyCode <= 0))
		{
			// empty
			_stringVariant = "Press a key";
			return;
		}
		_stringVariant.clear();
		if (_altRequired)
		{
			_stringVariant.append("Alt + ");
		}
		if (_ctrlRequired)
		{
			_stringVariant.append("Ctrl + ");
		}
		if (_shiftRequired)
		{
			_stringVariant.append("Shift + ");
		}
		if (_keyCode > 0)
		{
			_stringVariant.append(Utils::vkCodeToString(_keyCode));
		}
	}
}