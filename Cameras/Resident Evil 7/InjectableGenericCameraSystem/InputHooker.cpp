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
	typedef UINT(WINAPI* GETRAWINPUTDATA)(HRAWINPUT, UINT, LPVOID, PUINT, UINT);
	typedef SHORT(WINAPI* GETASYNCKEYSTATE)(int);
	typedef SHORT(WINAPI* GETKEYSTATE)(int);

	//--------------------------------------------------------------------------------------------------------------------------------
	// Pointers to the original hooked functions
	static XINPUTGETSTATE hookedXInputGetState = nullptr;
	static GETMESSAGEA hookedGetMessageA = nullptr;
	static GETMESSAGEW hookedGetMessageW = nullptr;
	static PEEKMESSAGEA hookedPeekMessageA = nullptr;
	static PEEKMESSAGEW hookedPeekMessageW = nullptr;
	static GETRAWINPUTDATA hookedGetRawInputData = nullptr;
	static GETASYNCKEYSTATE hookedGetAsyncKeyState = nullptr;
	static GETKEYSTATE hookedGetKeyState = nullptr;

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

	UINT WINAPI detourGetRawInputData(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader)
	{
		// First call the original function
		UINT result = hookedGetRawInputData(hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);

		// Only process actual data (not size queries)
		if (result > 0 && pData != nullptr && uiCommand == RID_INPUT)
		{
			RAWINPUT* rawInput = (RAWINPUT*)pData;

			// Only handle keyboard input
			if (rawInput->header.dwType == RIM_TYPEKEYBOARD)
			{
				// Check if we should block input
				bool shouldBlock = g_cameraEnabled &&
					Globals::instance().inputBlocked() &&
					Globals::instance().keyboardMouseControlCamera();

				if (shouldBlock)
				{
					// Instead of returning an error, clear the keyboard data
					// This way your tools can still process the structure
					rawInput->data.keyboard.VKey = 0;
					rawInput->data.keyboard.MakeCode = 0;
					rawInput->data.keyboard.Flags = 0;
					rawInput->data.keyboard.Reserved = 0;
					rawInput->data.keyboard.ExtraInformation = 0;
				}
			}
		}

		return result;  // Always return the original result
	}

	SHORT WINAPI detourGetAsyncKeyState(int vKey)
	{
		SHORT result = hookedGetAsyncKeyState(vKey);

		// Get the return address to check if this call is from our DLL
		HMODULE callerModule = nullptr;
		GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
			static_cast<LPCTSTR>(_ReturnAddress()),
			&callerModule);

		// Only block if not called from our DLL
		if (callerModule != _ourDllModule && g_cameraEnabled &&
			Globals::instance().inputBlocked() &&
			Globals::instance().keyboardMouseControlCamera())
		{
			return 0;
		}

		return result;
	}

	SHORT WINAPI detourGetKeyState(int vKey)
	{
		SHORT result = hookedGetKeyState(vKey);

		// Get the return address to check if this call is from our DLL
		HMODULE callerModule = nullptr;
		GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
			static_cast<LPCTSTR>(_ReturnAddress()),
			&callerModule);

		// Only block if not called from our DLL
		if (callerModule != _ourDllModule && g_cameraEnabled &&
			Globals::instance().inputBlocked() &&
			Globals::instance().keyboardMouseControlCamera())
		{
			return 0;
		}

		return result;
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
		if (MH_CreateHookApiEx(XINPUT_VERSION, "XInputGetState", &detourXInputGetState, &hookedXInputGetState) != MH_OK)
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

	void setRawInputHook(bool enableHook)
	{
		if (nullptr != hookedGetRawInputData)
		{
			return;
		}
		if (MH_CreateHookApiEx(L"user32", "GetRawInputData", &detourGetRawInputData, &hookedGetRawInputData) != MH_OK)
		{
			MessageHandler::logError("Hooking GetRawInputData failed!");
		}
		if (enableHook)
		{
			if (MH_EnableHook(MH_ALL_HOOKS) == MH_OK)
			{
				MessageHandler::logLine("Hook to GetRawInputData enabled");
			}
		}
		else
		{
			MessageHandler::logDebug("Hook set to GetRawInputData");
		}
	}


	// Sets the input hooks for the various input related functions we defined own wrapper functions for. After a successful hook setup they're enabled. 
	void setInputHooks()
	{

		// Get our DLL module handle once
		GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
			LPCTSTR(&setInputHooks),
			&_ourDllModule);

		InitializeCriticalSectionAndSpinCount(&_messageProcessCriticalSection, 0x400);

		setXInputHook(false);
		//setRawInputHook(false); 

		if (MH_CreateHookApiEx(L"user32", "GetMessageA", &detourGetMessageA, &hookedGetMessageA) != MH_OK)
			MessageHandler::logError("Hooking GetMessageA failed!");
		else
			MessageHandler::logDebug("Hook set to GetMessageA");

		if (MH_CreateHookApiEx(L"user32", "GetMessageW", &detourGetMessageW, &hookedGetMessageW) != MH_OK)
			MessageHandler::logError("Hooking GetMessageW failed!");
		else
			MessageHandler::logDebug("Hook set to GetMessageW");

		if (MH_CreateHookApiEx(L"user32", "PeekMessageA", &detourPeekMessageA, &hookedPeekMessageA) != MH_OK)
			MessageHandler::logError("Hooking PeekMessageA failed!");
		else
			MessageHandler::logDebug("Hook set to PeekMessageA");

		if (MH_CreateHookApiEx(L"user32", "PeekMessageW", &detourPeekMessageW, &hookedPeekMessageW) != MH_OK)
			MessageHandler::logError("Hooking PeekMessageW failed!");
		else
			MessageHandler::logDebug("Hook set to PeekMessageW");

		if (MH_CreateHookApiEx(L"user32", "GetAsyncKeyState", &detourGetAsyncKeyState, &hookedGetAsyncKeyState) != MH_OK)
			MessageHandler::logError("Hooking GetAsyncKeyState failed!");
		else
			MessageHandler::logDebug("Hook set to GetAsyncKeyState");

		if (MH_CreateHookApiEx(L"user32", "GetKeyState", &detourGetKeyState, &hookedGetKeyState) != MH_OK)
			MessageHandler::logError("Hooking GetKeyState failed!");
		else
			MessageHandler::logDebug("Hook set to GetKeyState");

		// Enable all hooks
		if (MH_EnableHook(MH_ALL_HOOKS) == MH_OK)
			MessageHandler::logLine("All hooks enabled.");
		else
			MessageHandler::logError("Enabling hooks failed.");
	}
}