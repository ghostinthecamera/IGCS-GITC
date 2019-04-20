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
#include "MinHook.h"

namespace IGCS
{
	using namespace IGCS::GameSpecific;

	System::System()
	{
	}


	System::~System()
	{
	}


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

		// calculate new camera values. We have two cameras, but they might not be available both, so we have to test before we do anything. 
		DirectX::XMVECTOR newLookQuaternion = _camera.calculateLookQuaternion();
		DirectX::XMFLOAT3 currentCoords;
		DirectX::XMFLOAT3 newCoords;
		if (GameSpecific::CameraManipulator::isCameraFound())
		{
			currentCoords = GameSpecific::CameraManipulator::getCurrentCameraCoords();
			newCoords = _camera.calculateNewCoords(currentCoords, newLookQuaternion);
			GameSpecific::CameraManipulator::writeNewCameraValuesToGameData(newCoords, newLookQuaternion);
		}
	}


	void System::handleUserInput()
	{
		Globals::instance().gamePad().update();
		if (_applyHammerPrevention)
		{
			_applyHammerPrevention = false;
			// sleep main thread for 300ms so key repeat delay is simulated. 
			Sleep(300);
		}

		if (Input::isActionActivated(ActionType::ToggleOverlay))
		{
			OverlayControl::toggleOverlay();
			_applyHammerPrevention = true;
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
		if (Input::isActionActivated(ActionType::CameraEnable))
		{
			if (g_cameraEnabled)
			{
				// it's going to be disabled, make sure things are alright when we give it back to the host
				CameraManipulator::restoreOriginalValuesAfterCameraDisable();
				toggleCameraMovementLockState(false);
				//InterceptorHelper::toggleInGameDoFOff(_aobBlocks, false);		// switch dof back on
			}
			else
			{
				// it's going to be enabled, so cache the original values before we enable it so we can restore it afterwards
				CameraManipulator::cacheOriginalValuesBeforeCameraEnable();
				_camera.resetAngles();
				CameraManipulator::displayCameraStructAddress();
				toggleInputBlockState(true);
				//InterceptorHelper::toggleInGameDoFOff(_aobBlocks, Globals::instance().settings().disableInGameDofWhenCameraIsEnabled);
			}
			g_cameraEnabled = g_cameraEnabled == 0 ? (BYTE)1 : (BYTE)0;
			displayCameraState();
			_applyHammerPrevention = true;
		}
		if (Input::isActionActivated(ActionType::FovReset) && Globals::instance().keyboardMouseControlCamera())
		{
			CameraManipulator::resetFoV();
		}
		if (Input::isActionActivated(ActionType::FovDecrease) && Globals::instance().keyboardMouseControlCamera())
		{
			CameraManipulator::changeFoV(-Globals::instance().settings().fovChangeSpeed);
		}
		if (Input::isActionActivated(ActionType::FovIncrease) && Globals::instance().keyboardMouseControlCamera())
		{
			CameraManipulator::changeFoV(Globals::instance().settings().fovChangeSpeed);
		}
		//if (Input::isActionActivated(ActionType::Timestop))
		//{
		//	toggleTimestopState();
		//	_applyHammerPrevention = true;
		//}

		//if (Input::isActionActivated(ActionType::HudToggle))
		//{
		//	toggleHudRenderState();
		//	_applyHammerPrevention = true;
		//}

		if (!g_cameraEnabled)
		{
			// camera is disabled. We simply disable all input to the camera movement, by returning now.
			return;
		}
		if (Input::isActionActivated(ActionType::BlockInput))
		{
			toggleInputBlockState(!Globals::instance().inputBlocked());
			_applyHammerPrevention = true;
		}
		_camera.resetMovement();
		Settings& settings = Globals::instance().settings();
		if (Input::isActionActivated(ActionType::CameraLock))
		{
			toggleCameraMovementLockState(!_cameraMovementLocked);
			_applyHammerPrevention = true;
		}
		if (_cameraMovementLocked)
		{
			// no movement allowed, simply return
			return;
		}

		bool altPressed = Utils::altPressed();
		bool rcontrolPressed = Utils::keyDown(VK_RCONTROL);
		float multiplier = altPressed ? settings.fastMovementMultiplier : rcontrolPressed ? settings.slowMovementMultiplier : 1.0f;
		handleKeyboardCameraMovement(multiplier);
		handleMouseCameraMovement(multiplier);
		handleGamePadMovement(multiplier);
	}


	void System::handleGamePadMovement(float multiplierBase)
	{
		if(!Globals::instance().controllerControlsCamera())
		{
			return;
		}

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
			if (gamePad.isButtonPressed(IGCS_BUTTON_RESET_FOV))
			{
				CameraManipulator::resetFoV();
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
		if (!Globals::instance().keyboardMouseControlCamera())
		{
			return;
		}
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
		Input::resetMouseDeltas();
	}


	void System::handleKeyboardCameraMovement(float multiplier)
	{
		if (!Globals::instance().keyboardMouseControlCamera())
		{
			return;
		}
		if (Input::isActionActivated(ActionType::MoveForward, true))
		{
			_camera.moveForward(multiplier);
		}
		if (Input::isActionActivated(ActionType::MoveBackward, true))
		{
			_camera.moveForward(-multiplier);
		}
		if (Input::isActionActivated(ActionType::MoveRight, true))
		{
			_camera.moveRight(multiplier);
		}
		if (Input::isActionActivated(ActionType::MoveLeft, true))
		{
			_camera.moveRight(-multiplier);
		}
		if (Input::isActionActivated(ActionType::MoveUp, true))
		{
			_camera.moveUp(multiplier);
		}
		if (Input::isActionActivated(ActionType::MoveDown, true))
		{
			_camera.moveUp(-multiplier);
		}
		if (Input::isActionActivated(ActionType::RotateDown, true))
		{
			_camera.pitch(-multiplier);
		}
		if (Input::isActionActivated(ActionType::RotateUp, true))
		{
			_camera.pitch(multiplier);
		}
		if (Input::isActionActivated(ActionType::RotateRight, true))
		{
			_camera.yaw(multiplier);
		}
		if (Input::isActionActivated(ActionType::RotateLeft, true))
		{
			_camera.yaw(-multiplier);
		}
		if (Input::isActionActivated(ActionType::TiltLeft, true))
		{
			_camera.roll(multiplier);
		}
		if (Input::isActionActivated(ActionType::TiltRight, true))
		{
			_camera.roll(-multiplier);
		}
	}


	// Initializes system. Will block till camera struct is found.
	void System::initialize()
	{
		MH_Initialize();
		OverlayControl::init();
		Globals::instance().mainWindowHandle(Utils::findMainWindow(GetCurrentProcessId()));
		InputHooker::setInputHooks();
		Input::registerRawInput();
		DX11Hooker::initializeHook();

		GameSpecific::InterceptorHelper::initializeAOBBlocks(_hostImageAddress, _hostImageSize, _aobBlocks);
		GameSpecific::InterceptorHelper::setCameraStructInterceptorHook(_aobBlocks);
		waitForCameraStructAddresses();		// blocks till camera is found.
		GameSpecific::InterceptorHelper::setPostCameraStructHooks(_aobBlocks);

		// camera struct found, init our own camera object now and hook into game code which uses camera.
		_cameraStructFound = true;
		_camera.setPitch(INITIAL_PITCH_RADIANS);
		_camera.setRoll(INITIAL_ROLL_RADIANS);
		_camera.setYaw(INITIAL_YAW_RADIANS);

		// apply initial settings
		CameraManipulator::applySettingsToGameState();
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


	void System::displayCameraState()
	{
		OverlayControl::addNotification(g_cameraEnabled ? "Camera enabled" : "Camera disabled");
	}


	//void System::toggleTimestopState()
	//{
	//	_timeStopped = !_timeStopped;
	//	OverlayControl::addNotification(_timeStopped ? "Game paused" : "Game unpaused");
	//	CameraManipulator::setTimeStopValue(_timeStopped);
	//}

	//void System::toggleHudRenderState()
	//{
	//	_hudToggled = !_hudToggled;
	//	InterceptorHelper::toggleHud(_aobBlocks, _hudToggled);
	//}
}