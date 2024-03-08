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
#include "MinHook.h"
#include "Gamepad.h"
#include "Globals.h"
#include "input.h"
#include "MessageHandler.h"

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

	//-----------------------------------------------
	// statics
	static CRITICAL_SECTION _messageProcessCriticalSection;

	//--------------------------------------------------------------------------------------------------------------------------------
	// Implementations

	// wrapper for easier setting up hooks for MinHook
	template <typename T>
	inline MH_STATUS MH_CreateHookEx(LPVOID pTarget, LPVOID pDetour, T** ppOriginal)
	{
		return MH_CreateHook(pTarget, pDetour, reinterpret_cast<LPVOID*>(ppOriginal));
	}

	template <typename T>
	inline MH_STATUS MH_CreateHookApiEx(LPCWSTR pszModule, LPCSTR pszProcName, LPVOID pDetour, T** ppOriginal)
	{
		return MH_CreateHookApi(pszModule, pszProcName, pDetour, reinterpret_cast<LPVOID*>(ppOriginal));
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
			if (Globals::instance().inputBlocked() && Globals::instance().controllerControlsCamera())
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
		EnterCriticalSection(&_messageProcessCriticalSection);
		if (lpMsg != nullptr && Input::handleMessage(lpMsg))
		{
			// message was handled by our code. This means it's a message we want to block if input blocking is enabled or the overlay / menu is shown
			if (Globals::instance().inputBlocked() && Globals::instance().keyboardMouseControlCamera())
			{
				lpMsg->message = WM_NULL;	// reset to WM_NULL so the host will receive a dummy message instead.
			}
		}
		LeaveCriticalSection(&_messageProcessCriticalSection);
	}
	   

	void setXInputHook(bool enableHook)
	{
		if (nullptr != hookedXInputGetState)
		{
			return;
		}
		if (MH_CreateHookApiEx(L"xinput1_3", "XInputGetState", &detourXInputGetState, &hookedXInputGetState) != MH_OK)
		{
			MessageHandler::logError("Hooking XINPUT failed! Try re-enabling the hook with the button on the General tab after you've used the controller in-game.");
		}
		if (enableHook)
		{
			if (MH_EnableHook(MH_ALL_HOOKS) == MH_OK)
			{
				MessageHandler::logLine("Hook to XInputGetState enabled");
			}
		}
		else
		{
			MessageHandler::logDebug("Hook set to XInputGetState");
		}
	}


	// Sets the input hooks for the various input related functions we defined own wrapper functions for. After a successful hook setup they're enabled. 
	void setInputHooks()
	{
		InitializeCriticalSectionAndSpinCount(&_messageProcessCriticalSection, 0x400);

		setXInputHook(false);

		if (MH_CreateHookApiEx(L"user32", "GetMessageA", &detourGetMessageA, &hookedGetMessageA) != MH_OK)
		{
			MessageHandler::logError("Hooking GetMessageA failed!");
		}
		MessageHandler::logDebug("Hook set to GetMessageA");

		if (MH_CreateHookApiEx(L"user32", "GetMessageW", &detourGetMessageW, &hookedGetMessageW) != MH_OK)
		{
			MessageHandler::logError("Hooking GetMessageW failed!");
		}
		MessageHandler::logDebug("Hook set to GetMessageW");

		if (MH_CreateHookApiEx(L"user32", "PeekMessageA", &detourPeekMessageA, &hookedPeekMessageA) != MH_OK)
		{
			MessageHandler::logError("Hooking PeekMessageA failed!");
		}
		MessageHandler::logDebug("Hook set to PeekMessageA");

		if (MH_CreateHookApiEx(L"user32", "PeekMessageW", &detourPeekMessageW, &hookedPeekMessageW) != MH_OK)
		{
			MessageHandler::logError("Hooking PeekMessageW failed!");
		}
		MessageHandler::logDebug("Hook set to PeekMessageW");

		// Enable all hooks
		if (MH_EnableHook(MH_ALL_HOOKS) == MH_OK)
		{
			MessageHandler::logLine("All hooks enabled.");
		}
		else
		{
			MessageHandler::logError("Enabling hooks failed.");
		}
	}
}