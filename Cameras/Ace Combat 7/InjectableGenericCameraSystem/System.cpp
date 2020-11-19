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
#include "System.h"
#include "Globals.h"
#include "Defaults.h"
#include "GameConstants.h"
#include "Gamepad.h"
#include "CameraManipulator.h"
#include "InterceptorHelper.h"
#include "InputHooker.h"
#include "input.h"
#include "CameraManipulator.h"
#include "GameImageHooker.h"
#include "UniversalD3D11Hook.h"
#include "OverlayConsole.h"
#include "OverlayControl.h"

namespace IGCS
{
	using namespace IGCS::GameSpecific;

	System::System()
	{
	}


	System::~System()
	{
	}

	BYTE hudstate = (BYTE)1;
	BYTE prevhudState = (BYTE)1;
	BYTE slomoActive = (BYTE)0;
	BYTE gamepauseState = (BYTE)0;

	void System::start(LPBYTE hostBaseAddress, DWORD hostImageSize)
	{
		Globals::instance().systemActive(true);
		_hostImageAddress = (LPBYTE)hostBaseAddress;
		_hostImageSize = hostImageSize;
		Globals::instance().gamePad().setInvertLStickY(CONTROLLER_Y_INVERT);
		Globals::instance().gamePad().setInvertRStickY(CONTROLLER_Y_INVERT);
		initialize();		// will block till camera is found
		mainLoop();
	}


	// Core loop of the system
	void System::mainLoop()
	{
		while (Globals::instance().systemActive())
		{
			Sleep(FRAME_SLEEP);
			updateFrame();
		}
	}


	// updates the data and camera for a frame 
	void System::updateFrame()
	{
		handleUserInput();
		writeNewCameraValuesToCameraStructs();
	}
	

	void System::writeNewCameraValuesToCameraStructs()
	{
		if (!g_cameraEnabled)
		{
			return;
		}

		// calculate new camera values.
		XMVECTOR newLookQuaternion = _camera.calculateLookQuaternion();
		XMFLOAT3 currentCoords = GameSpecific::CameraManipulator::getCurrentCameraCoords();
		XMFLOAT3 newCoords = _camera.calculateNewCoords(currentCoords, newLookQuaternion);
		float convertFactor = 180.0f / XM_PI;
		GameSpecific::CameraManipulator::writeNewCameraValuesToGameData(newCoords, (_camera.getPitch() * convertFactor), (_camera.getYaw() * convertFactor), 
																		(_camera.getRoll() * convertFactor));
	}


