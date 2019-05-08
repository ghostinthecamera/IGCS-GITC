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
#include "input.h"
#include "Camera.h"
#include "InterceptorHelper.h"
#include "Gamepad.h"
#include "Utils.h"

using namespace std;

//--------------------------------------------------------------------------------------------------------------------------------
// data shared with asm functions. This is allocated here, 'C' style and not in some datastructure as passing that to 
// MASM is rather tedious. 
extern "C" {
	BYTE _cameraEnabled = 0;
	//byte _timeStopped = 0;
}

//--------------------------------------------------------------------------------------------------------------------------------
// local data 
static Camera* _camera=NULL;
static Gamepad* _gamePad = NULL;
static Console* _consoleWrapper=NULL;
static LPBYTE _hostImageAddress = NULL;
static bool _cameraMovementLocked = false;
static bool _cameraStructFound = false;
static bool _mouseKeyboardInputBlocked = false;
static float _lookDirectionInverter = 1.0f;
//static bool _alternativeTimeStopped = false;
static WNDPROC _oldMainWindowWndProc = 0;

//--------------------------------------------------------------------------------------------------------------------------------
// Local function definitions
void MainLoop();
void UpdateFrame();
void HandleUserInput();
void InitCamera();
void WriteNewCameraValuesToCameraStructs();
void DisplayHelp();
void InjectMessageInterceptor();

//--------------------------------------------------------------------------------------------------------------------------------
// Implementations
void SystemStart(HMODULE hostBaseAddress, Console c)
{
	g_systemActive = true;
	_consoleWrapper = &c;
	_hostImageAddress = (LPBYTE)hostBaseAddress;
	_gamePad = new Gamepad(0);
	_gamePad->setInvertLStickY(CONTROLLER_Y_INVERT);
	_gamePad->setInvertRStickY(CONTROLLER_Y_INVERT);
	DisplayHelp();
	// initialization
	SetCameraStructInterceptorHook(_hostImageAddress);
	//SetTimestopInterceptorHook(_hostImageAddress);
	InitCamera();
	InjectMessageInterceptor();
	SetCameraWriteInterceptorHooks(_hostImageAddress);
	//SetFoVWriteInterceptorHooks(_hostImageAddress);
	// done initializing, start main loop of camera. We won't return from this
	MainLoop();
	delete _gamePad;
}


// Core loop of the system
void MainLoop()
{
	while (g_systemActive)
	{
		Sleep(FRAME_SLEEP);
		UpdateFrame();
	}
}


// updates the data and camera for a frame 
void UpdateFrame()
{
	HandleUserInput();
	WriteNewCameraValuesToCameraStructs();
}


