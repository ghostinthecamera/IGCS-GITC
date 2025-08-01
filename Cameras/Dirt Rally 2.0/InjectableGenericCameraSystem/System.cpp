////////////////////////////////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System
// Copyright(c) 2020, Frans Bouma
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
#include "MinHook.h"
#include "NamedPipeManager.h"
#include "MessageHandler.h"
#include "IGCSConnector.h"
#include "CameraPath.h"
#include "PathManager.h"
#include "Console.h"
#include "GameImageHooker.h"
#include "WindowHook.h"

namespace IGCS
{
	using namespace IGCS::GameSpecific;

	void preciseSleep(DWORD ms)
	{
		// grab frequency once per call (cheap enough), or stash in a static if you really care
		static LARGE_INTEGER freq = {};
		// Initialize frequency once
		if (freq.QuadPart == 0) {
			QueryPerformanceFrequency(&freq);
		}
		LARGE_INTEGER start;
		QueryPerformanceCounter(&start);                

		// how many ticks we need to wait
		LONGLONG waitTicks = (freq.QuadPart * ms) / 1000;

		LARGE_INTEGER now;
		do {
			Sleep(0);      // yield to other threads at this priority (already declared via stdafx.h)
			QueryPerformanceCounter(&now);
		} while ((now.QuadPart - start.QuadPart) < waitTicks);
	}

	System::System():
		_igcscacheData(),
		_originalData(),
		_hostImageAddress(nullptr),
		_hostImageSize(0),
		_lastFrameTime(),
		_frequency(),
		_useFixedDeltaTime(false),
		_fixedDeltaValue(0.0167f)
	{
	}


	System::~System()
	{
		D3DHook::instance().cleanUp();
		if (USE_WINDOWFOREGROUND_OVERRIDE)
		{
			WindowHook::cleanup();
		}
		
	}


	void System::start(LPBYTE hostBaseAddress, DWORD hostImageSize)
	{
		Globals::instance().systemActive(true);

		///
		Globals::instance().storeCurrentaobBlock(&_aobBlocks);
		Globals::instance().storeCurrentdeltaT(&_deltaTime);
		///
		
		_hostImageAddress = (LPBYTE)hostBaseAddress;
		_hostImageSize = hostImageSize;
		filesystem::path hostExeFilenameAndPath = Utils::obtainHostExeAndPath();
		_hostExeFilename = hostExeFilenameAndPath.stem();
		_hostExePath = hostExeFilenameAndPath.parent_path();
		Globals::instance().gamePad().setInvertLStickY(CONTROLLER_Y_INVERT);
		Globals::instance().gamePad().setInvertRStickY(CONTROLLER_Y_INVERT);

		// Initialize timing options
		_useFixedDeltaTime = false;  // Toggle this to use fixed or real delta time
		_fixedDeltaValue = 1.0f / 60.0f;  // Fixed delta time value at 60 FPS

		// Initialize the high-precision timer only for delta time calculation
		QueryPerformanceFrequency(&_frequency);
		QueryPerformanceCounter(&_lastFrameTime);

		initialize();    // will block till camera is found

		sysReady = true;

		mainLoop();
	}


	// Core loop of the system
	void System::mainLoop()
	{

		if (Globals::instance().settings().d3ddisabled)
		{
			MessageHandler::logLine("DirectXHook Disabled: Running frame update in IGCS thread");
			// If D3D is disabled, we can just run the main loop without any D3D hooks.
			while (Globals::instance().systemActive())
			{
				preciseSleep(FRAME_SLEEP);
				instance().updateFrame();
			}
		}
		else
		{
			if (D3DHook::instance().needsInitialization())
				D3DHook::instance().performQueuedInitialization();

			if (RUN_IN_HOOKED_PRESENT)
			{
				// If RUN_IN_HOOKED_PRESENT is true, we need to run the main loop in the hooked present function.
				// so we just log this and let the hooked present deal with it
				MessageHandler::logLine("DirectX Hook Enabled: Running frame update in hooked present");
			}
			else
			{
				MessageHandler::logLine("DirectX Hook Enabled: Running frame update in IGCS thread");
				CameraManipulator::displayAddresses();
				// If RUN_IN_HOOKED_PRESENT is false, we need to run the main loop in our own thread.
				// so we run updateframe while the system is active
				while (Globals::instance().systemActive())
				{
					preciseSleep(FRAME_SLEEP);
					instance().updateFrame();
				}
			}
		}

	}


	// updates the data and camera for a frame 
	void System::updateFrame()
	{
		frameSkip();
		updateDeltaTime();
		CameraManipulator::cacheGameAddresses(_addressData);
		validateAddresses(); //needed in dirt 2
		handleUserInput();
		cameraStateProcessor();
		IGCSConnector::fillDataForAddOn();
	}

	void System::validateAddresses()
	{
		isCameraStructValid = CameraManipulator::validateCameraMemory();
		isPlayerStructValid = CameraManipulator::validatePLayerPositionMemory();
	}

	void System::frameSkip()
	{
		if (Globals::instance().settings().d3ddisabled)
			return;

		if (!RUN_IN_HOOKED_PRESENT)
			return;

		if (!_skipActive)
			return;

		// ---------- VERSION A (frame counter) ----------
		// Simply count down our frames to skip. When we reach 0, we stop skipping.
		if (--_skipFramesLeft <= 0)
		{
			toggleGamePause(false);    // ► re‑pause
			_skipActive = false;
		}

		/* ---------- VERSION B (time counter) ----------
		_skipTimeLeft -= _deltaTime;
		if (_skipTimeLeft <= 0.0f)
		{
			toggleGamePause(false);    // ► re‑pause
			_skipActive = false;
		}
		*/
	}