	void System::handleUserInput()
	{
		Globals::instance().gamePad().update();
		bool altPressed = Input::keyDown(VK_LMENU) || Input::keyDown(VK_RMENU);
		bool rcontrolPressed = Input::keyDown(VK_RCONTROL);
		bool lcontrolPressed = Input::keyDown(VK_LCONTROL);

		// check if ultrawide fix is enabled
		if (Globals::instance().settings().ultrawidefix)
		{
			CameraManipulator::ultrawidefix();
		}
		else
		{
			g_ultraWidefix = (BYTE)0;
		}


		if (Input::keyDown(IGCS_KEY_TOGGLE_OVERLAY) && (lcontrolPressed || rcontrolPressed))
		{
			OverlayControl::toggleOverlay();
			Sleep(350);		// wait 100ms to avoid fast keyboard hammering
			// we're done now. 
			return;
		}
		if (!_cameraStructFound)
		{
			// camera not found yet, can't proceed.
			return;
		}
		if (OverlayControl::isMainMenuVisible() && !Globals::instance().settings().allowCameraMovementWhenMenuIsUp)
		{
			// stop here, so keys used in the camera system won't affect anything of the camera
			return;
		}
		if (Input::keyDown(IGCS_KEY_SLOMO))
		{
				CameraManipulator::sloMoFunc(Globals::instance().settings().slomoMult);
				Sleep(350);
		}
		if (Input::keyDown(IGCS_KEY_CAMERA_ENABLE))
		{
			if (g_cameraEnabled)
			{
				// it's going to be disabled, make sure things are alright when we give it back to the host
				CameraManipulator::restoreOriginalCameraValues();
				toggleCameraMovementLockState(false);
			}
			else
			{
				// it's going to be enabled, so cache the original values before we enable it so we can restore it afterwards
				CameraManipulator::cacheOriginalCameraValues();
				_camera.resetAngles();
				CameraManipulator::resetFoV();
			}
			g_cameraEnabled = g_cameraEnabled == 0 ? (BYTE)1 : (BYTE)0;
			displayCameraState();
			Sleep(350);				// wait for 350ms to avoid fast keyboard hammering
		}
		if (Input::keyDown(IGCS_KEY_TIMESTOP))
		{
			if (Globals::instance().settings().hudandtimestop)
			{
				float* hud1InMemory = reinterpret_cast<float*>(_hostImageAddress + HUD_TOGGLE_1);
				BYTE* hud2InMemory = reinterpret_cast<BYTE*>(_hostImageAddress + HUD_TOGGLE_2);

				if (*hud2InMemory == 1 && *hud1InMemory > 0.9f && !g_gamePaused)
				{
					prevhudState = (BYTE)1;
				}
				else if ((*hud2InMemory == 0 && *hud1InMemory < 0.5f && !g_gamePaused))
				{
					prevhudState = (BYTE)0;
				}

				if (g_gamePaused == 0 && hudstate == 1)
				{
					CameraManipulator::hudToggle();
					hudstate = hudstate == 1 ? (BYTE)0 : (BYTE)1;
					displayGameHUDState();
					Sleep(300);
				}
				if (g_gamePaused == 1 && hudstate == 0 && prevhudState == 1)
				{
					CameraManipulator::hudToggle();
					hudstate = hudstate == 1 ? (BYTE)0 : (BYTE)1;
					displayGameHUDState();
					Sleep(300);
				}
			}
		g_gamePaused = g_gamePaused == 0 ? (BYTE)1 : (BYTE)0;
		CameraManipulator::timeStop();
		displayGamePauseState();
		Sleep(350);
		}

		if (Input::keyDown(IGCS_KEY_HUD_TOGGLE))
		{
			hudstate = hudstate == 0 ? (BYTE)1 : (BYTE)0;
			CameraManipulator::hudToggle();
			displayGameHUDState();
			Sleep(350);
		}

		if (Input::keyDown(IGCS_KEY_FOV_RESET))
		{
			CameraManipulator::resetFoV();
		}
		if (Input::keyDown(IGCS_KEY_FOV_DECREASE))
		{
			CameraManipulator::changeFoV(-Globals::instance().settings().fovChangeSpeed);
		}
		if (Input::keyDown(IGCS_KEY_FOV_INCREASE))
		{
			CameraManipulator::changeFoV(Globals::instance().settings().fovChangeSpeed);
		}

		if (Input::keyDown(IGCS_KEY_ADVANCE))
		{
			if (g_gamePaused)
			{
				CameraManipulator::plusFrame(Globals::instance().settings().frameskip);
				displayframeskip();
				Sleep(350);
			}
		}

		if (!g_cameraEnabled)
		{
			// camera is disabled. We simply disable all input to the camera movement, by returning now.
			return;
		}

		if (Input::keyDown(IGCS_KEY_BLOCK_INPUT))
		{
			toggleInputBlockState(!Globals::instance().inputBlocked());
			Sleep(350);				// wait for 350ms to avoid fast keyboard hammering
		}
		
		_camera.resetMovement();
		Settings& settings = Globals::instance().settings();
		float multiplier = altPressed ? settings.fastMovementMultiplier : rcontrolPressed ? settings.slowMovementMultiplier : 1.0f;
		if (Input::keyDown(IGCS_KEY_CAMERA_LOCK))
		{
			toggleCameraMovementLockState(!_cameraMovementLocked);
			Sleep(350);				// wait for 350ms to avoid fast keyboard hammering
		}
		if (_cameraMovementLocked)
		{
			// no movement allowed, simply return
			return;
		}

		handleKeyboardCameraMovement(multiplier);
		handleMouseCameraMovement(multiplier);
		handleGamePadMovement(multiplier);
	}


	void System::handleGamePadMovement(float multiplierBase)
	{
		auto gamePad = Globals::instance().gamePad();

		if (gamePad.isConnected())
		{
			Settings& settings = Globals::instance().settings();
			float  multiplier = gamePad.isButtonPressed(IGCS_BUTTON_FASTER) ? settings.fastMovementMultiplier 
																			: gamePad.isButtonPressed(IGCS_BUTTON_SLOWER) ? settings.slowMovementMultiplier : multiplierBase;
			vec2 rightStickPosition = gamePad.getRStickPosition();
			_camera.pitch(rightStickPosition.y * multiplier);
			_camera.yaw(rightStickPosition.x * multiplier);

			vec2 leftStickPosition = gamePad.getLStickPosition();
			_camera.moveUp((gamePad.getLTrigger() - gamePad.getRTrigger()) * multiplier);
			_camera.moveForward(leftStickPosition.y * multiplier);
			_camera.moveRight(leftStickPosition.x * multiplier);

			if (gamePad.isButtonPressed(IGCS_BUTTON_TILT_LEFT))
			{
				_camera.roll(multiplier);
			}
			if (gamePad.isButtonPressed(IGCS_BUTTON_TILT_RIGHT))
			{
				_camera.roll(-multiplier);
			}
			if (gamePad.isButtonPressed(IGCS_BUTTON_FOV_DECREASE))
			{
				CameraManipulator::changeFoV(-Globals::instance().settings().fovChangeSpeed);
			}
			if (gamePad.isButtonPressed(IGCS_BUTTON_FOV_INCREASE))
			{
				CameraManipulator::changeFoV(Globals::instance().settings().fovChangeSpeed);
			}
		}
	}