void HandleUserInput()
{
	_gamePad->update();
	bool altPressed = KeyDown(VK_MENU) || KeyDown(VK_RMENU);
	bool controlPressed = KeyDown(VK_CONTROL) || KeyDown(VK_RCONTROL);

	if (KeyDown(IGCS_KEY_HELP) && altPressed)
	{
		DisplayHelp();
		// wait for 350ms to avoid fast keyboard hammering displaying multiple times the help!
		Sleep(350);
	}
	if (KeyDown(IGCS_KEY_INVERT_Y_LOOK))
	{
		_lookDirectionInverter = -_lookDirectionInverter;
		if (_lookDirectionInverter < 0)
		{
			_consoleWrapper->WriteLine("Y look direction is inverted");
		}
		else
		{
			_consoleWrapper->WriteLine("Y look direction is normal");
		}
		// wait for 350ms to avoid fast keyboard hammering switching look directive multiple times
		Sleep(350);
	}
	if(!_cameraStructFound)
	{
		// camera not found yet, can't proceed.
		return;
	}
	if(KeyDown(IGCS_KEY_CAMERA_ENABLE))
	{
		if (_cameraEnabled)
		{
			_consoleWrapper->WriteLine("Camera disabled");
		}
		else
		{
			_consoleWrapper->WriteLine("Camera enabled");
			_camera->ResetAngles();
		}
		_cameraEnabled = _cameraEnabled == 0 ? (BYTE)1 : (BYTE)0;
		// wait for 350ms to avoid fast keyboard hammering disabling the camera right away
		Sleep(350);
	}
	//if (KeyDown(IGCS_KEY_TIMESTOP))
	//{
	//	if (_timeStopped)
	//	{
	//		_consoleWrapper->WriteLine("Game unpaused");
	//	}
	//	else
	//	{
	//		_consoleWrapper->WriteLine("Game paused");
	//	}
	//	_timeStopped = _timeStopped == 0 ? (byte)1 : (byte)0;
	//	SetTimeStopValue(_timeStopped);
	//	// wait 350 ms to avoid fast keyboard hammering unlocking/locking the timestop right away
	//	Sleep(350);
	//}
	
	if (KeyDown(IGCS_KEY_BLOCK_MKB_INPUT))
	{
		if (_mouseKeyboardInputBlocked)
		{
			_consoleWrapper->WriteLine("Mouse / Keyboard input to game resumed");
		}
		else
		{
			_consoleWrapper->WriteLine("Mouse / Keyboard input to game blocked");
		}
		_mouseKeyboardInputBlocked = !_mouseKeyboardInputBlocked;
		// wait 350 ms to avoid fast keyboard hammering
		Sleep(350);
	}

	//////////////////////////////////////////////////// CAMERA
	if(!_cameraEnabled)
	{
		// camera is disabled. We simply disable all input to the camera movement. 
		return;
	}

	//if (KeyDown(IGCS_KEY_FOV_DECREASE))
	//{
	//	ChangeFoV(-DEFAULT_FOV_SPEED);
	//}
	//if (KeyDown(IGCS_KEY_FOV_INCREASE))
	//{
	//	ChangeFoV(DEFAULT_FOV_SPEED);
	//}

	_camera->ResetDeltas();
	float multiplier = altPressed ? FASTER_MULTIPLIER : controlPressed ? SLOWER_MULTIPLIER : 1.0f;

	if (KeyDown(IGCS_KEY_CAMERA_LOCK))
	{
		if (_cameraMovementLocked)
		{
			_consoleWrapper->WriteLine("Camera movement unlocked");
		}
		else
		{
			_consoleWrapper->WriteLine("Camera movement locked");
		}
		_cameraMovementLocked = !_cameraMovementLocked;
		// wait 350 ms to avoid fast keyboard hammering unlocking/locking the camera movement right away
		Sleep(350);
	}
	if (_cameraMovementLocked)
	{
		// no movement allowed, simply return
		return;
	}
	if(KeyDown(IGCS_KEY_MOVE_FORWARD))
	{
		_camera->MoveForward(multiplier);
	}
	if(KeyDown(IGCS_KEY_MOVE_BACKWARD))
	{
		_camera->MoveForward(-multiplier);
	}
	if (KeyDown(IGCS_KEY_MOVE_RIGHT))
	{
		_camera->MoveRight(multiplier);
	}
	if (KeyDown(IGCS_KEY_MOVE_LEFT))
	{
		_camera->MoveRight(-multiplier);
	}
	if (KeyDown(IGCS_KEY_MOVE_UP))
	{
		_camera->MoveUp(multiplier);
	}
	if (KeyDown(IGCS_KEY_MOVE_DOWN))
	{
		_camera->MoveUp(-multiplier);
	}
	if (KeyDown(IGCS_KEY_ROTATE_DOWN))
	{
		_camera->Pitch(multiplier * _lookDirectionInverter);
	}
	if (KeyDown(IGCS_KEY_ROTATE_UP))
	{
		_camera->Pitch((-multiplier) * _lookDirectionInverter);
	}
	if (KeyDown(IGCS_KEY_ROTATE_RIGHT))
	{
		_camera->Yaw(multiplier);
	}
	if (KeyDown(IGCS_KEY_ROTATE_LEFT))
	{
		_camera->Yaw(-multiplier);
	}
	if (KeyDown(IGCS_KEY_TILT_LEFT))
	{
		_camera->Roll(multiplier);
	}
	if (KeyDown(IGCS_KEY_TILT_RIGHT))
	{
		_camera->Roll(-multiplier);
	}

	// mouse 
	long mouseDeltaX = GetMouseDeltaX();
	long mouseDeltaY = GetMouseDeltaY();
	if(mouseDeltaY != 0)
	{
		_camera->Pitch(static_cast<float>(mouseDeltaY) * MOUSE_SPEED_CORRECTION * multiplier * _lookDirectionInverter);
	}
	if(mouseDeltaX != 0)
	{
		_camera->Yaw(static_cast<float>(mouseDeltaX) * MOUSE_SPEED_CORRECTION * multiplier);
	}
	// after we've read them, we can clear them
	ResetMouseDeltas();

	// gamepad
	if (_gamePad->isConnected())
	{
		multiplier = _gamePad->isButtonPressed(IGCS_BUTTON_FASTER) ? FASTER_MULTIPLIER : _gamePad->isButtonPressed(IGCS_BUTTON_SLOWER) ? SLOWER_MULTIPLIER : multiplier;

		vec2 rightStickPosition = _gamePad->getRStickPosition();
		_camera->Pitch(rightStickPosition.y * multiplier * _lookDirectionInverter);
		_camera->Yaw(rightStickPosition.x * multiplier);

		vec2 leftStickPosition = _gamePad->getLStickPosition();
		_camera->MoveForward(-leftStickPosition.y * multiplier);
		_camera->MoveRight(leftStickPosition.x * multiplier);
		_camera->MoveUp((_gamePad->getLTrigger() - _gamePad->getRTrigger()) * multiplier);

		if (_gamePad->isButtonPressed(IGCS_BUTTON_TILT_LEFT))
		{
			_camera->Roll(multiplier);
		}
		if (_gamePad->isButtonPressed(IGCS_BUTTON_TILT_RIGHT))
		{
			_camera->Roll(-multiplier);
		}
		//if (_gamePad->isButtonPressed(IGCS_BUTTON_FOV_DECREASE))
		//{
		//	ChangeFoV(-DEFAULT_FOV_SPEED);
		//}
		//if (_gamePad->isButtonPressed(IGCS_BUTTON_FOV_INCREASE))
		//{
		//	ChangeFoV(DEFAULT_FOV_SPEED);
		//}
	}
}


