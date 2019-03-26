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
#include "input.h"
#include "Utils.h"
#include "Globals.h"
#include "imgui_impl_dx11.h"
#include "OverlayConsole.h"
#include "OverlayControl.h"

namespace IGCS::Input
{
	using namespace std;

	// I know, multi-threaded programming, but these two values are scalars, and read/write of multiple threads (which is the case) doesn't really 
	// matter: wrapping this in mutexes is overkill: x86 processors can write up to 7 bytes in an atomic instruction, more than enough for a long, 
	// and the values are used in a system which is updated rapidly, so if a write overlaps a read, the next frame will correct that. The 'volatile'
	// keyword should be enough to mark them for the optimizer not to mess with them. 
	volatile static long _deltaMouseX = 0;
	volatile static long _deltaMouseY = 0;


	bool keyDown(int virtualKeyCode)
	{
		return (GetKeyState(virtualKeyCode) & 0x8000);
	}


	void resetMouseDeltas()
	{
		_deltaMouseX = 0;
		_deltaMouseY = 0;
	}


	void processRawMouseData(const RAWMOUSE *rmouse)
	{
		if (MOUSE_MOVE_RELATIVE == rmouse->usFlags)
		{
			_deltaMouseX = rmouse->lLastX;
			_deltaMouseY = rmouse->lLastY;
		}
	}


	long getMouseDeltaX()
	{
		return _deltaMouseX;
	}


	long getMouseDeltaY()
	{
		return _deltaMouseY;
	}


	void registerRawInput()
	{
		// get the main window of the host.
		RAWINPUTDEVICE rid[1];
		rid[0].usUsagePage = 0x01;
		rid[0].usUsage = 0x02;
		rid[0].dwFlags = 0;
		rid[0].hwndTarget = Globals::instance().mainWindowHandle();

		if (RegisterRawInputDevices(rid, 1, sizeof(rid[0])) == FALSE)
		{
			OverlayConsole::instance().logError("Couldn't register raw input. Error code: %010x", GetLastError());
		}
		else
		{
			OverlayConsole::instance().logDebug("Raw input registered");
		}
	}


	// returns true if the message was handled by this method, otherwise false.
	bool handleMessage(LPMSG lpMsg)
	{
		if (lpMsg == nullptr || lpMsg->hwnd == nullptr)
		{
			return false;
		}

		// first handle the message through the Imgui handler so we get an up to date IO structure for the overlay
		ImGui_ImplDX11_WndProcHandler(lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);

		// Stop here if the camera isn't enabled as the camera isn't controllable in that situation. 
		if(!g_cameraEnabled && !OverlayControl::isMainMenuVisible())
		{
			return false;
		}
		bool toReturn = false;
		switch (lpMsg->message)
		{
			case WM_INPUT:
			{
				// handle mouse
				RAWINPUT *pRI = NULL;

				// Determine how big the buffer should be
				UINT iBuffer;
				GetRawInputData((HRAWINPUT)lpMsg->lParam, RID_INPUT, NULL, &iBuffer, sizeof(RAWINPUTHEADER));
				// Allocate a buffer with enough size to hold the raw input data
				LPBYTE lpb = new BYTE[iBuffer];
				if (lpb == NULL)
				{
					return false;
				}
				// Get the raw input data
				UINT readSize = GetRawInputData((HRAWINPUT)lpMsg->lParam, RID_INPUT, lpb, &iBuffer, sizeof(RAWINPUTHEADER));
				if (readSize == iBuffer)
				{
					pRI = (RAWINPUT*)lpb;
					// Process the Mouse Messages
					if (pRI->header.dwType == RIM_TYPEMOUSE)
					{
						processRawMouseData(&pRI->data.mouse);
					}
				}
				delete lpb;
				toReturn = true;
			}
			break;
			// simply return true for all messages related to mouse / keyboard so they won't reach the message pump of the main window. 
			case WM_KEYDOWN:
			case WM_KEYUP:
			case WM_CAPTURECHANGED:
			case WM_LBUTTONDBLCLK:
			case WM_LBUTTONDOWN:
			case WM_MBUTTONDBLCLK:
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
			case WM_MOUSEACTIVATE:
			case WM_MOUSEHOVER:
			case WM_MOUSEHWHEEL:
			case WM_MOUSEMOVE:
			case WM_MOUSELEAVE:
			case WM_MOUSEWHEEL:
			case WM_NCHITTEST:
			case WM_NCLBUTTONDBLCLK:
			case WM_NCLBUTTONDOWN:
			case WM_NCLBUTTONUP:
			case WM_NCMBUTTONDBLCLK:
			case WM_NCMBUTTONDOWN:
			case WM_NCMBUTTONUP:
			case WM_NCMOUSEHOVER:
			case WM_NCMOUSELEAVE:
			case WM_NCMOUSEMOVE:
			case WM_NCRBUTTONDBLCLK:
			case WM_NCRBUTTONDOWN:
			case WM_NCRBUTTONUP:
			case WM_NCXBUTTONDBLCLK:
			case WM_NCXBUTTONDOWN:
			case WM_NCXBUTTONUP:
			case WM_RBUTTONDBLCLK:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
			case WM_XBUTTONDBLCLK:
			case WM_XBUTTONDOWN:
			case WM_XBUTTONUP:
			case WM_LBUTTONUP:
				// say we handled it, so the host won't see it
				toReturn = true;
				break;
			break;
		}
		return toReturn;
	}
}