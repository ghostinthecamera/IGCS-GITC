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
#include "Console.h"
#include "Globals.h"
#include "System.h"
#include "Utils.h"

using namespace std;
using namespace IGCS;

DWORD WINAPI MainThread(LPVOID lpParam);

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
			Globals::instance().systemActive(false);
			break;
	}
	return TRUE;
}


// lpParam gets the hModule value of the DllMain process
DWORD WINAPI MainThread(LPVOID lpParam)
{
	MODULEINFO hostModuleInfo = Utils::getModuleInfoOfContainingProcess();
	if (nullptr == hostModuleInfo.lpBaseOfDll)
	{
		Console::WriteError("Not able to obtain parent process base address... exiting");
	}
	else
	{
		System s;
		s.start((LPBYTE)hostModuleInfo.lpBaseOfDll, hostModuleInfo.SizeOfImage);
	}
	Console::Release();
	return 0;
}
