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
#pragma once

#include "Utils.h"
#include <string>
#include <vector>

namespace IGCS
{
    class AOBBlock
    {
    public:
        AOBBlock();
        AOBBlock(std::string blockName, std::string bytePatternAsString, int occurrence);
        AOBBlock(std::string blockName, std::string bytePatternAsString, int occurrence,
                 std::string secondaryBytePatternAsString, int secondaryOccurrence = 0);
        ~AOBBlock();

        bool scan(LPBYTE imageAddress, DWORD imageSize);
        LPBYTE locationInImage() { return _locationsInImage.size() > 0 ? _locationsInImage[0] : nullptr; }
        LPBYTE bytePattern() { return _bytePattern; }
        int occurrence() { return _occurrence; }
        int patternSize() { return _patternSize; }
        char* patternMask() { return _patternMask; }
        int customOffset() { return _customOffset; }
        std::string getName() { return _blockName; }
        bool isFound() { return _found; }
        LPBYTE absoluteAddress();
        LPBYTE absoluteAddress(int number);
        void storeFoundLocation(LPBYTE location);
        uint8_t* byteStorage;
        uint8_t* byteStorage2;
        bool nopState;
        bool nopState2;

    private:
        void createAOBPatternFromStringPattern(std::string pattern);

        std::string _blockName;
        std::string _bytePatternAsString;
        std::string _secondaryBytePatternAsString;
        LPBYTE _bytePattern;
        char* _patternMask;
        int _patternSize;
        int _customOffset;
        int _occurrence;        // starts at 1: if there are more occurrences, and e.g. the 3rd has to be picked, set this to 3.
        int _secondaryOccurrence;
        std::vector<LPBYTE> _locationsInImage; // the locations to use after the scan has been completed.
        bool _found;
    };
}