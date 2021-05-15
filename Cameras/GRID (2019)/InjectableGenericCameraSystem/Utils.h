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
#pragma once
#include "stdafx.h"
#include <filesystem>

namespace IGCS
{
	// forward declaration to avoid cyclic dependency.
	class AOBBlock;
}

namespace IGCS::Utils
{
	struct handle_data {
		unsigned long process_id;
		HWND best_handle;
	};

	template <typename T>
	T clamp(T value, T min, T max, T defaultValue)
	{
		return value < min ? defaultValue
			: value > max ? defaultValue : value;
	}

	template <typename T>
	T clamp(T value, T min, T defaultValue)
	{
		return value < min ? defaultValue : value;
	}


	HWND findMainWindow(unsigned long process_id);
	MODULEINFO getModuleInfoOfContainingProcess();
	MODULEINFO getModuleInfoOfDll(LPCWSTR libraryName);
	bool findAOBPattern(LPBYTE imageAddress, DWORD imageSize, AOBBlock* const toScanFor);
	uint8_t CharToByte(char c);
	LPBYTE calculateAbsoluteAddress(AOBBlock* locationData, int nextOpCodeOffset);
	std::string formatString(const char* fmt, ...);
	std::string formatStringVa(const char* fmt, va_list args);
	bool stringStartsWith(const char *a, const char *b);
	bool keyDown(int virtualKeyCode);
	bool altPressed();
	bool ctrlPressed();
	bool shiftPressed();
	std::string vkCodeToString(int vkCode);
	float floatFromBytes(uint8_t byteArray[], DWORD arrayLength, int startIndex);
	int intFromBytes(uint8_t byteArray[], DWORD arrayLength, int startIndex);
	std::string stringFromBytes(uint8_t byteArray[], DWORD arrayLength, int startIndex);
	std::filesystem::path obtainHostExeAndPath();
}