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
#include <comdef.h>

using namespace std;

BOOL isMainWindow(HWND handle)
{
	BOOL toReturn = GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
	if (toReturn)
	{
		// check window title as there can be more top windows. 
		int bufsize = GetWindowTextLength(handle) + 1;
		LPWSTR title = new WCHAR[bufsize];
		toReturn &= GetWindowText(handle, title, bufsize) > 0;
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
	CloseHandle(processHandle);
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


BYTE CharToByte(char c)
{
	BYTE b;
	sscanf_s(&c, "%hhx", &b);
	return b;
}


string formatString(const char *fmt, va_list args)
{
	va_list args_copy;
	va_copy(args_copy, args);

	int len = vsnprintf(NULL, 0, fmt, args_copy);
	char* buffer = new char[len + 2];
	vsnprintf(buffer, len+1, fmt, args_copy);
	string toReturn(buffer, len+1);
	return toReturn;
}


bool stringStartsWith(const char *a, const char *b)
{
	return strncmp(a, b, strlen(b)) == 0 ? 1 : 0;
}
