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
#include "AOBBlock.h"
#include "Utils.h"
#include "MessageHandler.h"
#include <string>
#include <algorithm>

namespace IGCS
{
    AOBBlock::AOBBlock()
        :
        byteStorage(nullptr),
        byteStorage2(nullptr),
        nopState(false),
        nopState2(false),
        _bytePattern(nullptr),
        _patternMask(nullptr),
        _patternSize(0),
        _customOffset(0),
        _occurrence(0),
        _secondaryOccurrence(0),
        _found(false)
    {
    }

    AOBBlock::AOBBlock(std::string blockName, std::string bytePatternAsString, int occurrence)
        : byteStorage(nullptr),
        byteStorage2(nullptr),
        nopState(false),
        nopState2(false),
        _blockName(std::move(blockName)),
        _bytePatternAsString(std::move(bytePatternAsString)),
        _bytePattern(nullptr),
        _patternMask(nullptr),
        _patternSize(0),
        _customOffset(0),
        _occurrence(occurrence),
        _secondaryOccurrence(0),
        _found(false)
    {
    }

    AOBBlock::AOBBlock(std::string blockName, std::string bytePatternAsString, int occurrence,
                       std::string secondaryBytePatternAsString, int secondaryOccurrence)
        : byteStorage(nullptr),
        byteStorage2(nullptr),
        nopState(false),
        nopState2(false),
        _blockName(std::move(blockName)),
        _bytePatternAsString(std::move(bytePatternAsString)),
        _secondaryBytePatternAsString(std::move(secondaryBytePatternAsString)),
        _bytePattern(nullptr),
        _patternMask(nullptr),
        _patternSize(0),
        _customOffset(0),
        _occurrence(occurrence),
        _secondaryOccurrence(secondaryOccurrence),
        _found(false)
    {
    }

    AOBBlock::~AOBBlock()
    {
        if (_bytePattern != nullptr)
        {
            free(_bytePattern);
            _bytePattern = nullptr;
        }

        if (_patternMask != nullptr)
        {
            free(_patternMask);
            _patternMask = nullptr;
        }

        // We don't free byteStorage and byteStorage2 here
        // as they might be managed externally
    }

    bool AOBBlock::scan(LPBYTE imageAddress, DWORD imageSize)
    {
        if (!imageAddress || !imageSize)
        {
            MessageHandler::logError("Invalid image address or size for block '%s'", _blockName.c_str());
            return false;
        }

        // Clear previous scan results
        _locationsInImage.clear();
        _found = false;

        if (_bytePattern != nullptr)
        {
            free(_bytePattern);
            _bytePattern = nullptr;
        }

        if (_patternMask != nullptr)
        {
            free(_patternMask);
            _patternMask = nullptr;
        }

        // Try primary pattern first
        createAOBPatternFromStringPattern(_bytePatternAsString);
        bool result = Utils::findAOBPattern(imageAddress, imageSize, this);

        if (result)
        {
            // Primary pattern found
            MessageHandler::logDebug("Pattern for block '%s' found %zu time(s):", _blockName.c_str(), _locationsInImage.size());
            _found = true;

            // Log occurrences up to the requested one
            size_t logLimit = std::min<size_t>(_locationsInImage.size(), _occurrence);
            for (size_t i = 0; i < logLimit; i++)
            {
                MessageHandler::logDebug("%zu: at address: %p", i + 1, static_cast<void*>(_locationsInImage[i]));
            }
        }
        else if (!_secondaryBytePatternAsString.empty())
        {
            // Try secondary pattern if primary not found
            MessageHandler::logDebug("Primary pattern not found for block '%s'. Attempting secondary pattern.", _blockName.c_str());

            // Reset pattern data before creating secondary pattern
            if (_bytePattern != nullptr)
            {
                free(_bytePattern);
                _bytePattern = nullptr;
            }

            if (_patternMask != nullptr)
            {
                free(_patternMask);
                _patternMask = nullptr;
            }

            // Create and scan for secondary pattern
            createAOBPatternFromStringPattern(_secondaryBytePatternAsString);
            result = Utils::findAOBPattern(imageAddress, imageSize, this);

            if (result)
            {
                MessageHandler::logDebug("Secondary pattern for block '%s' found %zu time(s):", _blockName.c_str(), _locationsInImage.size());
                _found = true;

                // Log occurrences up to the requested one
                size_t logLimit = std::min<size_t>(_locationsInImage.size(), _secondaryOccurrence > 0 ? _secondaryOccurrence : _occurrence);
                for (size_t i = 0; i < logLimit; i++)
                {
                    MessageHandler::logDebug("%zu: at address: %p", i + 1, static_cast<void*>(_locationsInImage[i]));
                }
            }
        }

        if (!result)
        {
            MessageHandler::logError("Can't find pattern for block '%s'! Hook not set.", _blockName.c_str());
        }

        return result;
    }