void WriteNewCameraValuesToCameraStructs()
{
	if (!_cameraEnabled)
	{
		return;
	}

	// calculate new camera values. We don't use the current quaternion at the moment as it bugs in some scenes where the quaternion is the one used in some effect layer. 
	// so we calculate one from scratch.
	XMVECTOR newLookQuaternion = _camera->CalculateLookQuaternion();
	XMFLOAT3 currentCoords = GetCurrentCameraCoords();
	XMFLOAT3 newCoords = _camera->CalculateNewCoords(currentCoords, newLookQuaternion);
	WriteNewCameraValuesToGameData(newLookQuaternion, newCoords);
}


void InitCamera()
{
	_consoleWrapper->WriteLine("Waiting for camera struct interception...");
	WaitForCameraStructAddresses();
	_cameraStructFound = true;
	_consoleWrapper->WriteLine("Camera found.");

	_camera = new Camera();
	_camera->SetPitch(INITIAL_PITCH_RADIANS);
	_camera->SetRoll(INITIAL_ROLL_RADIANS);
	_camera->SetYaw(INITIAL_YAW_RADIANS);
}


LRESULT CALLBACK InterceptorWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	if (_cameraEnabled)
	{
		// messages we need to handle ourselves. 
		switch(uMsg)
		{
			case WM_INPUT:
				// handle mouse
				RAWINPUT *pRI = NULL;

				// Determine how big the buffer should be
				UINT iBuffer;
				GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &iBuffer, sizeof(RAWINPUTHEADER));
				// Allocate a buffer with enough size to hold the raw input data
				LPBYTE lpb = new BYTE[iBuffer];
				if (lpb == NULL)
				{
					return 0;
				}
				// Get the raw input data
				UINT readSize = GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &iBuffer, sizeof(RAWINPUTHEADER));
				if (readSize == iBuffer)
				{
					pRI = (RAWINPUT*)lpb;
					// Process the Mouse Messages
					if (pRI->header.dwType == RIM_TYPEMOUSE)
					{
						ProcessRawMouseData(&pRI->data.mouse);
					}
				}
				delete lpb;
				break;
		}
	}
	if (_mouseKeyboardInputBlocked) 
	{
		switch (uMsg) 
		{
			// simply return 1 for all messages related to mouse / keyboard so they won't reach the message pump of the main window. 
			case WM_KEYDOWN:
			case WM_KEYUP:
			case WM_INPUT:
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
			return 1L;
		}
	}
	return CallWindowProc(_oldMainWindowWndProc, hwnd, uMsg, wParam, lParam);
}


