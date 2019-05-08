////////////////////////////////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System
// Copyright(c) 2016, Frans Bouma
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

using namespace std;

DWORD WINAPI MainThread(LPVOID lpParam);
MODULEINFO getModuleInfoOfDll(LPCWSTR libraryName);

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  reason, LPVOID lpReserved)
{
	DWORD threadID;
	HANDLE threadHandle;

	DisableThreadLibraryCalls(hModule);

	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		threadHandle = CreateThread(nullptr, 0, MainThread, hModule, 0, &threadID);
		SetThreadPriority(threadHandle, THREAD_PRIORITY_ABOVE_NORMAL);
		break;
	case DLL_PROCESS_DETACH:
		g_systemActive = false;
		break;
	}
	return TRUE;
}


// lpParam gets the hModule value of the DllMain process
DWORD WINAPI MainThread(LPVOID lpParam)
{
	Console c;
	c.Init();
	c.WriteHeader();

	MODULEINFO hostModuleInfo = getModuleInfoOfDll(L"UnityPlayer.dll");
	if(NULL == hostModuleInfo.lpBaseOfDll)
	{
		c.WriteError("Not able to obtain parent process base address... exiting");
	}
	else
	{
		SystemStart((HMODULE)hostModuleInfo.lpBaseOfDll, c);
	}
	c.Release();
	return 0;
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
