////////////////////////////////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System
// Copyright(c) 2017, Frans Bouma
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
#include "AOBBlock.h"
#include "Utils.h"
#include "OverlayConsole.h"

namespace IGCS
{
	AOBBlock::AOBBlock(string blockName, string bytePatternAsString, int occurrence)
									: _blockName{ blockName }, _bytePatternAsString{ bytePatternAsString }, _customOffset{ 0 }, _occurrence{ occurrence },
									  _bytePattern{ nullptr }, _patternMask{ nullptr }, _locationInImage{ nullptr }
	{
	}


	AOBBlock::~AOBBlock()
	{
	}


	bool AOBBlock::scan(LPBYTE imageAddress, DWORD imageSize)
	{
		createAOBPatternFromStringPattern(_bytePatternAsString);
		LPBYTE aobPatternLocation = Utils::findAOBPattern(imageAddress, imageSize, this);
		if (nullptr == aobPatternLocation)
		{
			OverlayConsole::instance().logError("Can't find pattern for block '%s'! Hook not set.", _blockName.c_str());
			return false;
		}
		else
		{
			OverlayConsole::instance().logDebug("Pattern for block '%s' found at address: %p", _blockName.c_str(), (void*)aobPatternLocation);
		}
		_locationInImage = aobPatternLocation;
		return true;
	}
	

	// Creates an aob_pattern struct which is usable with an aob scan. The pattern given is in the form of "aa bb ??" where '??' is a byte
	// which has to be skipped in the comparison, and 'aa' and 'bb' are hexadecimal bytes which have to have that value at that position.
	// If a '|' is specified in the pattern, the position of the byte following it is the start offset returned by the aob scanner, instead of
	// the position of the first byte of the pattern. 
	void AOBBlock::createAOBPatternFromStringPattern(string pattern)
	{
		int index = 0;
		char* pChar = &pattern[0];

		_patternSize = static_cast<int>(pattern.size());
		_bytePattern = (LPBYTE)calloc(0x1, _patternSize);
		_patternMask = (char*)calloc(0x1, _patternSize);
		_customOffset = 0;

		while (*pChar)
		{
			if (*pChar == ' ')
			{
				pChar++;
				continue;
			}

			if (*pChar == '|')
			{
				pChar++;
				_customOffset = index;
				continue;
			}

			if (*pChar == '?')
			{
				_patternMask[index++] += '?';
				pChar += 2;
				continue;
			}

			_patternMask[index] = 'x';
			_bytePattern[index++] = (Utils::CharToByte(pChar[0]) << 4) + Utils::CharToByte(pChar[1]);
			pChar += 2;
		}
	}

}