void InjectMessageInterceptor()
{
	HWND mainWindowHandle = FindMainWindow(GetCurrentProcessId());
	// WARNING: For x64 bit, cast InterceptorWndProc to (LONG_PTR) only, not (LONG)(LONG_PTR)! https://blogs.msdn.microsoft.com/oldnewthing/20131226-00/?p=2263
	_oldMainWindowWndProc = (WNDPROC)SetWindowLongPtr(mainWindowHandle, GWL_WNDPROC, (LONG)(LONG_PTR)InterceptorWndProc);
}


void DisplayHelp()
{
	//0         1         2         3         4         5         6         7
	//01234567890123456789012345678901234567890123456789012345678901234567890123456789
	_consoleWrapper->WriteLine("---[IGCS Help]----------------------------------------------------------------", CONSOLE_WHITE);
	_consoleWrapper->WriteLine("INS                                      : Enable/Disable camera");
	_consoleWrapper->WriteLine("HOME                                     : Lock/unlock camera movement");
	_consoleWrapper->WriteLine("ALT+rotate/move                          : Faster rotate / move");
	_consoleWrapper->WriteLine("CTRL+rotate/move                         : Slower rotate / move");
	_consoleWrapper->WriteLine("Controller Y-button + left/right-stick   : Faster rotate / move");
	_consoleWrapper->WriteLine("Controller X-button + left/right-stick   : Slower rotate / move");
	_consoleWrapper->WriteLine("Arrow up/down or mouse or right-stick    : Rotate camera up/down");
	_consoleWrapper->WriteLine("Arrow left/right or mouse or right-stick : Rotate camera left/right");
	_consoleWrapper->WriteLine("Numpad 8/Numpad 5 or left-stick          : Move camera forward/backward");
	_consoleWrapper->WriteLine("Numpad 4/Numpad 6 or left-stick          : Move camera left / right");
	_consoleWrapper->WriteLine("Numpad 7/Numpad 9 or left/right trigger  : Move camera up / down");
	_consoleWrapper->WriteLine("Numpad 1/Numpad 3 or d-pad left/right    : Tilt camera left / right");
	_consoleWrapper->WriteLine("Numpad +/Numpad - or d-pad up/down       : Free camera increase / decrease FoV");
	_consoleWrapper->WriteLine("Numpad 0                                 : Pause / Continue game (see readme!)");
	_consoleWrapper->WriteLine("Numpad .                                 : Toggle mouse/keyboard input to game");
	_consoleWrapper->WriteLine("Numpad /                                 : Toggle Y look direction");
	_consoleWrapper->WriteLine("ALT+H                                    : This help");
	_consoleWrapper->WriteLine("------------------------------------------------------------------------------", CONSOLE_WHITE);
}