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
#include "input.h"
#include "Utils.h"
#include "Globals.h"
#include "imgui_impl_win32.h"
#include "OverlayConsole.h"
#include "OverlayControl.h"
#include <mutex>
#include <atomic>

namespace IGCS::Input
{
	using namespace std;

	// The key/mouse states are unprotected for write/read. This is initially bad, but protecting it with a mutex is difficult as the present call
	// will read it and multiple calls from that thread causes mutex problems. For now they're unprotected (not that bad, the next frame it will be corrected anyway).
	static BYTE g_keyStates[256];		// 0x0==nothing, 0x88==key is down this frame, 0x08==key is released, 0x80==key was down previous frame.
	static BYTE g_mouseButtonStates[3];	// 0x0==nothing, 0x88==button is down this frame, 0x08==button is released, 0x80==button was down previous frame.
	static short g_mouseWheelDelta = 0;
	static atomic_long _deltaMouseX = 0;
	static atomic_long _deltaMouseY = 0;
	
	void setMouseButtonState(int button, bool down);

	void setKeyboardMouseStateInImGuiIO()
	{
		ImGuiIO& io = ImGui::GetIO();
		for (int i = 0; i < 256; i++)
		{
			if ((g_keyStates[i] & 0x88) == 0x88)
			{
				WORD ch = 0;
				io.AddInputCharacter(ToAscii(i, MapVirtualKey(i, MAPVK_VK_TO_VSC), g_keyStates, &ch, 0) ? ch : 0);
			}
		}

		for (int i = 0; i < 3; i++)
		{
			io.MouseDown[i] = (g_mouseButtonStates[i] & 0x80) == 0x80;
		}
		io.MouseWheel += g_mouseWheelDelta;
	}


	void collectPressedKeysCumulatively()
	{
		// check which keys are down. For keycode, the last one wins.
		ActionData& toCollectInto = Globals::instance().getKeyCollector();

		// keys below 7 aren't interesting.
		for (int i = 7; i < 256; i++)
		{
			if ((g_keyStates[i] & 0x88) == 0x88 || (g_keyStates[i] & 0x80)==0x80)
			{
				// this key is down.
				switch (i)
				{
				case VK_MENU:
					toCollectInto.setAltRequired();
					break;
				case VK_CONTROL:
					toCollectInto.setCtrlRequired();
					break;
				case VK_SHIFT:
					toCollectInto.setShiftRequired();
					break;
				default:
					toCollectInto.setKeyCode(i);
					break;
				}
			}
		}
	}


	bool isActionActivated(ActionType type)
	{
		return isActionActivated(type, false);
	}


	// altCtrlOptional is only effective for actions which don't have alt/ctrl/shift as a required key. Actions which do have one or more of these
	// keys as required, will ignore altCtrlShiftOptional and always test for these keys. 
	bool isActionActivated(ActionType type, bool altCtrlOptional)
	{
		ActionData* data = Globals::instance().getActionData(type);
		if (nullptr == data)
		{
			return false;
		}
		return data->isActive(altCtrlOptional);
	}


	// Resets the states in the keystates buffer by resetting their lower 4 bits. This will make sure the keystate interpretation code in NewFrame will
	// only pick up the key when its Keydown message was received the first time. 0x88/0x08 is used as this is the way GetKeyState() is reporting states too.
	void resetKeyStates()
	{
		// clear 0x08 bit on keys in keystate array so we avoid repeat issues.
		ImGuiIO& io = ImGui::GetIO();
		for (int i = 0; i < 256; i++)
		{
			g_keyStates[i] &= ~0x08;
			io.KeysDown[i] = 0x00;
		}
	}


	void resetMouseState()
	{
		// clear 0x08 bit on mouse buttons in mouse state array so we avoid repeat issues.
		for (int i = 0; i < 3; i++)
		{
			g_mouseButtonStates[i] &= ~0x08;
		}
		g_mouseWheelDelta = 0;
	}


