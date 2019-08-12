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
#include "AOBBlock.h"
#include "Utils.h"
#include "OverlayConsole.h"
#include "ScanPattern.h"

using namespace std;

namespace IGCS
{
	AOBBlock::AOBBlock(string blockName, string bytePatternAsString, int occurrence)
		: _blockName{ blockName }, _customOffset{ 0 }, _locationInImage{ nullptr }, _found{ false },
		_isNonCritical{ false }, _patternIndexThatMatched { 0 }, byteStorage{ nullptr }, nopState{ false }
	{
		addAlternative(bytePatternAsString, occurrence);
	}


	AOBBlock::~AOBBlock()
	{
	}


	// Adds an alternative AOB pattern + occurrence. Will be used if a previous pattern failed. 
	void AOBBlock::addAlternative(std::string bytePatternAsString, int occurrence)
	{
		ScanPattern toAdd = ScanPattern(bytePatternAsString, occurrence);
		_scanPatterns.push_back(toAdd);
	}


	bool AOBBlock::scan(LPBYTE imageAddress, DWORD imageSize)
	{
		_patternIndexThatMatched = -1;
		bool toReturn = _isNonCritical;		// by default this is false, so we'll return false by default if something fails, otherwise we silently 'succeed'. 
		LPBYTE aobPatternLocation = nullptr;
		int patternIndex = -1;
		for (auto& scanPattern : _scanPatterns)
		{
			patternIndex++;
			aobPatternLocation = Utils::findAOBPattern(imageAddress, imageSize, scanPattern);
			if (nullptr == aobPatternLocation)
			{
				// not found, try next
				continue;
			}
			// found
			_patternIndexThatMatched = patternIndex;
			_customOffset = scanPattern.customOffset();
			break;
		}
		if (nullptr == aobPatternLocation)
		{
			OverlayConsole::instance().logError("Can't find pattern for block '%s'! Hook not set.", _blockName.c_str());
			return toReturn;
		}
		else
		{
			OverlayConsole::instance().logDebug("Pattern for block '%s' found at address: %p", _blockName.c_str(), (void*)aobPatternLocation);
			_found = true;
		}
		_locationInImage = aobPatternLocation;
		return true;
	}
}