	void System::cameraStateProcessor()
	{
		if (!g_cameraEnabled)
			return;

		static auto& pathManager = CameraPathManager::instance();
		static auto& Camera = Camera::instance();

		pathManager.updateOffsetforVisualisation();

		switch (pathManager._pathManagerState)
		{
		case PlayingPath:
			if (pathRun == false) 
				pathRun = true;

			//all checks performed in handling of message to start path playing. We wouldn't be here if the path wasn't valid.
			pathManager.playPath(getDeltaTime());
			pathManager.sendPathProgress(pathManager.getSelectedPathPtr()->getProgress());
			break;
		case GotoNode:
			//all checks performed in handling of message to go to this Node. We wouldn't be here if the path wasn't valid.
			pathManager.gotoNodeIndex(_deltaTime);
			break;
		case Scrubbing:
			pathRun = false;
			pathManager.scrubPath();
			pathManager.sendPathProgress(pathManager.getSelectedPathPtr()->getProgress());
			break;
		case Idle:
			Camera.updateCamera(_deltaTime);
			Camera.resetTargetMovement();
			break;
		default:
			break;
		}
	}

	float System::getDeltaTime() const
	{
		const auto deltaType = static_cast<DeltaType>(Globals::instance().settings().deltaType);

		return (deltaType == DeltaType::FPSLinked) ? _deltaTime : Globals::instance().settings().fixedDelta;
	}

	void System::updateDeltaTime()
	{
		if (_useFixedDeltaTime)
		{
			// Use a consistent delta time value for smoother camera interpolation
			_deltaTime = _fixedDeltaValue;
		}
		else
		{
			// Use real-time calculation (original behavior)
			LARGE_INTEGER currentFrameTime;
			QueryPerformanceCounter(&currentFrameTime);
			_deltaTime = static_cast<float>(currentFrameTime.QuadPart - _lastFrameTime.QuadPart) / static_cast<float>(_frequency.QuadPart);
			_lastFrameTime = currentFrameTime;
		}
	}
	
	bool System::checkIfGamehasFocus()
	{
		HWND currentForegroundWindow = GetForegroundWindow();
		return (currentForegroundWindow == Globals::instance().mainWindowHandle());
	}


