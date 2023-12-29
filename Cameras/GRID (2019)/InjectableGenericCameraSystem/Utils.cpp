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
#include "Utils.h"
#include "GameConstants.h"
#include "AOBBlock.h"
#include <comdef.h>
#include <codecvt>
#include <filesystem>
#include "MessageHandler.h"

using namespace std;

namespace IGCS::Utils
{
	// This table is from Reshade.
	static const char vkCodeToStringLookup[256][16] =
	{
		"", "", "", "Cancel", "", "", "", "", "Backspace", "Tab", "", "", "Clear", "Enter", "", "",
		"Shift", "Control", "Alt", "Pause", "Caps Lock", "", "", "", "", "", "", "Escape", "", "", "", "",
		"Space", "Page Up", "Page Down", "End", "Home", "Left Arrow", "Up Arrow", "Right Arrow", "Down Arrow",
		"Select", "", "", "Print Screen", "Insert", "Delete", "Help",
		"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "", "", "", "", "", "",
		"", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
		"P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "Left Windows", "Right Windows", "", "", "Sleep",
		"Numpad 0", "Numpad 1", "Numpad 2", "Numpad 3", "Numpad 4", "Numpad 5", "Numpad 6", "Numpad 7", "Numpad 8", "Numpad 9",
		"Numpad *", "Numpad +", "", "Numpad -", "Numpad Decimal", "Numpad /",
		"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "F13", "F14", "F15", "F16",
		"F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24", "", "", "", "", "", "", "", "",
		"Num Lock", "Scroll Lock",
	};

	// Obtains the exe's filename + path and returns that as a path object.
	std::filesystem::path obtainHostExeAndPath()
	{
		char lpBuffer[MAX_PATH];
		GetModuleFileNameA(NULL, lpBuffer, MAX_PATH);
		return filesystem::path(lpBuffer);
	}
	

	BOOL isMainWindow(HWND handle)
	{
		BOOL toReturn = GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
		if (toReturn)
		{
			// check window title as there can be more top windows. 
			int bufsize = GetWindowTextLength(handle) + 1;
			LPWSTR title = new WCHAR[bufsize];
			GetWindowText(handle, title, bufsize);
			// trick to do a 'startswith' compare. Will only return 0 if the string actually starts with the fragment we want to compare with
			toReturn &= (wcsncmp(title, TEXT(GAME_WINDOW_TITLE), strlen(GAME_WINDOW_TITLE)) == 0);
			// convert title to char* so we can display it
			_bstr_t titleAsBstr(title);
			const char* titleAsChar = titleAsBstr;		// char conversion copy
			MessageHandler::logDebug("Window found with title: '%s'", titleAsChar);
		}
		return toReturn;
	}


	BOOL CALLBACK enumWindowsCallback(HWND handle, LPARAM lParam)
	{
		handle_data& data = *(handle_data*)lParam;
		unsigned long process_id = 0;
		GetWindowThreadProcessId(handle, &process_id);
		if (data.process_id != process_id || !isMainWindow(handle))
		{
			return TRUE;
		}
		data.best_handle = handle;
		return FALSE;
	}


