#include "stdafx.h"
#include "PathManager.h"
#include "Globals.h"
#include "MessageHandler.h"
#include "SerialiseCameraPaths.h"
#include "NamedPipeManager.h"
#include "Defaults.h"
#include "CameraManipulator.h"
#include "PathUtils.h"
#include "Console.h"
#include "BinaryPathStructs.h"

namespace IGCS
{
	// Function pointers
	static CameraPathManager::IgcsConnector_addCameraPath _addCameraPath = nullptr;
	static CameraPathManager::IgcsConnector_appendStateSnapshotToPath _appendStateSnapshotToPath = nullptr;
	static CameraPathManager::IgcsConnector_appendStateSnapshotAfterSnapshotOnPath _appendStateSnapshotAfterSnapshotOnPath = nullptr;
	static CameraPathManager::IgcsConnector_insertStateSnapshotBeforeSnapshotOnPath _insertStateSnapshotBeforeSnapshotOnPath = nullptr;
	static CameraPathManager::IgcsConnector_removeStateSnapshotFromPath _removeStateSnapshotFromPath = nullptr;
	static CameraPathManager::IgcsConnector_removeCameraPath _removeCameraPath = nullptr;
	static CameraPathManager::IgcsConnector_clearPaths _clearPaths = nullptr;
	static CameraPathManager::IgcsConnector_setReshadeState _setReshadeState = nullptr;
	static CameraPathManager::IgcsConnector_setReshadeStateInterpolated _setReshadeStateInterpolated = nullptr;
	static CameraPathManager::IgcsConnector_updateStateSnapshotOnPath _updateStateSnapshotOnPath = nullptr;

	using namespace DirectX;