	void System::handleUserInput()
	{
		if (_IGCSConnectorSessionActive || CameraPathManager::instance()._pathManagerState == GotoNode || _skipActive/* || _camPathManager._pathManagerState == PlayingPath*/)
		{
			return;
		}

		
		if(!checkIfGamehasFocus())
		{
			// our window isn't focused, exit
			return;
		}
		
		Globals::instance().gamePad().update();

		if (_applyHammerPrevention)
		{
			_applyHammerPrevention = false;
			// //sleep main thread for 200ms so key repeat delay is simulated. 
			//Sleep(200);
		}

		if (!_cameraStructFound)
		{
			// camera not found yet, can't proceed.
			return;
		}

		if (Input::isActionActivated(ActionType::CameraEnable))
		{
			if (g_cameraEnabled)
			{
				// it's going to be disabled, make sure things are alright when we give it back to the host
				CameraManipulator::restoreGameCameraData(_originalData);
				Camera::instance().setFoV(_originalData._fov, true);
				Globals::instance().cameraMovementLocked(false);
				InterceptorHelper::cameraSetup(_aobBlocks, false, _addressData);
				//NamedPipeManager::instance().writeBinaryPayload(0, MessageType::CameraEnabled);
			}
			else
			{
				// it's going to be enabled, so cache the original values before we enable it so we can restore it afterwards
				CameraManipulator::cacheGameCameraData(_originalData);
				CameraManipulator::displayAddresses();
				InterceptorHelper::cameraSetup(_aobBlocks, true, _addressData);
				Camera::instance().prepareCamera();
				//NamedPipeManager::instance().writeBinaryPayload(1, MessageType::CameraEnabled);
				// Game specific
				//NamedPipeManager::instance().writeFloatPayload(CameraManipulator::getMotionBlur(), MessageType::MotionBlurUpdate);
			}
			g_cameraEnabled ^= 1;
			NamedPipeManager::instance().writeBinaryPayload(g_cameraEnabled, MessageType::CameraEnabled);
			NamedPipeManager::instance().writeFloatPayload(CameraManipulator::getMotionBlur(), MessageType::MotionBlurUpdate);
			displayCameraState();
			_applyHammerPrevention = true;
		}

		if (Input::isActionActivated(ActionType::FovReset) && Globals::instance().keyboardMouseControlCamera())
		{
			Camera::instance().setFoV(_originalData._fov, false);
		}

		if (Input::isActionActivated(ActionType::FovDecrease) && Globals::instance().keyboardMouseControlCamera())
		{
			Camera::instance().changeFOV(-Globals::instance().settings().fovChangeSpeed);
		}

		if (Input::isActionActivated(ActionType::FovIncrease) && Globals::instance().keyboardMouseControlCamera())
		{
			Camera::instance().changeFOV(Globals::instance().settings().fovChangeSpeed);
		}

		if (Input::isActionActivated(ActionType::Timestop))
		{
			toggleGamePause();
			_applyHammerPrevention = true;
		}

		if (Input::isActionActivated(ActionType::HUDtoggle))
		{
			toggleHud();
			_applyHammerPrevention = true;
		}

		if (Input::isActionActivated(ActionType::SlowMo))
		{
			toggleSlowMo();
			_applyHammerPrevention = true;
		}

		if (Input::isActionActivated(ActionType::SkipFrames))
		{
			handleSkipFrames();
			_applyHammerPrevention = true;
		}

		if (Input::isActionActivated(ActionType::CycleDepthBuffers))
		{
			D3DHook::instance().cycleDepthBuffer();
			_applyHammerPrevention = true;
		}

		if (Input::isActionActivated(ActionType::ToggleDepthBuffer))
		{
			D3DHook::instance().toggleDepthBufferUsage();
			_applyHammerPrevention = true;
		}

		if (!g_cameraEnabled)
		{
			// camera is disabled. We simply disable all input to the camera movement, by returning now.
			return;
		}

		// Add a key to reset look-at offsets
		if (Input::isActionActivated(ActionType::ResetLookAtOffsets))
		{
			if (!Globals::instance().settings().lookAtEnabled || Camera::instance().getFixedCameraMount())
				return;

			Camera::instance().resetLookAtOffsets();
			MessageHandler::addNotification("Look-at offsets reset");
			_applyHammerPrevention = true;
		}

		if (Input::isActionActivated(ActionType::ToggleOffsetMode))
		{
			if (!Globals::instance().settings().lookAtEnabled || Camera::instance().getFixedCameraMount())
				return;

			Camera::instance().toggleOffsetMode();
			MessageHandler::addNotification(Camera::instance().getUseTargetOffsetMode() ?
				"Target offset mode enabled" : "Angle offset mode enabled");
			_applyHammerPrevention = true;
		}

		if (Input::isActionActivated(ActionType::ToggleHeightLockedMovement))
		{
			if (!Globals::instance().settings().lookAtEnabled || Camera::instance().getFixedCameraMount())
				return;

			Camera::instance().toggleHeightLockedMovement();
			MessageHandler::addNotification(Camera::instance().getHeightLockedMovement() ?
				"Height-locked movement enabled" : "Height-locked movement disabled");
			_applyHammerPrevention = true;
		}

		if (Input::isActionActivated(ActionType::ToggleFixedCameraMount))
		{
			Camera::instance().toggleFixedCameraMount();
			MessageHandler::addNotification(Camera::instance().getFixedCameraMount() ?
				"Fixed camera mount enabled" : "Fixed camera mount disabled");
			_applyHammerPrevention = true;
		}

		if (Input::isActionActivated(ActionType::ToggleLookAtVisualization))
		{
			auto& s = Camera::instance();
			// Only allow toggling when in target offset mode
			if (Globals::instance().settings().lookAtEnabled && s.getUseTargetOffsetMode())
			{
				const bool newValue = !s.getCameraLookAtVisualizationEnabled();
				s.setCameraLookAtVisualization(newValue);
				MessageHandler::addNotification(newValue ?
					"Free camera look-at visualization: ON" :
					"Free camera look-at visualization: OFF");
			}
			else
			{
				// Auto-disable if target offset mode is not enabled
				if (s.getCameraLookAtVisualizationEnabled())
				{
					s.setCameraLookAtVisualization(false);
					MessageHandler::addNotification("Look-at visualization disabled (requires target offset mode)");
				}
			}
		}


		handlePathManagerInput();

		if (Input::isActionActivated(ActionType::BlockInput))
		{
			toggleInputBlockState();
			_applyHammerPrevention = true;
		}


		if (Input::isActionActivated(ActionType::CameraLock)) 
		{
			toggleCameraMovementLockState();
			_applyHammerPrevention = true;
		}

		if (Globals::instance().cameraMovementLocked())
		{
			// no movement allowed, simply return
			return;
		}

		const float multiplier = calculateModifier();

		handleKeyboardCameraMovement(multiplier);
		handleMouseCameraMovement(multiplier);
		handleGamePadMovement(multiplier);
	}

	float System::calculateModifier()
	{
		Settings& settings = Globals::instance().settings();

		// Check for fast movement from ANY input device
		bool fastMovement = Utils::altPressed() || Input::isActionActivated(ActionType::GamepadFastModifier);

		// Check for slow movement from ANY input device  
		bool slowMovement = Utils::ctrlPressed() || Input::isActionActivated(ActionType::GamepadSlowModifier);

		// Determine base multiplier
		float baseMultiplier = fastMovement ? settings.fastMovementMultiplier :
			slowMovement ? settings.slowMovementMultiplier : 1.0f;

		// Apply FoV scaling
		float fovScalingFactor = Utils::clamp(abs(CameraManipulator::getCurrentFoV() / DEFAULT_FOV), 0.01f, 1.0f);
		return baseMultiplier * fovScalingFactor;
	}

