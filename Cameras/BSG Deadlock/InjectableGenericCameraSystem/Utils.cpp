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
#include "Utils.h"
#include "GameConstants.h"
#include "AOBBlock.h"

using namespace std;

namespace IGCS::Utils
{
	BOOL isMainWindow(HWND handle)
	{
		BOOL toReturn = GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
		if (toReturn)
		{
			// check window title as there can be more top windows. 
			int bufsize = GetWindowTextLength(handle) + 1;
			LPWSTR title = new WCHAR[bufsize];
			GetWindowText(handle, title, bufsize);
			toReturn &= (_wcsicmp(title, TEXT(GAME_WINDOW_TITLE)) == 0);
#ifdef _DEBUG
			wcout << "Window found with title: '" << title << "'" << endl;
#endif
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
			if(!EnumProcessModulesEx(processHandle, &processModule, sizeof(processModule), &cbNeeded, LIST_MODULES_32BIT | LIST_MODULES_64BIT))
			{
				processModule = nullptr;
			}
		}
		MODULEINFO toReturn;
		if (nullptr != processModule)
		{
			if (!GetModuleInformation(processHandle, processModule, &toReturn, sizeof(MODULEINFO)))
			{
				toReturn.lpBaseOfDll = nullptr;
			}
		}
		CloseHandle(processHandle);
		return toReturn;
	}

	
	MODULEINFO getModuleInfoOfDll(LPCWSTR libraryName)
	{
		HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());
		HMODULE dllModule = GetModuleHandle(libraryName);
		MODULEINFO toReturn;
		if (nullptr != dllModule)
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


	BYTE CharToByte(char c)
	{
		BYTE b;
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


	LPBYTE findAOBPattern(LPBYTE imageAddress, DWORD imageSize, AOBBlock* const toScanFor)
	{
		BYTE firstByte = *(toScanFor->bytePattern());
		__int64 length = (__int64)imageAddress + imageSize - toScanFor->patternSize();

		LPBYTE toReturn = nullptr;
		LPBYTE startOfScan = imageAddress;
		for (int occurrence = 0; occurrence < toScanFor->occurrence(); occurrence++)
		{
			// reset the pointer found, as we're not interested in this occurrence, we need a following occurrence.
			toReturn = nullptr;
			for (__int64 i = (__int64)startOfScan; i < length; i += 4)
			{
				unsigned x = *(unsigned*)(i);

				if ((x & 0xFF) == firstByte)
				{
					if (DataCompare(reinterpret_cast<BYTE*>(i), toScanFor->bytePattern(), toScanFor->patternMask()))
					{
						toReturn = reinterpret_cast<BYTE*>(i);
						break;
					}
				}
				if ((x & 0xFF00) >> 8 == firstByte)
				{
					if (DataCompare(reinterpret_cast<BYTE*>(i + 1), toScanFor->bytePattern(), toScanFor->patternMask()))
					{
						toReturn = reinterpret_cast<BYTE*>(i + 1);
						break;
					}
				}
				if ((x & 0xFF0000) >> 16 == firstByte)
				{
					if (DataCompare(reinterpret_cast<BYTE*>(i + 2), toScanFor->bytePattern(), toScanFor->patternMask()))
					{
						toReturn = reinterpret_cast<BYTE*>(i + 2);
						break;
					}
				}
				if ((x & 0xFF000000) >> 24 == firstByte)
				{
					if (DataCompare(reinterpret_cast<BYTE*>(i + 3), toScanFor->bytePattern(), toScanFor->patternMask()))
					{
						toReturn = reinterpret_cast<BYTE*>(i + 3);
						break;
					}
				}
			}
			if (nullptr == toReturn)
			{
				// not found, give up
				return toReturn;
			}
			startOfScan = toReturn + 1;	// otherwise we'll match ourselves. 
		}
		return toReturn;
	}


	// locationData is the AOB block with the address of the rip relative value to read for the calculation.
	// nextOpCodeOffset is used to calculate the address of the next instruction as that's the address the rip relative value is relative off. In general
	// this is 4 (the size of the int32 for the rip relative value), but sometimes the rip relative value is inside an instruction following one or more bytes before the 
	// next instruction starts.
	LPBYTE calculateAbsoluteAddress(AOBBlock* locationData, int nextOpCodeOffset)
	{
		// ripRelativeValueAddress is the absolute address of a DWORD value which is a RIP relative offset. A calculation has to be performed on x64 to
		// calculate from that rip relative value and its location the real absolute address it refers to. That address is returned.
		LPBYTE ripRelativeValueAddress = locationData->locationInImage() + locationData->customOffset();
		return  ripRelativeValueAddress + nextOpCodeOffset + *((__int32*)ripRelativeValueAddress);
	}
}