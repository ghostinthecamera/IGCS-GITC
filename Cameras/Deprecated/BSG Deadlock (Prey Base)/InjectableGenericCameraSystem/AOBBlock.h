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
#pragma once
#include "AOBBlock.h"
#include "Utils.h"

using namespace std;

namespace IGCS
{
	class AOBBlock
	{
	public:
		AOBBlock(string blockName, string bytePatternAsString, int occurrence);
		~AOBBlock();

		bool scan(LPBYTE imageAddress, DWORD imageSize);
		LPBYTE locationInImage() { return _locationInImage; }
		LPBYTE bytePattern() { return _bytePattern; }
		int occurrence() { return _occurrence; }
		int patternSize() { return _patternSize; }
		char* patternMask() { return _patternMask; }
		int customOffset() { return _customOffset; }

	private:
		void createAOBPatternFromStringPattern(string pattern);

		string _blockName;
		string _bytePatternAsString;
		LPBYTE _bytePattern;
		char* _patternMask;
		int _patternSize;
		int _customOffset;
		int _occurrence;		// starts at 1: if there are more occurrences, and e.g. the 3rd has to be picked, set this to 3.
		LPBYTE _locationInImage;	// the location to use after the scan has been completed.
	};

}