	void System::handlePathManagerInput()
	{
		auto& pathManager = CameraPathManager::instance();
		if (!pathManager._pathManagerEnabled)
		{
			// Path manager is disabled, exit
			return;
		}

		if (_IGCSConnectorSessionActive || _skipActive)
			return;

		// Path Visualization Toggle
		if (Input::isActionActivated(ActionType::PathVisualizationToggle))
		{
			const bool toggleValue = !Globals::instance().settings().togglePathVisualisation;
			Utils::callOnChange(Globals::instance().settings().togglePathVisualisation, toggleValue, [](auto t) {D3DHook::instance().setVisualization(t); });
			NamedPipeManager::instance().writeBinaryPayload(D3DHook::instance().isVisualizationEnabled(), MessageType::UpdateVisualisation);
			MessageHandler::addNotification(toggleValue ? "Path visualization enabled" : "Path visualization disabled");
			_applyHammerPrevention = true;
		}

		if (Input::isActionActivated(ActionType::PathCreate))
		{
			// Check if we've reached the maximum number of paths
			if (pathManager.getPaths().size() >= PATH_MANAGER_MAXPATHS)
			{
				MessageHandler::addNotification("Maximum number of paths reached: " + PATH_MANAGER_MAXPATHS);
				return;
			}

			// Create path with default name

			if (const std::string pathName = pathManager.handleAddPathMessage(); !pathName.empty())
			{
				MessageHandler::addNotificationOnly("Path created: " + pathName);
			}
			else
			{
				MessageHandler::logError("Failed to create path");
			}

			_applyHammerPrevention = true;
		}


		if (pathManager.getPathCount() == 0)
			return; // Don't allow any other input if no paths are available


		// Path Play/Stop
		if (Input::isActionActivated(ActionType::PathPlayStop))
		{
			const auto selectedPath = pathManager.getSelectedPath();

			if (pathManager._pathManagerState == PlayingPath)
			{
				// If a path is playing, stop it
				pathManager.handleStopPathMessage();
				MessageHandler::addNotification("Path playback stopped");
			}
			else
			{
				// Otherwise, play the selected path - safety checks are done in the function
				pathManager.handlePlayPathMessage();
				MessageHandler::addNotification("Playing path: " + selectedPath);
			}
			_applyHammerPrevention = true;
		}

		if (pathManager._pathManagerState == PlayingPath)
			return; // Don't allow any other input while a path is playing

		// Add Node
		if (Input::isActionActivated(ActionType::PathAddNode))
		{
			const auto selectedPath = pathManager.getSelectedPath();

			if (pathManager.getPath(selectedPath)->GetNodeCount() == PATH_MAX_NODES)
			{
				MessageHandler::addNotification("Maximum number of nodes reached: (" + to_string(PATH_MAX_NODES) + ") for path: " + selectedPath);
				return;
			}

			const auto n = pathManager.handleAddNodeMessage();
			MessageHandler::addNotificationOnly("Node " + std::to_string(n) + " added to path: " + selectedPath);
			_applyHammerPrevention = true;
		}

		if (Input::isActionActivated(ActionType::PathDelete))
		{
			const auto selectedPath = pathManager.getSelectedPath();

			if (pathManager.handleDeletePathMessage())
			{
				MessageHandler::addNotificationOnly("Path deleted: " + selectedPath);
			}
			else
			{
				MessageHandler::logError("Failed to delete path: %s", selectedPath.c_str());
			}
			_applyHammerPrevention = true;
		}

		if (Input::isActionActivated(ActionType::PathDeleteLastNode))
		{
			const auto selectedPath = pathManager.getSelectedPath();

			pathManager.handleDeleteNodeMessage(255);
			MessageHandler::addNotificationOnly("Last node deleted from path: " + selectedPath);
			_applyHammerPrevention = true;
		}

		if (Input::isActionActivated(ActionType::PathCycle))
		{

			const std::string currentPath = pathManager.getSelectedPath();

			NamedPipeManager::instance().writeBinaryPayload(1, MessageType::CyclePath);
			// Get current path index

			int pathIndex;

			// Make sure the current path exists
			try {
				pathIndex = pathManager.getPathIndex(currentPath);
			}
			catch (const std::out_of_range&) {
				MessageHandler::logDebug("Path doesnt exist reverting to 0");
				// If current path doesn't exist, start from index 0
				pathIndex = 0;
			}

			// Handle wraparound
			if (pathIndex + 2 > pathManager.getPathCount())
				pathIndex = 0;
			else
				pathIndex++;

			// Safely get next path name
			try {
				std::string pathName = pathManager.getPathAtIndex(pathIndex);
				//CameraPathManager::instance().setSelectedPath(pathName); //Should be done in UI
				MessageHandler::addNotification("Path \"" + pathName + "\" selected");
			}
			catch (const std::out_of_range&) {
				MessageHandler::addNotification("Error cycling to next path");
			}
		}


	}