	void setMouseButtonState(int button, bool down)
	{
		if (button < 0 || button > 3)
		{
			return;
		}
		g_mouseButtonStates[button] = down ? 0x88 : 0x08;
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

		// simply use our main window handle.
		HWND handleToUse = Globals::instance().mainWindowHandle();
		if (handleToUse == nullptr)
		{
			return false;
		}

		ImGuiIO& io = ImGui::GetIO();
		// first handle the message through the Imgui handler so we get an up to date IO structure for the overlay
		LRESULT handledByImGuiHandler = ImGui_ImplWin32_WndProcHandler(handleToUse, lpMsg->message, lpMsg->wParam, lpMsg->lParam);
		if (!handledByImGuiHandler)
		{
			// grab mouse position
			ScreenToClient(static_cast<HWND>(handleToUse), &lpMsg->pt);
			io.MousePos.x = static_cast<float>(lpMsg->pt.x);
			io.MousePos.y = static_cast<float>(lpMsg->pt.y);
		}

		bool toReturn = false;
		switch (lpMsg->message)
		{
			case WM_INPUT:
			{
				// Determine how big the buffer should be
				UINT bufferSize;
				GetRawInputData((HRAWINPUT)lpMsg->lParam, RID_INPUT, NULL, &bufferSize, sizeof(RAWINPUTHEADER));
				LPBYTE lpb = new BYTE[bufferSize];
				// Get the raw input data
				UINT readSize = GetRawInputData((HRAWINPUT)lpMsg->lParam, RID_INPUT, lpb, &bufferSize, sizeof(RAWINPUTHEADER));
				if (readSize == bufferSize)
				{
					RAWINPUT* pRawData = (RAWINPUT*)lpb;
					// Process the Mouse Messages
					if (pRawData->header.dwType == RIM_TYPEMOUSE)
					{
						processRawMouseData(&pRawData->data.mouse);
						if (!handledByImGuiHandler)
						{
							// check mouse buttons / wheel.
							if (pRawData->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
							{
								setMouseButtonState(0, true);
							}
							else
							{
								if (pRawData->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)
								{
									setMouseButtonState(0, false);
								}
							}
							if (pRawData->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
							{
								setMouseButtonState(1, true);
							}
							else
							{
								if (pRawData->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
								{
									setMouseButtonState(1, false);
								}
							}
							if (pRawData->data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
							{
								setMouseButtonState(2, true);
							}
							else
							{
								if (pRawData->data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)
								{
									setMouseButtonState(2, false);
								}
							}
							if (pRawData->data.mouse.usButtonFlags & RI_MOUSE_WHEEL)
							{
								g_mouseWheelDelta += static_cast<short>(pRawData->data.mouse.usButtonData) / WHEEL_DELTA;
							}
						}
					}
					if (pRawData->header.dwType == RIM_TYPEKEYBOARD && IGCS_SUPPORT_RAWKEYBOARDINPUT)
					{
						// convert keyboard input to keypress/keydown.
						if (pRawData->data.keyboard.VKey != 0xFF)
						{
							g_keyStates[pRawData->data.keyboard.VKey] = (pRawData->data.keyboard.Flags & RI_KEY_BREAK) == 0 ? 0x88 : 0x08;
							io.KeysDown[pRawData->data.keyboard.VKey] = 0x01;
						}
					}
				}
				delete lpb;
				toReturn = true;
			}
			break;
			// simply return true for all messages related to mouse / keyboard so they won't reach the message pump of the main window. 
			case WM_KEYDOWN:
				if (lpMsg->wParam < 256)
				{
					g_keyStates[lpMsg->wParam] = 0x88;
				}
				toReturn = true;
				break;
			case WM_KEYUP:
				if (lpMsg->wParam < 256)
				{
					g_keyStates[lpMsg->wParam] = 0x08;
				}
				toReturn = true;
				break;
			case WM_SYSKEYDOWN:
				if (IGCS_SUPPORT_RAWKEYBOARDINPUT)
				{
					if (lpMsg->wParam < 256)
					{
						g_keyStates[lpMsg->wParam] = 0x88;
					}
					toReturn = true;
				}
				break;
			case WM_SYSKEYUP:
				if (IGCS_SUPPORT_RAWKEYBOARDINPUT)
				{
					if (lpMsg->wParam < 256)
					{
						g_keyStates[lpMsg->wParam] = 0x08;
					}
					toReturn = true;
				}
				break;
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

		if (!g_cameraEnabled && !OverlayControl::isMainMenuVisible())
		{
			// let the game also handle the message
			return false;
		}
		return toReturn;
	}
}