	MODULEINFO getModuleInfoOfContainingProcess()
	{
		HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());
		HMODULE processModule = nullptr;
		if (nullptr != processHandle)
		{
			DWORD cbNeeded;
			if (!EnumProcessModulesEx(processHandle, &processModule, sizeof(processModule), &cbNeeded, LIST_MODULES_32BIT | LIST_MODULES_64BIT))
			{
				processModule = nullptr;
			}
		}
		MODULEINFO toReturn;
		if (nullptr == processModule)
		{
			toReturn.lpBaseOfDll = nullptr;
		}
		else
		{
			if (!GetModuleInformation(processHandle, processModule, &toReturn, sizeof(MODULEINFO)))
			{
				toReturn.lpBaseOfDll = nullptr;
			}
		}
		if (processHandle != nullptr)
		{
			CloseHandle(processHandle);
		}
		return toReturn;
	}


	MODULEINFO getModuleInfoOfDll(LPCWSTR libraryName)
	{
		HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());
		HMODULE dllModule = GetModuleHandle(libraryName);
		MODULEINFO toReturn;
		if (nullptr == dllModule)
		{
			toReturn.lpBaseOfDll = nullptr;
		}
		else
		{
			if (!GetModuleInformation(processHandle, dllModule, &toReturn, sizeof(MODULEINFO)))
			{
				toReturn.lpBaseOfDll = nullptr;
			}
		}
		CloseHandle(processHandle);
		return toReturn;
	}


	HWND findMainWindow(unsigned long process_id)
	{
		handle_data data;
		data.process_id = process_id;
		data.best_handle = 0;
		EnumWindows(enumWindowsCallback, (LPARAM)&data);
		return data.best_handle;
	}


	uint8_t CharToByte(char c)
	{
		uint8_t b;
		sscanf_s(&c, "%hhx", &b);
		return b;
	}


	bool DataCompare(LPBYTE image, LPBYTE bytePattern, char* patternMask)
	{
		for (; *patternMask; ++patternMask, ++image, ++bytePattern)
		{
			if (*patternMask == 'x' && *image != *bytePattern)
			{
				return false;
			}
		}
		return (*patternMask) == 0;
	}


	// returns false if not found, true otherwise
	bool findAOBPattern(LPBYTE imageAddress, DWORD imageSize, AOBBlock* const toScanFor)
	{
		uint8_t firstByte = *(toScanFor->bytePattern());
		__int64 length = (__int64)imageAddress + imageSize - toScanFor->patternSize();

		LPBYTE startOfScan = imageAddress;
		for (int occurrence = 0; occurrence < toScanFor->occurrence(); occurrence++)
		{
			// reset the pointer found, as we're not interested in this occurrence, we need a following occurrence.
			LPBYTE currentAddress = nullptr;
			for (__int64 i = (__int64)startOfScan; i < length; i += 4)
			{
				unsigned x = *(unsigned*)(i);

				if ((x & 0xFF) == firstByte)
				{
					if (DataCompare(reinterpret_cast<uint8_t*>(i), toScanFor->bytePattern(), toScanFor->patternMask()))
					{
						currentAddress = reinterpret_cast<uint8_t*>(i);
						break;
					}
				}
				if ((x & 0xFF00) >> 8 == firstByte)
				{
					if (DataCompare(reinterpret_cast<uint8_t*>(i + 1), toScanFor->bytePattern(), toScanFor->patternMask()))
					{
						currentAddress = reinterpret_cast<uint8_t*>(i + 1);
						break;
					}
				}
				if ((x & 0xFF0000) >> 16 == firstByte)
				{
					if (DataCompare(reinterpret_cast<uint8_t*>(i + 2), toScanFor->bytePattern(), toScanFor->patternMask()))
					{
						currentAddress = reinterpret_cast<uint8_t*>(i + 2);
						break;
					}
				}
				if ((x & 0xFF000000) >> 24 == firstByte)
				{
					if (DataCompare(reinterpret_cast<uint8_t*>(i + 3), toScanFor->bytePattern(), toScanFor->patternMask()))
					{
						currentAddress = reinterpret_cast<uint8_t*>(i + 3);
						break;
					}
				}
			}
			if (nullptr == currentAddress)
			{
				// not found, give up
				return false;
			}
			// found an occurrence, store it
			toScanFor->storeFoundLocation(currentAddress);
			startOfScan = currentAddress + 1;	// otherwise we'll match ourselves. 
		}
		return true;
	}


	// locationData is the AOB block with the address of the rip relative value to read for the calculation.
	// nextOpCodeOffset is used to calculate the address of the next instruction as that's the address the rip relative value is relative off. In general
	// this is 4 (the size of the int32 for the rip relative value), but sometimes the rip relative value is inside an instruction following one or more bytes before the 
	// next instruction starts.
	LPBYTE calculateAbsoluteAddress(AOBBlock* locationData, int nextOpCodeOffset)
	{
		assert(locationData != nullptr);
		// ripRelativeValueAddress is the absolute address of a DWORD value which is a RIP relative offset. A calculation has to be performed on x64 to
		// calculate from that rip relative value and its location the real absolute address it refers to. That address is returned.
		LPBYTE ripRelativeValueAddress = locationData->locationInImage() + locationData->customOffset();
		return  ripRelativeValueAddress + nextOpCodeOffset + *((__int32*)ripRelativeValueAddress);
	}

	
	string formatString(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		string formattedArgs = formatStringVa(fmt, args);
		va_end(args);
		return formattedArgs;
	}


	string formatStringVa(const char* fmt, va_list args)
	{
		va_list args_copy;
		va_copy(args_copy, args);

		int len = vsnprintf(NULL, 0, fmt, args_copy);
		char* buffer = new char[len + 2];
		vsnprintf(buffer, len + 1, fmt, args_copy);
		string toReturn(buffer, len + 1);
		return toReturn;
	}


	bool stringStartsWith(const char *a, const char *b)
	{
		return strncmp(a, b, strlen(b)) == 0 ? 1 : 0;
	}

	
	bool keyDown(int virtualKeyCode)
	{
		return (GetKeyState(virtualKeyCode) & 0x8000);
	}

	
	bool altPressed()
	{
		return keyDown(VK_LMENU) || keyDown(VK_RMENU);
	}

	
	bool ctrlPressed()
	{
		return keyDown(VK_LCONTROL) || keyDown(VK_RCONTROL);
	}

	
	bool shiftPressed()
	{
		return keyDown(VK_LSHIFT) || keyDown(VK_RSHIFT);
	}

	
	std::string vkCodeToString(int vkCode)
	{
		if (vkCode > 255 || vkCode < 0)
		{
			return "";
		}
		string toReturn = vkCodeToStringLookup[vkCode];
		return toReturn;
	}

	
	float floatFromBytes(uint8_t byteArray[], DWORD arrayLength, int startIndex)
	{
		if(arrayLength<startIndex+4)
		{
			return -1.0f;
		}
		float* floatInArray = reinterpret_cast<float*>(byteArray + startIndex);
		return *floatInArray;
	}

	
	int intFromBytes(uint8_t byteArray[], DWORD arrayLength, int startIndex)
	{
		if (arrayLength < startIndex + 4)
		{
			return -1;
		}
		int* intInArray = reinterpret_cast<int*>(byteArray + startIndex);
		return *intInArray;
	}

	
	std::string stringFromBytes(uint8_t byteArray[], DWORD arrayLength, int startIndex)
	{
		if (arrayLength < startIndex + 4)
		{
			return "";
		}
		char* charInArray = reinterpret_cast<char*>(byteArray + startIndex);
		// copy over the bytes in our own array which has a trailing 0
		char* characters = new char[(arrayLength - startIndex) + 1];
		memcpy(characters, charInArray, arrayLength - startIndex);
		characters[(arrayLength - startIndex)] = '\0';
		string toReturn(characters);
		return toReturn;
	}
}