	void System::handleGamePadMovement(float multiplier)
	{
		// Early exit if controller doesn't control camera
		if (!Globals::instance().controllerControlsCamera())
		{
			return;
		}

		auto gamePad = Globals::instance().gamePad();
		if (!gamePad.isConnected())
		{
			return;
		}

		// Cache frequently used references
		Settings& settings = Globals::instance().settings();
		Camera& camera = Camera::instance();

		// Get all input states
		vec2 rightStickPosition = gamePad.getRStickPosition();
		vec2 leftStickPosition = gamePad.getLStickPosition();
		float triggerValue = (gamePad.getRTrigger() - gamePad.getLTrigger()) * multiplier;

		// ========== MODE-SPECIFIC CONTROLS ==========
		if (settings.lookAtEnabled && camera.getUseTargetOffsetMode())
		{
			// ===== LOOK-AT MODE: TARGET OFFSET =====
			// Right stick: Move look-at target in player's local coordinate system
			camera.addLookAtTargetRight(-rightStickPosition.x * multiplier * settings.movementSpeed);
			camera.addLookAtTargetForward(rightStickPosition.y * multiplier * settings.movementSpeed);

			// Triggers: Vertical movement (special case: LB + triggers = vertical target offset)
			if (gamePad.isButtonPressed(Gamepad::button_t::LB)) {
				camera.addLookAtTargetUp(triggerValue * settings.movementSpeed);
			}
			else {
				camera.moveUp(triggerValue);
			}

			// Roll: Adjust roll offset
			if (Input::isActionActivated(ActionType::TiltLeft))
				camera.addLookAtRollOffset(-multiplier * settings.rotationSpeed);
			if (Input::isActionActivated(ActionType::TiltRight))
				camera.addLookAtRollOffset(multiplier * settings.rotationSpeed);
		}
		else if (settings.lookAtEnabled && !camera.getUseTargetOffsetMode())
		{
			// ===== LOOK-AT MODE: ANGLE OFFSET =====
			// Right stick: Adjust offset angles
			camera.addLookAtPitchOffset(rightStickPosition.y * multiplier * settings.rotationSpeed);
			camera.addLookAtYawOffset(rightStickPosition.x * multiplier * settings.rotationSpeed);

			// Triggers: Camera up/down
			camera.moveUp(triggerValue);

			// Roll: Adjust roll offset
			if (Input::isActionActivated(ActionType::TiltLeft))
				camera.addLookAtRollOffset(-multiplier * settings.rotationSpeed);
			if (Input::isActionActivated(ActionType::TiltRight))
				camera.addLookAtRollOffset(multiplier * settings.rotationSpeed);
		}
		else
		{
			// ===== NORMAL MODE (DEFAULT) =====
			// Right stick: Direct camera rotation
			camera.targetPitch(rightStickPosition.y * multiplier);
			camera.targetYaw(rightStickPosition.x * multiplier);

			// Triggers: Camera up/down
			camera.moveUp(triggerValue);

			// Roll: Direct roll
			if (Input::isActionActivated(ActionType::TiltLeft))
				camera.targetRoll(-multiplier);
			if (Input::isActionActivated(ActionType::TiltRight))
				camera.targetRoll(multiplier);
		}

		// ========== COMMON CONTROLS (ALL MODES) ==========
		// Left stick: Always controls camera movement
		camera.moveForward(leftStickPosition.y * multiplier);
		camera.moveRight(leftStickPosition.x * multiplier);

		// FOV controls: Always available
		if (Input::isActionActivated(ActionType::FovReset))
		{
			camera.setFoV(_originalData._fov, false);
		}
		if (Input::isActionActivated(ActionType::FovDecrease))
		{
			camera.changeFOV(-settings.fovChangeSpeed);
		}
		if (Input::isActionActivated(ActionType::FovIncrease))
		{
			camera.changeFOV(settings.fovChangeSpeed);
		}
	}


	void System::handleMouseCameraMovement(float multiplier)
	{
		// Early exit if mouse doesn't control camera
		if (!Globals::instance().keyboardMouseControlCamera())
		{
			return;
		}

		// Cache frequently used references
		Settings& settings = Globals::instance().settings();
		Camera& camera = Camera::instance();

		// Get all input states
		long mouseDeltaX = Input::getMouseDeltaX();
		long mouseDeltaY = Input::getMouseDeltaY();
		short mouseWheelDelta = Input::getMouseWheelDelta();
		bool leftButtonPressed = Input::isMouseButtonDown(0);
		bool rightButtonPressed = Input::isMouseButtonDown(1);
		bool bothButtonsPressed = leftButtonPressed && rightButtonPressed;
		bool noButtonPressed = !(leftButtonPressed || rightButtonPressed);

		// Calculate scaled values
		float xValue = static_cast<float>(mouseDeltaX) * MOUSE_SPEED_CORRECTION * multiplier;
		float yValue = static_cast<float>(mouseDeltaY) * MOUSE_SPEED_CORRECTION * multiplier;

		// ========== BUTTON-BASED MOVEMENT (COMMON TO ALL MODES) ==========
		// These controls work the same regardless of camera mode

		if (abs(mouseDeltaY) > 1 && !noButtonPressed)
		{
			if (leftButtonPressed && !bothButtonsPressed)
			{
				// Left button: Camera up/down
				camera.moveUp(-yValue);
			}
			else if (rightButtonPressed && !bothButtonsPressed)
			{
				// Right button: Camera forward/backward
				camera.moveForward(-yValue, true);
			}
		}

		if (abs(mouseDeltaX) > 1 && !noButtonPressed && !bothButtonsPressed)
		{
			// Single button pressed: Camera left/right
			camera.moveRight(xValue);
		}

		// ========== MODE-SPECIFIC CONTROLS ==========
		// No-button controls and special cases that vary by mode

		if (settings.lookAtEnabled && camera.getUseTargetOffsetMode())
		{
			// ===== LOOK-AT MODE: TARGET OFFSET =====

			// No buttons + mouse Y: Move target forward/backward
			if (abs(mouseDeltaY) > 1 && noButtonPressed)
			{
				camera.addLookAtTargetForward(-yValue * settings.movementSpeed);
			}

			// No buttons + mouse X: Move target left/right
			if (abs(mouseDeltaX) > 1 && noButtonPressed)
			{
				camera.addLookAtTargetRight(-xValue * settings.movementSpeed);
			}

			// Both buttons + mouse X: Roll offset
			if (abs(mouseDeltaX) > 1 && bothButtonsPressed)
			{
				camera.addLookAtRollOffset(xValue * settings.rotationSpeed);
			}

			// Mouse wheel: Vertical target offset
			if (abs(mouseWheelDelta) > 0)
			{
				camera.addLookAtTargetUp(static_cast<float>(mouseWheelDelta) * settings.movementSpeed);
			}
		}
		else if (settings.lookAtEnabled && !camera.getUseTargetOffsetMode())
		{
			// ===== LOOK-AT MODE: ANGLE OFFSET =====

			// No buttons + mouse Y: Pitch offset
			if (abs(mouseDeltaY) > 1 && noButtonPressed)
			{
				camera.addLookAtPitchOffset(-yValue * settings.rotationSpeed);
			}

			// No buttons + mouse X: Yaw offset
			if (abs(mouseDeltaX) > 1 && noButtonPressed)
			{
				camera.addLookAtYawOffset(xValue * settings.rotationSpeed);
			}

			// Both buttons + mouse X: Roll offset
			if (abs(mouseDeltaX) > 1 && bothButtonsPressed)
			{
				camera.addLookAtRollOffset(xValue * settings.rotationSpeed);
			}

			// Mouse wheel: FOV control
			if (abs(mouseWheelDelta) > 0)
			{
				camera.changeFOV(-(static_cast<float>(mouseWheelDelta) * settings.fovChangeSpeed));
			}
		}
		else
		{
			// ===== NORMAL MODE (DEFAULT) =====

			// No buttons + mouse Y: Direct pitch
			if (abs(mouseDeltaY) > 1 && noButtonPressed)
			{
				camera.targetPitch(-yValue);
			}

			// No buttons + mouse X: Direct yaw
			if (abs(mouseDeltaX) > 1 && noButtonPressed)
			{
				camera.targetYaw(xValue);
			}

			// Both buttons + mouse X: Direct roll
			if (abs(mouseDeltaX) > 1 && bothButtonsPressed)
			{
				camera.targetRoll(xValue);
			}

			// Mouse wheel: FOV control
			if (abs(mouseWheelDelta) > 0)
			{
				camera.changeFOV(-(static_cast<float>(mouseWheelDelta) * settings.fovChangeSpeed));
			}
		}

		// Reset mouse deltas for next frame
		Input::resetMouseDeltas();
	}