	void System::handleMouseCameraMovement(float multiplier)
	{
		long mouseDeltaX = Input::getMouseDeltaX();
		long mouseDeltaY = Input::getMouseDeltaY();
		if (abs(mouseDeltaY) > 1)
		{
			_camera.pitch(-(static_cast<float>(mouseDeltaY) * MOUSE_SPEED_CORRECTION * multiplier));
		}
		if (abs(mouseDeltaX) > 1)
		{
			_camera.yaw(static_cast<float>(mouseDeltaX) * MOUSE_SPEED_CORRECTION * multiplier);
		}
	}


	void System::handleKeyboardCameraMovement(float multiplier)
	{
		if (Input::keyDown(IGCS_KEY_MOVE_FORWARD))
		{
			_camera.moveForward(multiplier);
		}
		if (Input::keyDown(IGCS_KEY_MOVE_BACKWARD))
		{
			_camera.moveForward(-multiplier);
		}
		if (Input::keyDown(IGCS_KEY_MOVE_RIGHT))
		{
			_camera.moveRight(multiplier);
		}
		if (Input::keyDown(IGCS_KEY_MOVE_LEFT))
		{
			_camera.moveRight(-multiplier);
		}
		if (Input::keyDown(IGCS_KEY_MOVE_UP))
		{
			_camera.moveUp(multiplier);
		}
		if (Input::keyDown(IGCS_KEY_MOVE_DOWN))
		{
			_camera.moveUp(-multiplier);
		}
		if (Input::keyDown(IGCS_KEY_ROTATE_DOWN))
		{
			_camera.pitch(-multiplier);
		}
		if (Input::keyDown(IGCS_KEY_ROTATE_UP))
		{
			_camera.pitch(multiplier);
		}
		if (Input::keyDown(IGCS_KEY_ROTATE_RIGHT))
		{
			_camera.yaw(multiplier);
		}
		if (Input::keyDown(IGCS_KEY_ROTATE_LEFT))
		{
			_camera.yaw(-multiplier);
		}
		if (Input::keyDown(IGCS_KEY_TILT_LEFT))
		{
			_camera.roll(multiplier);
		}
		if (Input::keyDown(IGCS_KEY_TILT_RIGHT))
		{
			_camera.roll(-multiplier);
		}
	}


	// Initializes system. Will block till camera struct is found.
	void System::initialize()
	{
		Globals::instance().mainWindowHandle(Utils::findMainWindow(GetCurrentProcessId()));
		InputHooker::setInputHooks();
		DX11Hooker::initializeHook();
		Input::registerRawInput();
		GameSpecific::CameraManipulator::setImageAddress(_hostImageAddress);
		GameSpecific::InterceptorHelper::initializeAOBBlocks(_hostImageAddress, _hostImageSize, _aobBlocks);
		GameSpecific::InterceptorHelper::setCameraStructInterceptorHook(_aobBlocks);
		waitForCameraStructAddresses();		// blocks till camera is found.
		GameSpecific::InterceptorHelper::setPostCameraStructHooks(_aobBlocks, _hostImageAddress);

		// camera struct found, init our own camera object now and hook into game code which uses camera.
		_cameraStructFound = true;
		_camera.setPitch(INITIAL_PITCH_RADIANS);
		_camera.setRoll(INITIAL_ROLL_RADIANS);
		_camera.setYaw(INITIAL_YAW_RADIANS);

		//enable hotsampling
		GameSpecific::InterceptorHelper::enableHotsampling(_aobBlocks);
	}


	// Waits for the interceptor to pick up the camera struct address. Should only return if address is found 
	void System::waitForCameraStructAddresses()
	{
		OverlayConsole::instance().logLine("Waiting for camera struct interception...");
		while(!GameSpecific::CameraManipulator::isCameraFound())
		{
			handleUserInput();
			Sleep(100);
		}
		OverlayControl::addNotification("Camera found.");
		GameSpecific::CameraManipulator::displayCameraStructAddress();
	}
		

	void System::toggleInputBlockState(bool newValue)
	{
		if (Globals::instance().inputBlocked() == newValue)
		{
			// already in this state. Ignore
			return;
		}
		Globals::instance().inputBlocked(newValue);
		OverlayControl::addNotification(newValue ? "Input to game blocked" : "Input to game enabled");
	}


	void System::toggleCameraMovementLockState(bool newValue)
	{
		if (_cameraMovementLocked == newValue)
		{
			// already in this state. Ignore
			return;
		}
		_cameraMovementLocked = newValue;
		OverlayControl::addNotification(_cameraMovementLocked ? "Camera movement is locked" : "Camera movement is unlocked");
	}


	void System::displayCameraState()
	{
		OverlayControl::addNotification(g_cameraEnabled ? "Camera enabled" : "Camera disabled");
	}


	void System::displayGamePauseState()
	{
		OverlayControl::addNotification(g_gamePaused ? "Game paused" : "Game unpaused");
	}

	void System::displayGameHUDState()
	{
		OverlayControl::addNotification(hudstate ? "HUD On" : "HUD Off");
	}

	void System::displayframeskip()
	{
		std::string frames = std::to_string(Globals::instance().settings().frameskip);
		string framewording = wording + frames + millisecs;
		OverlayControl::addNotification(framewording);
	}
}