    LPBYTE AOBBlock::absoluteAddress()
    {
        // Occurrence is 1-based in the API, 0-based in the vector
        return absoluteAddress(_occurrence - 1);
    }

    LPBYTE AOBBlock::absoluteAddress(int number)
    {
        if (number < 0 || static_cast<size_t>(number) >= _locationsInImage.size())
        {
            return nullptr;
        }
        return _locationsInImage[number] + _customOffset;
    }

    void AOBBlock::storeFoundLocation(LPBYTE location)
    {
        if (location)
        {
            _locationsInImage.push_back(location);
        }
    }

    void AOBBlock::createAOBPatternFromStringPattern(std::string pattern)
    {
        if (pattern.empty())
        {
            MessageHandler::logError("Empty pattern provided for block '%s'", _blockName.c_str());
            return;
        }

        // First pass: count the actual bytes in the pattern
        int patternLength = 0;
        bool foundCustomOffset = false;

        for (size_t i = 0; i < pattern.size(); )
        {
            char c = pattern[i];

            if (c == ' ')
            {
                // Skip whitespace
                i++;
                continue;
            }

            if (c == '|')
            {
                // Mark the offset position
                if (!foundCustomOffset)
                {
                    _customOffset = patternLength;
                    foundCustomOffset = true;
                }
                i++;
                continue;
            }

            if (c == '?' && i + 1 < pattern.size() && pattern[i + 1] == '?')
            {
                // Wildcard byte
                patternLength++;
                i += 2;
                continue;
            }

            // Regular hex byte (must have 2 characters)
            if (i + 1 < pattern.size() && isxdigit(c) && isxdigit(pattern[i + 1]))
            {
                patternLength++;
                i += 2;
            }
            else
            {
                // Invalid character, skip it
                i++;
            }
        }

        // Allocate memory for pattern and mask
        _patternSize = patternLength;
        _bytePattern = (LPBYTE)calloc(1, patternLength);
        _patternMask = (char*)calloc(1, patternLength + 1); // +1 for null terminator

        // Second pass: fill the arrays
        int index = 0;
        for (size_t i = 0; i < pattern.size() && index < patternLength; )
        {
            char c = pattern[i];

            if (c == ' ')
            {
                i++;
                continue;
            }

            if (c == '|')
            {
                i++;
                continue;
            }

            if (c == '?' && i + 1 < pattern.size() && pattern[i + 1] == '?')
            {
                // Wildcard byte
                _patternMask[index] = '?';
                _bytePattern[index] = 0; // Value doesn't matter for wildcards
                index++;
                i += 2;
                continue;
            }

            // Regular hex byte
            if (i + 1 < pattern.size() && isxdigit(c) && isxdigit(pattern[i + 1]))
            {
                _patternMask[index] = 'x';
                _bytePattern[index] = (Utils::CharToByte(c) << 4) | Utils::CharToByte(pattern[i + 1]);
                index++;
                i += 2;
            }
            else
            {
                // Invalid character, skip it
                i++;
            }
        }

        // Ensure null termination of pattern mask
        _patternMask[patternLength] = '\0';
    }
}