	void System::handleKeyboardCameraMovement(float multiplier)
	{
		// Early exit if keyboard doesn't control camera
		if (!Globals::instance().keyboardMouseControlCamera())
		{
			return;
		}

		// Cache frequently used references
		Settings& settings = Globals::instance().settings();
		Camera& camera = Camera::instance();

		// ========== MOVEMENT CONTROLS (COMMON TO ALL MODES) ==========
		// Basic movement works the same in all camera modes
		if (Input::isActionActivated(ActionType::MoveForward, true))
			camera.moveForward(multiplier, true);

		if (Input::isActionActivated(ActionType::MoveBackward, true))
			camera.moveForward(-multiplier, true);

		if (Input::isActionActivated(ActionType::MoveLeft, true))
			camera.moveRight(-multiplier);

		if (Input::isActionActivated(ActionType::MoveRight, true))
			camera.moveRight(multiplier);

		if (Input::isActionActivated(ActionType::MoveUp, true))
			camera.moveUp(multiplier);

		if (Input::isActionActivated(ActionType::MoveDown, true))
			camera.moveUp(-multiplier);

		// ========== MODE-SPECIFIC ROTATION CONTROLS ==========

		if (settings.lookAtEnabled && camera.getUseTargetOffsetMode())
		{
			// ===== LOOK-AT MODE: TARGET OFFSET =====

			// Rotate keys move the target in local space
			if (Input::isActionActivated(ActionType::RotateUp, true))
				camera.addLookAtTargetForward(multiplier * settings.movementSpeed);
			if (Input::isActionActivated(ActionType::RotateDown, true))
				camera.addLookAtTargetForward(-multiplier * settings.movementSpeed);
			if (Input::isActionActivated(ActionType::RotateRight, true))
				camera.addLookAtTargetRight(-multiplier * settings.movementSpeed);
			if (Input::isActionActivated(ActionType::RotateLeft, true))
				camera.addLookAtTargetRight(multiplier * settings.movementSpeed);

			// Tilt keys adjust roll offset
			if (Input::isActionActivated(ActionType::TiltLeft, true))
				camera.addLookAtRollOffset(-multiplier * settings.rotationSpeed);
			if (Input::isActionActivated(ActionType::TiltRight, true))
				camera.addLookAtRollOffset(multiplier * settings.rotationSpeed);

			if (Input::isActionActivated(ActionType::MoveUpTarget))
				camera.addLookAtTargetUp(multiplier * settings.movementSpeed);
			if (Input::isActionActivated(ActionType::MoveDownTarget))
				camera.addLookAtTargetUp(-multiplier * settings.movementSpeed);
		}
		else if (settings.lookAtEnabled && !camera.getUseTargetOffsetMode())
		{
			// ===== LOOK-AT MODE: ANGLE OFFSET =====

			// Rotate keys adjust offset angles
			if (Input::isActionActivated(ActionType::RotateUp, true))
				camera.addLookAtPitchOffset(multiplier * settings.rotationSpeed);
			if (Input::isActionActivated(ActionType::RotateDown, true))
				camera.addLookAtPitchOffset(-multiplier * settings.rotationSpeed);
			if (Input::isActionActivated(ActionType::RotateRight, true))
				camera.addLookAtYawOffset(multiplier * settings.rotationSpeed);
			if (Input::isActionActivated(ActionType::RotateLeft, true))
				camera.addLookAtYawOffset(-multiplier * settings.rotationSpeed);

			// Tilt keys adjust roll offset
			if (Input::isActionActivated(ActionType::TiltLeft, true))
				camera.addLookAtRollOffset(-multiplier * settings.rotationSpeed);
			if (Input::isActionActivated(ActionType::TiltRight, true))
				camera.addLookAtRollOffset(multiplier * settings.rotationSpeed);
		}
		else
		{
			// ===== NORMAL MODE (DEFAULT) =====

			// Rotate keys control direct rotation
			if (Input::isActionActivated(ActionType::RotateUp, true))
				camera.targetPitch(multiplier);
			if (Input::isActionActivated(ActionType::RotateDown, true))
				camera.targetPitch(-multiplier);
			if (Input::isActionActivated(ActionType::RotateRight, true))
				camera.targetYaw(multiplier);
			if (Input::isActionActivated(ActionType::RotateLeft, true))
				camera.targetYaw(-multiplier);

			// Tilt keys control direct roll
			if (Input::isActionActivated(ActionType::TiltLeft, true))
				camera.targetRoll(-multiplier);
			if (Input::isActionActivated(ActionType::TiltRight, true))
				camera.targetRoll(multiplier);
		}
	}