	CameraPathManager::CameraPathManager() :
		nodePayload{ .nodeIndex= static_cast<uint8_t>(0), .currentQ= XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f), .currentP=
			XMFLOAT3(0.0f, 0.0f, 0.0f), .currentfov= 0.0f, .interpolating= false, .elapsedTime= 0.0f, .totalDuration=
			0.0f
		},
		_selectedPath{}
	{
	}

	CameraPathManager::~CameraPathManager()
	{
		_paths.clear();
	}

	CameraPath* CameraPathManager::getPath(const std::string& pathName) 
	{
		if (_paths.contains(pathName)) {
			return &_paths[pathName];
		}
		MessageHandler::logError("CameraPathManager::getPath::Path not found - returning nullptr");
		return nullptr;
	}

	void CameraPathManager::playPath(const float deltaTime)
	{
		_paths[_selectedPath].interpolateNodes(deltaTime);
	}

	void CameraPathManager::handlePlayPathMessage()
	{
		if (_pathManagerState != Idle){
			MessageHandler::logDebug("PathManager::handlePlayPathMessage:Path Controller not idle - exiting playPath");
			return;
		}

		if (!_paths.contains(_selectedPath)){
			MessageHandler::logError("CameraPathManager::handlePlayPathMessage::Path not found - exiting playPath");
			return;
		}

		//MessageHandler::logDebug("NamedPipeMessageManager::handlePlayPathMessage::%s loaded to play", _selectedPath.c_str());

		_paths[_selectedPath].resetPath();
		_paths[_selectedPath].setTotalDuration(static_cast<float>(Globals::instance().settings().pathDurationSetting));

		// Handle settings
		if (Globals::instance().settings().waitBeforePlaying)
			Sleep(3000);
		if (Globals::instance().settings().unpauseOnPlay && Globals::instance().gamePaused())
			System::instance().toggleGamePause(false);

		//Set state
		System::instance().pathRun = false;
		updatePMState(PlayingPath);
		MessageHandler::logDebug("CameraPathManager::handlePlayPathMessage::Path Playback Started");
	}

	void CameraPathManager::setSelectedPath(uint8_t buffer[], const DWORD bytesRead)
	{
		// Check if there is at least one byte delivered to us
		if (bytesRead < 1)
		{
			MessageHandler::logError("CameraPathManager::setSelectedPath::Corrupted byteArray");
			return;
		}

		if (const std::string pathName = Utils::stringFromBytes(buffer, bytesRead, 2); !pathName.empty())
		{
			if (_paths.contains(pathName))
			{
				_selectedPath.clear();
				_selectedPath = pathName;
			}
		}
		else
		{
			MessageHandler::logError("CameraPathManager::setSelectedPath::Path name is empty, not set");
		}
	}

	void CameraPathManager::setPathManagerStatus(uint8_t buffer[], const DWORD bytesRead)
	{
		if (bytesRead < 1)
		{
			MessageHandler::logError("CameraPathManager::setPathManagerStatus::Corrupted byteArray");
			return;
		}
		const auto state = static_cast<PathManager>(buffer[2]);
		_pathManagerEnabled = state;
	}

	void CameraPathManager::setSelectedNodeIndex(uint8_t buffer[], const DWORD bytesRead)
	{
		// Check if there is at least one byte delivered to us
		if (bytesRead < 1)
		{
			MessageHandler::logError("CameraPathManager::setSelectedNodeIndex::Corrupted byteArray");
			return;
		}

		const uint8_t nodeIndex = buffer[2]; // Read the node index from the byte array
		if (nodeIndex > _paths.at(_selectedPath).getNodeSize())
		{
			MessageHandler::logDebug("CameraPathManager::setSelectedNodeIndex::node index out of bounds for given path");
			return;
		}

		if (_paths.contains(_selectedPath))
		{
			_selectedNodeIndex = nodeIndex;
		}
	}

	void CameraPathManager::sendSelectedPathUpdate(const std::string& pathName) const
	{
		if (pathName.empty())
		{
			MessageHandler::logError("CameraPathManager::sendSelectedPathUpdate::Path name is empty, no message sent");
			return;
		}
		if (!_paths.contains(pathName))
		{
			MessageHandler::logError("CameraPathManager::sendSelectedPathUpdate::Path not found, no message sent");
			return;
		}

		NamedPipeManager::instance().writeTextPayload(pathName, MessageType::UpdateSelectedPath);
	}

	void CameraPathManager::setSelectedPath(const string& pathName)
	{
		if (_paths.contains(pathName))
		{
			_selectedPath.clear();
			_selectedPath = pathName;
		}
		else
		{
			MessageHandler::logError("CameraPathManager::setSelectedPath::Path name is empty, not set");
		}
	}

	void CameraPathManager::handleStopPathMessage()
	{
		switch (_pathManagerState)
		{
		case Idle:
			MessageHandler::logDebug("CameraPathManager::handleStopPathMessage::Controller already Idle. Returning");
			break;
		case PlayingPath:
			updatePMState(Idle);
			_paths.at(_selectedPath).interpolateNodes(System::instance().getDT());
			(_pathManagerState == Idle) ?
				MessageHandler::logDebug("CameraPathManager::handleStopPathMessage::Controller state set to Idle") :
				MessageHandler::logError("CameraPathManager::handleStopPathMessage::Controller state change unsuccessful");
			break;
		case GotoNode:
			MessageHandler::logDebug("CameraPathManager::handleStopPathMessage::gotoNodeIndex currently playing. Returning");
			break;
		default:
			break;
		}
	}

	bool CameraPathManager::createPath(const std::string& pathName) 
	{
		try
		{
			_paths[pathName] = CameraPath(pathName);
			_paths[pathName].resetPath();

			// Create equivalent path in IgcsConnector
			if (reshadeAddonisConnected())
				addReshadePath();

			// Update path indices
			rebuildPathIndices();

			MessageHandler::logLine("Path created: %s ",pathName.c_str());

			return true;
		}
		catch (const std::exception&)
		{
			MessageHandler::logError("CameraPathManager::createPath::Failed to create path: %s", pathName.c_str());
			return false;
		}

	}

	bool CameraPathManager::handleDeletePathMessage(const std::string& pathName) {
		if (_paths.contains(pathName)) {
			_paths.erase(_paths.find(pathName)); // Remove the path from the map

			// remove equivalent path in IgcsConnector
			if (reshadeAddonisConnected())
				removeReshadePath(pathName);

			// Update path indices
			rebuildPathIndices();

			//MessageHandler::logDebug("Path deleted: %s", pathName.c_str());
			sendAllCameraPathsCombined();
			return true;
		}
		MessageHandler::logDebug("Path not found: %s", pathName.c_str());
		return false;
	}

	bool CameraPathManager::handleDeletePathMessage() {
		if (_paths.contains(_selectedPath)) {
			_paths.erase(_paths.find(_selectedPath)); // Remove the path from the map

			// remove equivalent path in IgcsConnector
			if (reshadeAddonisConnected())
				removeReshadePath(_selectedPath);

			// Update path indices
			rebuildPathIndices();

			MessageHandler::logLine("Path deleted: %s", _selectedPath.c_str());
			sendAllCameraPathsCombined();
			return true;
		}
		MessageHandler::logError("Path not found: %s", _selectedPath.c_str());
		return false;
	}

	uint8_t CameraPathManager::handleAddNodeMessage()
	{
		
		if (_paths.contains(_selectedPath) && !_selectedPath.empty())
		{
			const auto n = _paths.at(_selectedPath).addNode();
			
			sendAllCameraPathsCombined();
			
			//MessageHandler::logDebug("CameraPathManager::handleAddNodeMessage::%s node added", _selectedPath.c_str());
			//MessageHandler::logLine("Node %zu added to path: %s", n, _selectedPath.c_str());
			return n;
		}
		MessageHandler::logError("CameraPathManager::handleAddNodeMessage::%s not found in array", _selectedPath.c_str());
		return 0;
	}

	void CameraPathManager::handleInsertNodeBeforeMessage(uint8_t byteArray[], const DWORD arrayLength)
	{
		// Check if there is at least one byte delivered to us
		if (arrayLength < 1)
		{
			MessageHandler::logError("CameraPathManager::handleInsertNodeBeforeMessage::Corrupted byteArray");
			return;
		}

		// Read the byte from index 0 (the node index)
		const uint8_t nodeIndex = byteArray[2];

		if (!_paths.contains(_selectedPath))
		{
			MessageHandler::logError("CameraPathManager::handleInsertNodeBeforeMessage::Path not found - exiting insertNodeBefore");
			return;
		}

		//MessageHandler::logDebug("CameraPathManager::handleInsertNodeBeforeMessage::Inserting Node before %zu", nodeIndex);
		_paths.at(_selectedPath).insertNodeBefore(nodeIndex);
		sendAllCameraPathsCombined();
	}

	void CameraPathManager::sendAllCameraPathsCombined() const
	{
		//std::string jsonData = serializeAllCameraPaths(*this);
		//Console::WriteLine(jsonData);
		//NamedPipeManager::instance().writeTextPayload(jsonData, MessageType::CameraPathData);

		sendAllCameraPathsBinary();
		// Update visualization
		D3DHook::instance().safeInterpolationModeChange();
	}

	void CameraPathManager::handleAddPathMessage(uint8_t byteArray[], const DWORD arrayLength)
	{
		// Check if there is at least one byte delivered to us
		if (arrayLength < 1)
		{
			MessageHandler::logError("CameraPathManager::handleAddPathMessage::Corrupted byteArray");
			return;
		}

		const std::string pathName = Utils::stringFromBytes(byteArray, arrayLength, 2);
		if (createPath(pathName))
		{
			//MessageHandler::logDebug("CameraPathManager::handleAddPathMessage::Path created: %s", pathName.c_str());
			if (_paths.contains(pathName))
			{
				//MessageHandler::logDebug("CameraPathManager::handleAddPathMessage::Path found: %s", pathName.c_str());
				_paths.at(pathName).setTotalDuration(static_cast<float>(Globals::instance().settings().pathDurationSetting));
				_paths.at(pathName).addNode();
				sendAllCameraPathsCombined();

				sendSelectedPathUpdate(pathName);
			}
			else
			{
				MessageHandler::logError("CameraPathManager::handleAddPathMessage::Path not found: %s", pathName.c_str());
			}
		}
		else
		{
			MessageHandler::logError("CameraPathManager::handleAddPathMessage::Failed to create path: %s", pathName.c_str());
		}
	}

	std::string CameraPathManager::handleAddPathMessage()
	{
		std::string pathName = generateDefaultPathName();
		if (createPath(pathName))
		{
			// Set the created path as selected
			_selectedPath = pathName;

			// Configure the path
			_paths.at(pathName).setTotalDuration(static_cast<float>(Globals::instance().settings().pathDurationSetting));
			_paths.at(pathName).addNode();

			// Send the updated paths to the client
			sendAllCameraPathsCombined();
			sendSelectedPathUpdate(pathName);
			updateOffsetforVisualisation();
			return pathName;
		
		}
		MessageHandler::logError("CameraPathManager::handleAddPathMessage::Failed to create path: %s", pathName.c_str());
		return "";
	}

	std::string CameraPathManager::generateDefaultPathName() const
	{
		const std::string baseName = "Path";
		int maxNumber = 0;

		// Loop through all existing paths
		for (const auto& [fst, snd] : _paths)
		{
			// Check if the path name starts with the base name
			if (const std::string& pathName = fst; pathName.compare(0, baseName.length(), baseName) == 0)
			{
				// Extract the number part after the base name
				std::string numberPart = pathName.substr(baseName.length());

				// Try to parse the number
				try
				{
					if (const int number = std::stoi(numberPart); number > maxNumber)
					{
						maxNumber = number;
					}
				}
				catch (const std::exception&)
				{
					// Not a valid number, skip this path
					continue;
				}
			}
		}

		// Return base name with next number
		return baseName + std::to_string(maxNumber + 1);
	}


	void CameraPathManager::handleDeleteNodeMessage(uint8_t byteArray[], const DWORD arrayLength)
	{
		// Check if there is at least one byte delivered to us
		if (arrayLength < 1)
		{
			MessageHandler::logError("CameraPathManager::decodeDeleteNotePayLoad::Corrupted byteArray");
			return;
		}

		// Read the byte from index 0.
		const uint8_t nodeIndex = byteArray[2];

		if (!_paths.contains(_selectedPath))
		{
			MessageHandler::logError("CameraPathManager::decodeDeleteNotePayLoad::Path not found - exiting deleteNode");
			return;
		}

		//MessageHandler::logDebug("CameraPathManager::decodeDeleteNotePayLoad::Deleting Node %zu", nodeIndex);
		_paths.at(_selectedPath).deleteNode(nodeIndex);
		sendAllCameraPathsCombined();
	}

	void CameraPathManager::handleDeleteNodeMessage(const uint8_t nodeIndex)
	{

		if (!_paths.contains(_selectedPath))
		{
			MessageHandler::logError("CameraPathManager::decodeDeleteNotePayLoad::Path not found - exiting deleteNode");
			return;
		}

		if (!Globals::instance().settings().d3ddisabled && D3DHook::instance().isVisualizationEnabled()) {
			D3DHook::instance().markResourcesForUpdate();
		}

		//MessageHandler::logDebug("CameraPathManager::decodeDeleteNotePayLoad::Deleting Node %zu", nodeIndex);
		_paths.at(_selectedPath).deleteNode(nodeIndex);
		sendAllCameraPathsCombined();
	}

	void CameraPathManager::refreshPath()
	{
		//CameraPath* path = getPath(pathName);
		if (!_paths.contains(_selectedPath))
		{
			MessageHandler::logDebug("CameraPathManager::refreshPath::%s not found in array", _selectedPath.c_str());
			return;
		}
		_paths.at(_selectedPath).continuityCheck();
		_paths.at(_selectedPath).generateArcLengthTables();
	}

	void CameraPathManager::updateNode(uint8_t byteArray[], DWORD arrayLength)
	{
		// Read the node index from the first byte.
		const uint8_t index = byteArray[2];

		if (!_paths.contains(_selectedPath))
		{
			MessageHandler::logError("CameraPathManager::updateNode::%s not found in array", _selectedPath.c_str());
			return;
		}
		_paths.at(_selectedPath).updateNode(index);
		
		sendAllCameraPathsCombined();
	}

	void CameraPathManager::goToNodeSetup(uint8_t byteArray[], const DWORD arrayLength)
	{
		// Check if there is at least one byte delivered to us
		if (arrayLength < 1) {
			MessageHandler::logDebug("CameraPathManager::goToNodeSetup::Corrupted byteArray");
			return;
		}

		if (!_pathManagerState == Idle) {
			MessageHandler::logDebug("NamedPipeSubsystem::goToNodeSetup Case:Path Controller not idle - exiting gotoNodeIndex");
			return;
		}

		// Read the node index from the first byte.
		nodePayload.nodeIndex = byteArray[2];
		std::string pathName = Utils::stringFromBytes(byteArray, arrayLength, 3);// Decode the rest of the bytes as a string starting at index 3.
		//MessageHandler::logDebug("CameraPathManager::goToNodeSetup::Index Extracted: %u", nodePayload.nodeIndex);


		if (!_paths.contains(pathName)) {
			MessageHandler::logDebug("CameraPathManager::goToNodeSetup::No path found in array");
			MessageHandler::logError("Path: %s not found in array", pathName.c_str());
			return;
		}

		if (_selectedPath != pathName) {
			MessageHandler::logDebug("CameraPathManager::goToNodeSetup::Path name mismatch - returning");
			return;
		}

		initNodePayload();
		updatePMState(GotoNode);

		MessageHandler::logLine("Going to Node %zu of path: %s", nodePayload.nodeIndex, _selectedPath.c_str());
	}

	void CameraPathManager::initNodePayload()
	{
		nodePayload.currentQ = Camera::instance().getToolsQuaternion();
		nodePayload.currentP = Camera::instance().getInternalPosition();
		nodePayload.currentfov = GameSpecific::CameraManipulator::getCurrentFoV();
		nodePayload.interpolating = false;
		nodePayload.elapsedTime = 0.0f;
		nodePayload.totalDuration = 1.0f;
		nodePayload.path = getPath(_selectedPath);
	}


	void CameraPathManager::gotoNodeIndex(const float deltaTime)
	{
		// Early validation checks
		if (_pathManagerState != PathManagerStatus::GotoNode) {
			MessageHandler::logDebug("CameraPathManager::gotoNodeIndex: Path Controller not in correct mode - returning");
			updatePMState(Idle);
			return;
		}

		if (!_paths.contains(_selectedPath)) {
			MessageHandler::logDebug("CameraPathManager::gotoNodeIndex: Invalid path - returning without applying node");
			updatePMState(Idle);
			return;
		}

		if (nodePayload.nodeIndex < 0 || nodePayload.nodeIndex >= _paths[_selectedPath].GetNodeCount()) {
			MessageHandler::logDebug("CameraPathManager::gotoNodeIndex: Invalid node index - returning without applying node");
			updatePMState(Idle);
			return;
		}

		// Load current position and rotation
		const XMVECTOR cP = XMLoadFloat3(&nodePayload.currentP);
		const XMVECTOR cR = XMLoadFloat4(&nodePayload.currentQ);
		const float cF = nodePayload.currentfov;

		// Get target position and rotation
		const XMVECTOR tP = nodePayload.path->getNodePosition(nodePayload.nodeIndex);
		XMVECTOR tR = nodePayload.path->getNodeRotation(nodePayload.nodeIndex);
		const float tF = nodePayload.path->getNodeFOV(nodePayload.nodeIndex);

		// Update interpolation progress
		nodePayload.elapsedTime += deltaTime;
		float t = nodePayload.elapsedTime / nodePayload.totalDuration;
		t = static_cast<float>(PathUtils::smoothStepExp(0.0, 1.0, t, 1.4)); // Apply easing
		t = min(1.0f, t); // Clamp to [0,1]

		//Check quaternion continuity
		PathUtils::EnsureQuaternionContinuity(cR, tR);

		// Perform Catmull-Rom interpolation for smooth movement
		const XMVECTOR iP = XMVectorCatmullRom(cP, cP, tP, tP, t);
		const XMVECTOR iR = XMQuaternionNormalize(XMVectorCatmullRom(cR, cR, tR, tR, t));
		const float iF = PathUtils::lerpFMA(cF, tF, t);

		// Apply the interpolated camera transform
		GameSpecific::CameraManipulator::writeNewCameraValuesToGameData(iP, iR);
		GameSpecific::CameraManipulator::changeFoV(iF);

		// Check if interpolation is complete
		if (t >= 1.0f) {
			// Set final orientation using Euler angles
			XMFLOAT3 position;
			const XMFLOAT3 eulers = Utils::QuaternionToEulerAngles(nodePayload.path->getNodeRotation(nodePayload.nodeIndex));
			const XMVECTOR pos = nodePayload.path->getNodePosition(nodePayload.nodeIndex);
			XMStoreFloat3(&position, pos);
			const float fov = nodePayload.path->getNodeFOV(nodePayload.nodeIndex);

			//Write final values to game
			Camera::instance().setAllRotation(eulers);
			Camera::instance().setInternalPosition(position);
			Camera::instance().setFoV(fov, true);

			//apply reshade state if connected
			applyReshadeState(_selectedPath, nodePayload.nodeIndex);

			// Reset state
			updatePMState(Idle);
			nodePayload.interpolating = false;
		}
	}

	void CameraPathManager::sendAllCameraPathsBinary() const
	{
		// Calculate required buffer size
		size_t totalSize = BinaryPathStructs::calculateTotalSize(*this);

		// Create a buffer to hold all the data
		std::vector<uint8_t> buffer(totalSize);
		size_t currentPos = 0;

		// Write the main header
		BinaryPathStructs::BinaryPathHeader mainHeader;
		mainHeader.formatVersion = BinaryPathStructs::BINARY_PATH_FORMAT_VERSION;
		mainHeader.pathCount = static_cast<uint8_t>(_paths.size());

		// Ensure we don't exceed the limits of our data types
		if (getPaths().size() > 255) {
			MessageHandler::logError("Too many paths to serialize in binary format (max 255)");
			return;
		}

		// Copy the header to the buffer
		memcpy(buffer.data() + currentPos, &mainHeader, sizeof(BinaryPathStructs::BinaryPathHeader));
		currentPos += sizeof(BinaryPathStructs::BinaryPathHeader);

		// Write each path
		for (const auto& pair : _paths)
		{
			const std::string& pathName = pair.first;
			const CameraPath& path = pair.second;
			size_t nodeCount = path.GetNodeCount();

			// Ensure we don't exceed the limits of our data types
			if (pathName.length() > 65535) {
				MessageHandler::logError("Path name too long for binary format (max 65535 chars)");
				continue;
			}

			if (nodeCount > 255) {
				MessageHandler::logError("Too many nodes in path for binary format (max 255)");
				continue;
			}

			// Create and fill the path header
			BinaryPathStructs::BinaryPathData pathHeader;
			pathHeader.nameLength = static_cast<uint16_t>(pathName.length());
			pathHeader.nodeCount = static_cast<uint8_t>(nodeCount);

			// Copy the path header to the buffer
			memcpy(buffer.data() + currentPos, &pathHeader, sizeof(BinaryPathStructs::BinaryPathData));
			currentPos += sizeof(BinaryPathStructs::BinaryPathData);

			// Copy the path name to the buffer
			memcpy(buffer.data() + currentPos, pathName.c_str(), pathName.length());
			currentPos += pathName.length();

			// Write each node
			for (size_t i = 0; i < nodeCount; i++)
			{
				// Ensure we don't exceed the limits of our data types
				if (i > 255) {
					MessageHandler::logError("Node index too large for binary format (max 255)");
					break;
				}

				BinaryPathStructs::BinaryNodeData nodeData;
				nodeData.index = static_cast<uint8_t>(i);

				// Convert position vector to float array
				XMFLOAT3 pos;
				XMStoreFloat3(&pos, path.getNodePosition(i));
				nodeData.position[0] = pos.x;
				nodeData.position[1] = pos.y;
				nodeData.position[2] = pos.z;

				// Convert rotation quaternion to float array
				XMFLOAT4 rot;
				XMStoreFloat4(&rot, path.getNodeRotation(i));
				nodeData.rotation[0] = rot.x;
				nodeData.rotation[1] = rot.y;
				nodeData.rotation[2] = rot.z;
				nodeData.rotation[3] = rot.w;

				// Set FOV
				nodeData.fov = path.getNodeFOV(i);

				// Copy the node data to the buffer
				memcpy(buffer.data() + currentPos, &nodeData, sizeof(BinaryPathStructs::BinaryNodeData));
				currentPos += sizeof(BinaryPathStructs::BinaryNodeData);
			}
		}

		// Send the binary data through the pipe
		NamedPipeManager::instance().writeBinaryPayload(
			buffer.data(),         // Data pointer
			buffer.size(),         // Data size
			MessageType::CameraPathBinaryData);  // Message type
	}

	void CameraPathManager::handlePathScrubbingMessage(uint8_t byteArray[], DWORD arrayLength)
	{
		if (arrayLength < 3)
		{
			MessageHandler::logError("CameraPathManager::handlePathScrubbingMessage::Corrupted byteArray");
			return;
		}

		bool startScrubbing = (byteArray[2] == 1);

		if (startScrubbing)
		{
			_preScrubbingState = _pathManagerState;
			// Initialize scrubbing state
			if (_paths.contains(_selectedPath))
			{
				float currentProgress = _paths[_selectedPath].getProgress();
				_currentScrubbingProgress = currentProgress;
				_targetScrubbingProgress = currentProgress;
			}
			
			// Update UI
			updatePMState(Scrubbing);
			MessageHandler::logDebug("PathManager: Entered scrubbing state");
		}
		else
		{
			// Handle exit from scrubbing based on where we were before
			if (_preScrubbingState == PlayingPath)
			{
				// Resume playing from current scrubbed position
				updatePMState(PlayingPath);

				// Update path's internal time/arc length to match scrubbed position
				if (_paths.contains(_selectedPath))
				{
					_paths[_selectedPath].setProgressPosition(_currentScrubbingProgress);
				}

				// The next call to interpolateNodes() in the PlayingPath state will handle
				// path completion if we've scrubbed to the end
				MessageHandler::logDebug("PathManager: Resumed playing from scrubbed position");
			}
			else
			{
				// If we were idle before scrubbing, return to idle
				updatePMState(Idle);
				// Ensure camera is set to the exact scrubbed position
				if (_paths.contains(_selectedPath))
				{
					_paths[_selectedPath].setProgressPosition(_currentScrubbingProgress);
					_paths[_selectedPath].interpolateNodes(0.0f);
				}

				MessageHandler::logDebug("PathManager: Exited scrubbing state to Idle");
			}
		}
	}

	void CameraPathManager::updatePMState(const PathManagerStatus status)
	{
		//Update status
		_pathManagerState = status;

		//Update UI
		NamedPipeManager::instance().writeBinaryPayload(_pathManagerState, MessageType::UpdatePathPlaying);
	}

	// ADD new method for smooth interpolation:
	void CameraPathManager::updateScrubbingInterpolation(float deltaTime)
	{
		if (!_paths.contains(_selectedPath))
			return;

		// Smooth interpolation to target
		float t = min(1.0f, deltaTime * SCRUBBING_SMOOTHING);
		_currentScrubbingProgress = _currentScrubbingProgress + t * (_targetScrubbingProgress - _currentScrubbingProgress);

		// Apply the interpolated position
		_paths[_selectedPath].setProgressPosition(_currentScrubbingProgress);
		_paths[_selectedPath].interpolateNodes(0.0f);
	}

	void CameraPathManager::scrubPath()
	{
		if (_pathManagerState == Scrubbing)
		{
			updateScrubbingInterpolation(System::instance().getDT());
		}
	}

	void CameraPathManager::sendPathProgress(float progress) const
	{
		// Ensure progress is valid
		progress = max(0.0f, min(1.0f, progress));

		// Check for NaN or infinity
		if (!std::isfinite(progress))
		{
			progress = 0.0f;
		}

		// Debug log the actual progress value
		//Console::WriteLine("Sending path progress: %.4f", progress);

		// Use the NamedPipeManager's binary payload method directly
		NamedPipeManager::instance().writeFloatPayload(progress, MessageType::PathProgress);
	}

	void CameraPathManager::updateOffsetforVisualisation()
	{
		if (GameSpecific::CameraManipulator::getCameraStructAddress() == nullptr)
		{
			return;
		}

		if (!_pathManagerEnabled)
		{
			return;
		}

		if (!_paths.contains(_selectedPath))
			return;

		if (!Globals::instance().settings().pathLookAtEnabled)
		{
			return; 
		}

		try
		{
			// Update the offset for visualization
			_paths[_selectedPath].updatePathLookAtTarget();
		}
		catch (exception& e)
		{
			MessageHandler::logError("CameraPathManager::updateOffsetforVisualisation::Exception occurred: %s", e.what());
		}
		catch (...)
		{
			MessageHandler::logError("CameraPathManager::updateOffsetforVisualisation::Unknown exception occurred");
		}

	}

	void CameraPathManager::connectToAddon(const HMODULE hModule)
	{
		_addCameraPath = reinterpret_cast<IgcsConnector_addCameraPath>(GetProcAddress(hModule, "addCameraPath"));
		_appendStateSnapshotToPath = reinterpret_cast<IgcsConnector_appendStateSnapshotToPath>(GetProcAddress(hModule, "appendStateSnapshotToPath"));
		_appendStateSnapshotAfterSnapshotOnPath = reinterpret_cast<IgcsConnector_appendStateSnapshotAfterSnapshotOnPath>(GetProcAddress(hModule, "appendStateSnapshotAfterSnapshotOnPath"));
		_insertStateSnapshotBeforeSnapshotOnPath = reinterpret_cast<IgcsConnector_insertStateSnapshotBeforeSnapshotOnPath>(GetProcAddress(hModule, "insertStateSnapshotBeforeSnapshotOnPath"));
		_removeStateSnapshotFromPath = reinterpret_cast<IgcsConnector_removeStateSnapshotFromPath>(GetProcAddress(hModule, "removeStateSnapshotFromPath"));
		_removeCameraPath = reinterpret_cast<IgcsConnector_removeCameraPath>(GetProcAddress(hModule, "removeCameraPath"));
		_clearPaths = reinterpret_cast<IgcsConnector_clearPaths>(GetProcAddress(hModule, "clearPaths"));
		_setReshadeState = reinterpret_cast<IgcsConnector_setReshadeState>(GetProcAddress(hModule, "setReshadeState"));
		_setReshadeStateInterpolated = reinterpret_cast<IgcsConnector_setReshadeStateInterpolated>(GetProcAddress(hModule, "setReshadeStateInterpolated"));
		_updateStateSnapshotOnPath = reinterpret_cast<IgcsConnector_updateStateSnapshotOnPath>(GetProcAddress(hModule, "updateStateSnapshotOnPath"));


		// Log connection status
		if (_addCameraPath && _appendStateSnapshotToPath && _setReshadeState && _setReshadeStateInterpolated) {
			MessageHandler::logLine("Successfully connected to IgcsConnector path management functions");
		}
		else {
			MessageHandler::logError("Failed to get all IgcsConnector path management functions");
		}
	}

	// Path Management Helper Functions
	bool CameraPathManager::reshadeAddonisConnected() {
		return _addCameraPath != nullptr &&
			_appendStateSnapshotToPath != nullptr &&
			_setReshadeState != nullptr &&
			_removeStateSnapshotFromPath != nullptr;
	}

	void CameraPathManager::addReshadePath() {
		if (reshadeAddonisConnected()) {
			_addCameraPath();
			MessageHandler::logDebug("IGCSConnector::addPath - Added new camera path");
			return;
		}
		//MessageHandler::logDebug("IGCSConnector::addPath - Not connected to IgcsConnector addon");
	}

	void CameraPathManager::appendStateToReshadePath(const std::string& pathName) {
		if (reshadeAddonisConnected()) {

			if (!pathIndexMap.contains(pathName)) {
				MessageHandler::logError("IGCSConnector::appendStateToPath - Path not found: %s", pathName.c_str());
				return;
			}

			int pathIndex = pathIndexMap[pathName];

			_appendStateSnapshotToPath(pathIndex);
			MessageHandler::logDebug("IGCSConnector::appendStateToPath - Added state to path %d", pathIndex);
			return;
		}
		//MessageHandler::logDebug("IGCSConnector::appendStateToPath - Not connected to IgcsConnector addon");
	}

	//not used
	void CameraPathManager::appendStateAfterReshadeSnapshot(const int pathIndex, const int snapshotIndex) {
		if (reshadeAddonisConnected()) {
			_appendStateSnapshotAfterSnapshotOnPath(pathIndex, snapshotIndex);
			MessageHandler::logDebug("IGCSConnector::appendStateAfterSnapshot - Added state after index %d on path %d",
				snapshotIndex, pathIndex);
			return;
		}
		//MessageHandler::logDebug("IGCSConnector::appendStateAfterSnapshot - Not connected to IgcsConnector addon");
	}

	void CameraPathManager::insertStateBeforeReshadeSnapshot(const std::string& pathName, const int snapshotIndex) {
		if (reshadeAddonisConnected()) {

			if (!pathIndexMap.contains(pathName)) {
				MessageHandler::logError("IGCSConnector::removeState - Path not found: %s", pathName.c_str());
				return;
			}

			int pathIndex = pathIndexMap[pathName];

			_insertStateSnapshotBeforeSnapshotOnPath(pathIndex, snapshotIndex);
			MessageHandler::logDebug("IGCSConnector::insertStateBeforeSnapshot - Inserted state before index %d on path %d",
				snapshotIndex, pathIndex);
			return;
		}
		//MessageHandler::logDebug("IGCSConnector::insertStateBeforeSnapshot - Not connected to IgcsConnector addon");
	}

	void CameraPathManager::removeReshadeState(const std::string& pathName, const int stateIndex) {
		if (reshadeAddonisConnected()) {

			if (!pathIndexMap.contains(pathName)) {
				MessageHandler::logError("IGCSConnector::removeState - Path not found: %s", pathName.c_str());
				return;
			}

			int pathIndex = pathIndexMap[pathName];

			_removeStateSnapshotFromPath(pathIndex, stateIndex);
			MessageHandler::logDebug("IGCSConnector::removeState - Removed state %d from path %s (IGCS Path Index: %d)",
				stateIndex, pathName.c_str(), pathIndex);
			return;
		}
		//MessageHandler::logDebug("IGCSConnector::removeState - Not connected to IgcsConnector addon");
	}

	void CameraPathManager::removeReshadePath(const std::string& pathName) {
		if (!reshadeAddonisConnected()) {
			//MessageHandler::logDebug("IGCSConnector::removePath - Not connected to IgcsConnector addon");
			return;
		}

		if (!pathIndexMap.contains(pathName)) {
			MessageHandler::logError("IGCSConnector::removePath - Path not found: %s", pathName.c_str());
			return;
		}

		int pathIndex = pathIndexMap[pathName];

		_removeCameraPath(pathIndex);
		MessageHandler::logDebug("IGCSConnector::removePath - Removed path: %s - IGCS index %d", pathName.c_str(), pathIndex);
	}

	void CameraPathManager::clearAllReshadelPaths() {
		if (reshadeAddonisConnected() && _clearPaths) {
			_clearPaths();
			//MessageHandler::logDebug("IGCSConnector::clearAllPaths - Cleared all paths");
		}
		//MessageHandler::logDebug("IGCSConnector::clearAllPaths - Not connected to IgcsConnector addon");
	}

	void CameraPathManager::applyReshadeState(const std::string& pathName, const int stateIndex) {
		if (!reshadeAddonisConnected()) {
			//MessageHandler::logDebug("IGCSConnector::applyState - Not connected to IgcsConnector addon");
			return;
		}

		if (!pathIndexMap.contains(pathName)) {
			MessageHandler::logError("IGCSConnector::applyState - Path not found: %s", pathName.c_str());
			return;
		}

		const int pathIndex = pathIndexMap[pathName];

		_setReshadeState(pathIndex, stateIndex);
		MessageHandler::logDebug("IGCSConnector::applyState - Applied state %d from path %s (index: %d)",
			stateIndex, pathName.c_str(), pathIndex);
	}

	void CameraPathManager::applyInterpolatedReshadeState(const std::string& pathName, const int fromState, const int toState, const float factor) {
		if (reshadeAddonisConnected()) {


			if (!pathIndexMap.contains(pathName)) {
				MessageHandler::logError("IGCSConnector::applyState - Path not found: %s", pathName.c_str());
				return;
			}

			int pathIndex = pathIndexMap[pathName];


			_setReshadeStateInterpolated(pathIndex, fromState, toState, factor);

			//MessageHandler::logDebug("IGCSConnector::applyInterpolatedState - Applied interpolated state between %d and %d (factor: %.3f) from path %d",
			//	fromState, toState, factor, pathIndex);
			return;
		}
		//MessageHandler::logDebug("IGCSConnector::applyInterpolatedState - Not connected to IgcsConnector addon");
	}

	void CameraPathManager::updateReshadeState(const std::string& pathName, const int stateIndex) {
		if (reshadeAddonisConnected() && _updateStateSnapshotOnPath) {

			if (!pathIndexMap.contains(pathName)) {
				MessageHandler::logError("IGCSConnector::applyState - Path not found: %s", pathName.c_str());
				return;
			}

			int pathIndex = pathIndexMap[pathName];

			_updateStateSnapshotOnPath(pathIndex, stateIndex);

			MessageHandler::logDebug("IGCSConnector::updateState - Updated state %d on path %d",
				stateIndex, pathIndex);

			return;
		}
		//MessageHandler::logDebug("IGCSConnector::updateState - Not connected to IgcsConnector addon");
	}

	void CameraPathManager::rebuildPathIndices() {
		// Clear existing maps
		pathIndexMap.clear();
		pathOrder.clear();
		indexPathMap.clear();

		// Rebuild with current paths
		int idx = 0;
		for (const auto& [pathName, pathObj] : _paths) {
			pathIndexMap[pathName] = idx;
			indexPathMap[idx] = pathName;
			pathOrder.push_back(pathName);
			idx++;
		}
	}


}