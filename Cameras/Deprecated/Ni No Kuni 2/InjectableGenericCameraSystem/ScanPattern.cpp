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
#include "ScanPattern.h"

namespace IGCS
{
	ScanPattern::ScanPattern(std::string bytePatternAsString, int occurrence) : 
		_bytePatternAsString{ bytePatternAsString }, _occurrence{ occurrence }, _bytePattern{ nullptr }, _patternMask{ nullptr }, _patternSize{-1}, 
		_customOffset{ 0 }
	{
		createAOBPatternFromStringPattern();
	}


	ScanPattern::~ScanPattern()
	{
	}


	// Updates this pattern with the data used with an aob scan. This pattern contains a string in the form of "aa bb ??" where '??' is a byte
	// which has to be skipped in the comparison, and 'aa' and 'bb' are hexadecimal bytes which have to have that value at that position.
	// If a '|' is specified in the pattern, the position of the byte following it is the start offset returned by the aob scanner, instead of
	// the position of the first byte of the pattern. 
	void ScanPattern::createAOBPatternFromStringPattern()
	{
		if (_bytePattern != nullptr)
		{
			// already initialized
			return;
		}
		int index = 0;
		char* pChar = (char*)_bytePatternAsString.c_str();
		_patternSize = static_cast<int>(_bytePatternAsString.size());
		_bytePattern = (LPBYTE)calloc(_patternSize, sizeof(BYTE));
		_patternMask = (char*)calloc((__int64)_patternSize + 2, sizeof(char));
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
			_bytePattern[index++] = (CharToByte(pChar[0]) << 4) + CharToByte(pChar[1]);
			pChar += 2;
		}
	}
	

	BYTE ScanPattern::CharToByte(char c)
	{
		BYTE b;
		sscanf_s(&c, "%hhx", &b);
		return b;
	}
}