	void System::checkDXHookRequired()
	{
		// Check if D3D hooking is disabled via shared memory
		wchar_t memName[64];
		DWORD processId = GetCurrentProcessId();
		swprintf_s(memName, L"IGCS_D3D_DISABLED_%lu", processId);

		if (HANDLE hMapFile = OpenFileMapping(FILE_MAP_READ, FALSE, memName); hMapFile != nullptr) {
			// Shared memory exists - D3D hooking should be disabled
			//MessageHandler::logLine("DirectX hooking disabled via shared memory flag");
			// Set the setting value to ensure consistency
			Globals::instance().settings().d3ddisabled = 1;
			CloseHandle(hMapFile);
		}
	}

	// Initializes system. Will block till camera struct is found.
	void System::initialize()
	{
		MH_Initialize();

		checkDXHookRequired();

		// first grab the window handle
		Globals::instance().mainWindowHandle(Utils::findMainWindow(GetCurrentProcessId()));
		NamedPipeManager::instance().connectDllToClient();
		NamedPipeManager::instance().startListening();
		InputHooker::setInputHooks();
		Input::registerRawInput();

		//Initialise hooks (D3D, window etc)
		initializeHooks();


		blocksInit = InterceptorHelper::initializeAOBBlocks(_hostImageAddress, _hostImageSize, _aobBlocks);
		InterceptorHelper::getAbsoluteAddresses(_aobBlocks); //return if needed
		cameraStructInit = InterceptorHelper::setCameraStructInterceptorHook(_aobBlocks);
		waitForCameraStructAddresses();		// blocks till camera is found.
		postCameraStructInit = InterceptorHelper::setPostCameraStructHooks(_aobBlocks);

		// camera struct found, init our own camera object now and hook into game code which uses camera.
		_cameraStructFound = true;
		Camera::instance().resetAngles();

		// apply initial settings
		CameraManipulator::applySettingsToGameState();

		//connect to reshade
		IGCSConnector::setupIGCSConnectorConnection();

		//display address for debug:
		CameraManipulator::displayAddresses();

		//apply any code changes now
		InterceptorHelper::toolsInit(_aobBlocks);

		_deltaTime = 0.0f;

	}

	// Initialise hooks here
	void System::initializeHooks()
	{
		if (USE_WINDOWFOREGROUND_OVERRIDE)
		{
			if (!WindowHook::initialize()) {
				MessageHandler::logError("Failed to initialize window hook");
			}
		}

		// Initialize hooks here
		if (Globals::instance().settings().d3ddisabled)
		{
			MessageHandler::logLine("DirectXHook Disabled: No hooks to initialize");
		}
		else
		{
			if (!D3DHook::instance().initialize())
				MessageHandler::logError("Failed to initialize D3D hook");
		}

	}


	// Waits for the interceptor to pick up the camera struct address. Should only return if address is found 
	void System::waitForCameraStructAddresses()
	{
		MessageHandler::logLine("Waiting for camera struct interception...");
		while(!CameraManipulator::isCameraFound())
		{
			handleUserInput();
			Sleep(100);
		}
		MessageHandler::addNotification("Camera found.");
	}
		

	void System::toggleCameraMovementLockState()
	{
		MessageHandler::addNotification(Globals::instance().toggleCameraMovementLocked() ? "Camera movement is locked" : "Camera movement is unlocked");
	}


	void System::toggleInputBlockState()
	{
		MessageHandler::addNotification(Globals::instance().toggleInputBlocked() ? "Input to game blocked" : "Input to game enabled");
	}


	void System::toggleHud()
	{
		bool hudVisible = Globals::instance().toggleHudVisible();
		InterceptorHelper::toggleHud(_aobBlocks, hudVisible);
		MessageHandler::addNotification(hudVisible ? "HUD visible" : "HUD hidden");
	}


	void System::handleSkipFrames()
	{

		// Only valid when the game is currently paused and not already in slo‑mo
		if (!Globals::instance().gamePaused() || Globals::instance().sloMo())
			return;

		if (Globals::instance().settings().d3ddisabled)
		{
			// first unpause
			toggleGamePause(false);
			preciseSleep(32);
			toggleGamePause(false);
		}
		else
		{
			if (RUN_IN_HOOKED_PRESENT)
			{
				// ► un‑pause
				toggleGamePause(false);

				// VERSION A – frame based
				_skipActive = true;
				_skipFramesLeft = 4; // skip n rendered frames

				// VERSION B – time based
				//_skipActive  = true;
				//_skipTimeLeft = 0.033f;       // ~33 ms at 60 FPS
			}
			else
			{
				// first unpause
				toggleGamePause(false);
				preciseSleep(32);
				toggleGamePause(false);
			}
		}

	}


