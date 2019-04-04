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
#include "MinHook.h"
#include "Gamepad.h"
#include "Globals.h"
#include "input.h"

using namespace std;

namespace IGCS::InputHooker
{
	//--------------------------------------------------------------------------------------------------------------------------------
	// Forward declarations
	void processMessage(LPMSG lpMsg, bool removeIfRequired);

	//--------------------------------------------------------------------------------------------------------------------------------
	// Typedefs of functions to hook
	typedef DWORD(WINAPI *XINPUTGETSTATE)(DWORD, XINPUT_STATE*);
	typedef BOOL(WINAPI *GETMESSAGEA)(LPMSG, HWND, UINT, UINT);
	typedef BOOL(WINAPI *GETMESSAGEW)(LPMSG, HWND, UINT, UINT);
	typedef BOOL(WINAPI *PEEKMESSAGEA)(LPMSG, HWND, UINT, UINT, UINT);
	typedef BOOL(WINAPI *PEEKMESSAGEW)(LPMSG, HWND, UINT, UINT, UINT);

	//--------------------------------------------------------------------------------------------------------------------------------
	// Pointers to the original hooked functions
	static XINPUTGETSTATE hookedXInputGetState = nullptr;
	static GETMESSAGEA hookedGetMessageA = nullptr;
	static GETMESSAGEW hookedGetMessageW = nullptr;
	static PEEKMESSAGEA hookedPeekMessageA = nullptr;
	static PEEKMESSAGEW hookedPeekMessageW = nullptr;

	//--------------------------------------------------------------------------------------------------------------------------------
	// Implementations

	// wrapper for easier setting up hooks for MinHook
	template <typename T>
	inline MH_STATUS MH_CreateHookEx(LPVOID pTarget, LPVOID pDetour, T** ppOriginal)
	{
		return MH_CreateHook(pTarget, pDetour, reinterpret_cast<LPVOID*>(ppOriginal));
	}

	template <typename T>
	inline MH_STATUS MH_CreateHookApiEx(
		LPCWSTR pszModule, LPCSTR pszProcName, LPVOID pDetour, T** ppOriginal)
	{
		return MH_CreateHookApi(
			pszModule, pszProcName, pDetour, reinterpret_cast<LPVOID*>(ppOriginal));
	}


	// Our own version of XInputGetState
	DWORD WINAPI detourXInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState)
	{
		// first call the original function
		DWORD toReturn = hookedXInputGetState(dwUserIndex, pState);
		// check if the passed in pState is equal to our gamestate. If so, always allow.
		if (g_cameraEnabled && pState != Globals::instance().gamePad().getState())
		{
			// check if input is blocked. If so, zero the state, so the host will see no input data
			if (Globals::instance().inputBlocked())
			{
				ZeroMemory(pState, sizeof(XINPUT_STATE));
			}
		}
		return toReturn;
	}


	// Our own version of GetMessageA
	BOOL WINAPI detourGetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
	{
		// first call the original function
		if (!hookedGetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax))
		{
			return FALSE;
		}
		processMessage(lpMsg, true);
		return TRUE;
	}


	// Our own version of GetMessageW
	BOOL WINAPI detourGetMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
	{
		// first call the original function
		if (!hookedGetMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax))
		{
			return FALSE;
		}
		processMessage(lpMsg, true);
		return TRUE;
	}


	// Our own version of PeekMessageA
	BOOL WINAPI detourPeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
	{
		// first call the original function
		if (!hookedPeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg))
		{
			return FALSE;
		}
		processMessage(lpMsg, wRemoveMsg & PM_REMOVE);
		return TRUE;
	}

	// Our own version of PeekMessageW
	BOOL WINAPI detourPeekMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
	{
		// first call the original function
		if (!hookedPeekMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg))
		{
			return FALSE;
		}
		processMessage(lpMsg, wRemoveMsg & PM_REMOVE);
		return TRUE;
	}


	void processMessage(LPMSG lpMsg, bool removeIfRequired)
	{
		if (lpMsg->hwnd != nullptr /* && removeIfRequired */ && Input::handleMessage(lpMsg))
		{
			// message was handled by our code. This means it's a message we want to block if input blocking is enabled. 
			if (Globals::instance().inputBlocked())
			{
				lpMsg->message = WM_NULL;	// reset to WM_NULL so the host will receive a dummy message instead.
			}
		}
	}


	// Sets the input hooks for the various input related functions we defined own wrapper functions for. After a successful hook setup
	// they're enabled. 
	void setInputHooks()
	{
		MH_Initialize();
		if (MH_CreateHookApiEx(L"xinput9_1_0", "XInputGetState", &detourXInputGetState, &hookedXInputGetState) != MH_OK)
		{
			Console::WriteError("Hooking XInput9_1_0 failed!");
		}
#ifdef _DEBUG
		Console::WriteLine("Hook set to XInputSetState");
#endif

		if (MH_CreateHookApiEx(L"user32", "GetMessageA", &detourGetMessageA, &hookedGetMessageA) != MH_OK)
		{
			Console::WriteError("Hooking GetMessageA failed!");
		}
#ifdef _DEBUG
		Console::WriteLine("Hook set to GetMessageA");
#endif
		if (MH_CreateHookApiEx(L"user32", "GetMessageW", &detourGetMessageW, &hookedGetMessageW) != MH_OK)
		{
			Console::WriteError("Hooking GetMessageW failed!");
		}
#ifdef _DEBUG
		Console::WriteLine("Hook set to GetMessageW");
#endif
		if (MH_CreateHookApiEx(L"user32", "PeekMessageA", &detourPeekMessageA, &hookedPeekMessageA) != MH_OK)
		{
			Console::WriteError("Hooking PeekMessageA failed!");
		}
#ifdef _DEBUG
		Console::WriteLine("Hook set to PeekMessageA");
#endif
		if (MH_CreateHookApiEx(L"user32", "PeekMessageW", &detourPeekMessageW, &hookedPeekMessageW) != MH_OK)
		{
			Console::WriteError("Hooking PeekMessageW failed!");
		}
#ifdef _DEBUG
		Console::WriteLine("Hook set to PeekMessageW");
#endif

		// Enable all hooks
		if (MH_EnableHook(MH_ALL_HOOKS) == MH_OK)
		{
			Console::WriteLine("All hooks enabled.");
		}
		else
		{
			Console::WriteError("Enabling hooks failed.");
		}
	}
}