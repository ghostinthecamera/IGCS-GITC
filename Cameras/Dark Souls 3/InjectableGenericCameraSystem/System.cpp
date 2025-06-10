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

#include "Camera.h"
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
#include "GameImageHooker.h"

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
				System::instance().updateFrame();
			}
		}
		else
		{
			if (D3DHook::instance().needsInitialization()) {
				D3DHook::instance().performQueuedInitialization();
			}

			if (RUN_IN_HOOKED_PRESENT)
			{
				// If D3D is enabled, we need to run the main loop in the hooked present function.
				// This is to ensure that we can manipulate the camera during rendering.
				MessageHandler::logLine("DirectX Hook Enabled: Running frame update in hooked present");
			}
			else
			{
				MessageHandler::logLine("DirectX Hook Enabled: Running frame update in IGCS thread");
				// If D3D is disabled, we can just run the main loop without any D3D hooks.
				while (Globals::instance().systemActive())
				{
					preciseSleep(FRAME_SLEEP);
					System::instance().updateFrame();
				}
			}
		}

	}


	// updates the data and camera for a frame 
	void System::updateFrame()
	{
		frameSkip();
		updateDeltaTime();
		timestop();
		CameraManipulator::cacheGameAddresses(_addressData);
		handleUserInput();
		cameraStateProcessor();
		IGCSConnector::fillDataForAddOn();
	}

	void System::timestop()
	{
		//if (auto& s = Globals::instance(); s.gamePaused() || s.sloMo())
		//{
		//	auto& aob = _aobBlocks[TIMESTOP_KEY_NOP];
		//	const bool isNOP = (*aob.absoluteAddress() == 0x90);
		//	//MessageHandler::logLine("First byte is %s", isNOP ? "NOP" : "NOT NOP");

		//	// If it's not a NOP but should be (game is paused or in slowmo), restore the state
		//	if (!isNOP) {
		//		aob.nopState = false;
		//		MessageHandler::logLine("Restoring time manipulation state - byte was unexpectedly changed");
		//		Utils::toggleNOPState(_aobBlocks[TIMESTOP_KEY_NOP], 6, true);

		//		if (s.gamePaused()) {
		//			CameraManipulator::setTimeStopValue(true);  // Pause the game
		//		}
		//		else if (s.sloMo()) {
		//			CameraManipulator::setSlowMo(s.settings().gameSpeed, true);  // Restore slowmo speed
		//		}
		//	}
		//}

		if (auto& s = Globals::instance(); s.gamePaused() || s.sloMo())
		{
			auto& aob = _aobBlocks[TIMESTOP_KEY_NOP];
			GameImageHooker::nopRange(aob.absoluteAddress(), 6);

			if (s.gamePaused())
			{
				CameraManipulator::setTimeStopValue(true);
			}
			else if (s.sloMo())
			{
				CameraManipulator::setSlowMo(s.settings().gameSpeed, true);
			}
		}

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
			}
			else
			{
				// it's going to be enabled, so cache the original values before we enable it so we can restore it afterwards
				CameraManipulator::cacheGameCameraData(_originalData);
				CameraManipulator::displayAddresses();
				InterceptorHelper::cameraSetup(_aobBlocks, true, _addressData);
				Camera::instance().prepareCamera();
			}
			g_cameraEnabled ^= 1;
			NamedPipeManager::instance().writeBinaryPayload(g_cameraEnabled, MessageType::CameraEnabled);
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

		if (Input::isActionActivated(ActionType::PlayerOnly))
		{
			togglePlayerOnly();
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
		handlePathManagerInput();
	}

	float System::calculateModifier()
	{
		const Settings& settings = Globals::instance().settings();

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
			Utils::onChange(Globals::instance().settings().togglePathVisualisation, toggleValue, [](auto t) {D3DHook::instance().setVisualization(t); });
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
				MessageHandler::addNotification("Path created: " + pathName);
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
				MessageHandler::addNotification("Playing path: " + selectedPath);
				pathManager.handlePlayPathMessage();
				
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
				//CameraPathManager::instance().setSelectedPath(pathName); //Should be done in call from UI
				MessageHandler::addNotification("Path \"" + pathName + "\" selected");
			}
			catch (const std::out_of_range&) {
				MessageHandler::addNotification("Error cycling to next path");
			}
		}


	}


	void System::handleGamePadMovement(float multiplier)
	{
		if(!Globals::instance().controllerControlsCamera())
		{
			return;
		}

		if (auto gamePad = Globals::instance().gamePad(); gamePad.isConnected())
		{
			Settings& settings = Globals::instance().settings();
			
			vec2 rightStickPosition = gamePad.getRStickPosition();
			Camera::instance().targetPitch(rightStickPosition.y * multiplier);
			Camera::instance().targetYaw(rightStickPosition.x * multiplier);


			vec2 leftStickPosition = gamePad.getLStickPosition();
			Camera::instance().moveUp((gamePad.getRTrigger() - gamePad.getLTrigger()) * multiplier);
			Camera::instance().moveForward(leftStickPosition.y * multiplier);
			Camera::instance().moveRight(leftStickPosition.x * multiplier);

			if (Input::isActionActivated((ActionType::TiltLeft)))
			{
				Camera::instance().targetRoll(-multiplier);
			}
			if (Input::isActionActivated((ActionType::TiltRight)))
			{
				Camera::instance().targetRoll(multiplier);
			}
			if (Input::isActionActivated((ActionType::FovReset)))
			{
				Camera::instance().setFoV(_originalData._fov, false);
			}
			if (Input::isActionActivated((ActionType::FovDecrease)))
			{
				Camera::instance().changeFOV(-Globals::instance().settings().fovChangeSpeed);
			}
			if (Input::isActionActivated((ActionType::FovIncrease)))
			{
				Camera::instance().changeFOV(Globals::instance().settings().fovChangeSpeed);
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
		bool leftButtonPressed = Input::isMouseButtonDown(0);
		bool rightButtonPressed = Input::isMouseButtonDown(1);
		bool noButtonPressed = !(leftButtonPressed || rightButtonPressed);
		if (abs(mouseDeltaY) > 1)
		{
			float yValue = (static_cast<float>(mouseDeltaY)* MOUSE_SPEED_CORRECTION* multiplier);
			if (noButtonPressed)
			{
				Camera::instance().targetPitch(-yValue);
			}
			else
			{
				if(leftButtonPressed)
				{
					// move up / down
					Camera::instance().moveUp(-yValue);
				}
				else
				{
					// forward/backwards
					Camera::instance().moveForward(-yValue, true);
				}
			}
		}
		if (abs(mouseDeltaX) > 1)
		{
			float xValue = static_cast<float>(mouseDeltaX)* MOUSE_SPEED_CORRECTION* multiplier;
			if (noButtonPressed)
			{
				Camera::instance().targetYaw(xValue);
			}
			else
			{
				// if both buttons are pressed: do tilt
				if(leftButtonPressed&&rightButtonPressed)
				{
					Camera::instance().targetRoll(xValue);
				}
				else
				{
					// always left/right
					Camera::instance().moveRight(xValue);
				}
			}
		}
		short mouseWheelDelta = Input::getMouseWheelDelta();
		if(abs(mouseWheelDelta) > 0)
		{
			Camera::instance().changeFOV(-(static_cast<float>(mouseWheelDelta) * Globals::instance().settings().fovChangeSpeed));
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
			Camera::instance().moveForward(multiplier, true);
		}
		if (Input::isActionActivated(ActionType::MoveBackward, true))
		{
			Camera::instance().moveForward(-multiplier, true);
		}
		if (Input::isActionActivated(ActionType::MoveRight, true))
		{
			Camera::instance().moveRight(multiplier);
		}
		if (Input::isActionActivated(ActionType::MoveLeft, true))
		{
			Camera::instance().moveRight(-multiplier);
		}
		if (Input::isActionActivated(ActionType::MoveUp, true))
		{
			Camera::instance().moveUp(multiplier);
		}
		if (Input::isActionActivated(ActionType::MoveDown, true))
		{
			Camera::instance().moveUp(-multiplier);
		}
		if (Input::isActionActivated(ActionType::RotateDown, true))
		{
			Camera::instance().targetPitch(-multiplier);
		}
		if (Input::isActionActivated(ActionType::RotateUp, true))
		{
			Camera::instance().targetPitch(multiplier);
		}
		if (Input::isActionActivated(ActionType::RotateRight, true))
		{
			Camera::instance().targetYaw(multiplier);
		}
		if (Input::isActionActivated(ActionType::RotateLeft, true))
		{
			Camera::instance().targetYaw(-multiplier);
		}
		if (Input::isActionActivated(ActionType::TiltLeft, true))
		{
			Camera::instance().targetRoll(-multiplier);
		}
		if (Input::isActionActivated(ActionType::TiltRight, true))
		{
			Camera::instance().targetRoll(multiplier);
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
		//Input::registerRawInput();

		_blocksInit = InterceptorHelper::initializeAOBBlocks(_hostImageAddress, _hostImageSize, _aobBlocks);
		//GameSpecific::InterceptorHelper::getAbsoluteAddresses(_aobBlocks); //return if needed
		_cameraStructInit = InterceptorHelper::setCameraStructInterceptorHook(_aobBlocks);
		waitForCameraStructAddresses();		// blocks till camera is found.
		_postCameraStructInit = InterceptorHelper::setPostCameraStructHooks(_aobBlocks);

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

		if (Globals::instance().settings().d3ddisabled == 1) {
			MessageHandler::logLine("DirectX hooking disabled");
		}
		else {
			if (!D3DHook::instance().initialize()) {
				MessageHandler::logError("Failed to initialize D3D hook");
			}
		}

	}


	// Waits for the interceptor to pick up the camera struct address. Should only return if address is found 
	void System::waitForCameraStructAddresses()
	{
		MessageHandler::logLine("Waiting for camera struct interception...");
		while(!GameSpecific::CameraManipulator::isCameraFound())
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

	void System::toggleBytePause(bool displayNotification)
	{
		const bool bytepaused = Globals::instance().toggleBytePaused();
		InterceptorHelper::togglePause(_aobBlocks, bytepaused);

		if (displayNotification)
		{
			MessageHandler::addNotification(bytepaused ? "Game paused (alternate)" : "Game unpaused (alternate)");
		}
	}


	void System::toggleGamePause(bool displayNotification)
	{
		auto& s = Globals::instance();

		const auto& gamePaused = s.gamePaused();
		const bool& slowMo = s.sloMo();

		if (!gamePaused && !slowMo)
		{
			Utils::toggleNOPState(_aobBlocks[TIMESTOP_KEY_NOP], 6, true);
			//Utils::toggleNOPState(_aobBlocks[TIMESTOP_CORRECT_KEY], 2, true);
			CameraManipulator::cachetimespeed();
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
			Utils::toggleNOPState(_aobBlocks[TIMESTOP_KEY_NOP], 6, false);
			//Utils::toggleNOPState(_aobBlocks[TIMESTOP_CORRECT_KEY], 2, false);
		}
		if (gamePaused && !slowMo)
		{
			//it's going to be unpaused
			Utils::toggleNOPState(_aobBlocks[TIMESTOP_KEY_NOP], 6, false);
			//Utils::toggleNOPState(_aobBlocks[TIMESTOP_CORRECT_KEY], 2, false);
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
			Utils::toggleNOPState(_aobBlocks[TIMESTOP_KEY_NOP], 6, true);
			//Utils::toggleNOPState(_aobBlocks[TIMESTOP_CORRECT_KEY], 2, true);
			CameraManipulator::cachetimespeed();
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
			Utils::toggleNOPState(_aobBlocks[TIMESTOP_KEY_NOP], 6, false);
			//Utils::toggleNOPState(_aobBlocks[TIMESTOP_CORRECT_KEY], 2, false);
		}
		if (slowMo && !gamePaused)
		{
			//returning to normal speed - no actions to take
			Utils::toggleNOPState(_aobBlocks[TIMESTOP_KEY_NOP], 6, false);
			//Utils::toggleNOPState(_aobBlocks[TIMESTOP_CORRECT_KEY], 2, false);
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

	// Game Specific

	void System::togglePlayerOnly()
	{
		if (_addressData.playerPointerAddress == nullptr)
		{
			MessageHandler::logError("Player Pointer not found...returning");
			return;
		}
		Globals::instance().togglePlayerOnly();
		g_playersonly = g_playersonly == 0 ? static_cast<uint8_t>(1) : static_cast<uint8_t>(0); //we set the value here so that the cmp  in our detour can be taken
		MessageHandler::addNotification(Globals::instance().playeronly() ? "PlayersOnly On" : "PlayersOnly Off");
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
			CameraManipulator::restoreCurrentCameraCoords(_igcscacheData.Coordinates);
		
		Camera::instance().moveRight(stepLeftRight / movementspeed,false);
		Camera::instance().moveUp((stepUpDown / movementspeed) / upmovementmultiplier, false);
		if (fovDegrees>0)
			CameraManipulator::restoreFOV(CameraManipulator::fovinRadians(fovDegrees));
		
		CameraManipulator::updateCameraDataInGameData();
		Camera::instance().resetMovement();
	}

}