	void System::toggleGamePause(bool displayNotification)
	{
		auto& s = Globals::instance();

		const auto& gamePaused = s.gamePaused();
		const bool& slowMo = s.sloMo();

		if (!gamePaused && !slowMo)
		{
			CameraManipulator::cachetimespeed(); //its going to be paused
			//add code nop here = true
			Utils::toggleNOPState(_aobBlocks[TIMESCALE_NOP], 8, true);
		}
		if (!gamePaused && slowMo)
		{
			//it's going to be paused while in slowmotion so set slowmo flag to false
			Globals::instance().sloMo(false); //set the slowmo flag to false
		}
		if (gamePaused && slowMo)
		{
			//it's going to be unpaused but slowmoflag true so set slotmo flag to false
			Globals::instance().sloMo(false); //set the slowmo flag to false
			//add code nop here = false
			Utils::toggleNOPState(_aobBlocks[TIMESCALE_NOP], 8, false);
		}
		if (gamePaused && !slowMo)
		{
			//it's going to be unpaused
			//add code nop here = false
			Utils::toggleNOPState(_aobBlocks[TIMESCALE_NOP], 8, false);
		}

		s.toggleGamePaused();
		CameraManipulator::setTimeStopValue(gamePaused);
		

		if (displayNotification)
		{
			MessageHandler::addNotification(gamePaused ? "Game paused" : "Game unpaused");
		}
	}

	void System::toggleSlowMo(bool displaynotification)
	{
		auto& s = Globals::instance();

		const auto& gamePaused = s.gamePaused();
		const bool& slowMo = s.sloMo();

		if (!slowMo && !gamePaused)
		{
			CameraManipulator::cachetimespeed();
			// add code nop here = true
			Utils::toggleNOPState(_aobBlocks[TIMESCALE_NOP], 8, true);
		}
		if (!slowMo && gamePaused)
		{
			//slowmotion to be enabled while in pause, disable pause
			Globals::instance().gamePaused(false);
		}
		if (slowMo && gamePaused)
		{
			//slowmotion to be disabled while in pause, disable pause
			Globals::instance().gamePaused(false);
			// add code nop here = false
			Utils::toggleNOPState(_aobBlocks[TIMESCALE_NOP], 8, false);
		}
		if (slowMo && !gamePaused)
		{
			//returning to normal speed - no actions to take
			//add code nop here = false
			Utils::toggleNOPState(_aobBlocks[TIMESCALE_NOP], 8, false);
		}

		s.toggleSlowMo();
		CameraManipulator::setSlowMo(s.settings().gameSpeed, slowMo);
		

		if (displaynotification)
		{
			MessageHandler::addNotification(slowMo ? "SlowMo" : "Resume");
		}

	}

	void System::toggledepthBufferUsage()
	{
		D3DHook::instance().toggleDepthBufferUsage();
		if (D3DHook::instance().isUsingDepthBuffer())
			MessageHandler::addNotification("Using Depth Buffer");
		else
			MessageHandler::addNotification("Depth Buffer disabled");
	}

	void System::displayCameraState()
	{
		MessageHandler::addNotification(g_cameraEnabled ? "Camera enabled" : "Camera disabled");
	}


	/// <summary>
	/// IGCSDOF
	/// </summary>
	/// <param name="type"></param>
	/// <returns></returns>

	uint8_t System::startIGCSsession(uint8_t type)
	{
		if (!g_cameraEnabled)
			return IGCScamenabled;
		
		if (_IGCSConnectorSessionActive)
			return IGCSsessionactive;
		
		setIGCSsession(true, type);
		CameraManipulator::cacheigcsData( _igcscacheData);

		return IGCSsuccess;
	}


	void System::clearMovementData()
	{
		Camera::instance().resetMovement();
	}


	void System::endIGCSsession()
	{
		if (!_IGCSConnectorSessionActive)
			return;
		

		setIGCSsession(false, DEFAULT_IGCS_TYPE);
		CameraManipulator::restoreigcsData(_igcscacheData);
	}


	void System::stepCameraforIGCSSession(float stepAngle)
	{
		if (!_IGCSConnectorSessionActive)
			return;
		
		Camera::instance().targetYaw(stepAngle);
	}


	void System::stepCameraforIGCSSession(float stepLeftRight, float stepUpDown, float fovDegrees, bool fromStartPosition)
	{
		if (!_IGCSConnectorSessionActive)
			return;

		const auto s = Globals::instance().settings();

		const float movementspeed = s.movementSpeed;
		const float upmovementmultiplier = s.movementUpMultiplier;
		if (fromStartPosition)
			Camera::instance().setInternalPosition(_igcscacheData.Coordinates);
		
		Camera::instance().moveRight(stepLeftRight / movementspeed,false);
		Camera::instance().moveUp((stepUpDown / movementspeed) / upmovementmultiplier, false);
		if (fovDegrees>0)
			CameraManipulator::restoreFOV(CameraManipulator::fovinRadians(fovDegrees));
		
		CameraManipulator::updateCameraDataInGameData();
		Camera::instance().resetMovement();
	}

}
