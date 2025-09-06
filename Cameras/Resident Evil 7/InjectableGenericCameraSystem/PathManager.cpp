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
#include "D3D12Hook.h"

namespace IGCS
{
	// Function pointers
	static PathManager::IgcsConnector_addCameraPath _addCameraPath = nullptr;
	static PathManager::IgcsConnector_appendStateSnapshotToPath _appendStateSnapshotToPath = nullptr;
	static PathManager::IgcsConnector_appendStateSnapshotAfterSnapshotOnPath _appendStateSnapshotAfterSnapshotOnPath = nullptr;
	static PathManager::IgcsConnector_insertStateSnapshotBeforeSnapshotOnPath _insertStateSnapshotBeforeSnapshotOnPath = nullptr;
	static PathManager::IgcsConnector_removeStateSnapshotFromPath _removeStateSnapshotFromPath = nullptr;
	static PathManager::IgcsConnector_removeCameraPath _removeCameraPath = nullptr;
	static PathManager::IgcsConnector_clearPaths _clearPaths = nullptr;
	static PathManager::IgcsConnector_setReshadeState _setReshadeState = nullptr;
	static PathManager::IgcsConnector_setReshadeStateInterpolated _setReshadeStateInterpolated = nullptr;
	static PathManager::IgcsConnector_updateStateSnapshotOnPath _updateStateSnapshotOnPath = nullptr;

	using namespace DirectX;

	PathManager::PathManager() :
		nodePayload{ .nodeIndex= static_cast<uint8_t>(0), .currentQ= XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f), .currentP=
			XMFLOAT3(0.0f, 0.0f, 0.0f), .currentfov= 0.0f, .interpolating= false, .elapsedTime= 0.0f, .totalDuration=
			0.0f
		},
		_selectedPath{}
	{
	}

	PathManager::~PathManager()
	{
		_paths.clear();
	}

	CameraPath* PathManager::getPath(const std::string& pathName) 
	{
		if (_paths.contains(pathName)) {
			return &_paths[pathName];
		}
		MessageHandler::logError("PathManager::getPath::Path not found - returning nullptr");
		return nullptr;
	}

	void PathManager::playPath(const float deltaTime)
	{
		_paths[_selectedPath].interpolateNodes(deltaTime);
	}

	void PathManager::handlePlayPathMessage()
	{
		if (_pathManagerStatus != Idle){
			MessageHandler::logDebug("PathManagerEnabled::handlePlayPathMessage:Path Controller not idle - exiting playPath");
			return;
		}

		if (!_paths.contains(_selectedPath)){
			MessageHandler::logError("PathManager::handlePlayPathMessage::Path not found - exiting playPath");
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
		MessageHandler::logDebug("PathManager::handlePlayPathMessage::Path Playback Started");
	}

	void PathManager::setSelectedPath(uint8_t buffer[], const DWORD bytesRead)
	{
		// Check if there is at least one byte delivered to us
		if (bytesRead < 1)
		{
			MessageHandler::logError("PathManager::setSelectedPath::Corrupted byteArray");
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
			MessageHandler::logError("PathManager::setSelectedPath::Path name is empty, not set");
		}
	}

	void PathManager::setPathManagerStatus(uint8_t buffer[], const DWORD bytesRead)
	{
		if (bytesRead < 1)
		{
			MessageHandler::logError("PathManager::setPathManagerStatus::Corrupted byteArray");
			return;
		}
		const auto state = static_cast<PathManagerState>(buffer[2]);
		_pathManagerState = state;
	}

	void PathManager::setSelectedNodeIndex(uint8_t buffer[], const DWORD bytesRead)
	{
		// Check if there is at least one byte delivered to us
		if (bytesRead < 1)
		{
			MessageHandler::logError("PathManager::setSelectedNodeIndex::Corrupted byteArray");
			return;
		}

		const uint8_t nodeIndex = buffer[2]; // Read the node index from the byte array
		if (nodeIndex > _paths.at(_selectedPath).getNodeSize())
		{
			MessageHandler::logDebug("PathManager::setSelectedNodeIndex::node index out of bounds for given path");
			return;
		}

		if (_paths.contains(_selectedPath))
		{
			_selectedNodeIndex = nodeIndex;
		}
	}

	void PathManager::sendSelectedPathUpdate(const std::string& pathName) const
	{
		if (pathName.empty())
		{
			MessageHandler::logError("PathManager::sendSelectedPathUpdate::Path name is empty, no message sent");
			return;
		}
		if (!_paths.contains(pathName))
		{
			MessageHandler::logError("PathManager::sendSelectedPathUpdate::Path not found, no message sent");
			return;
		}

		NamedPipeManager::instance().writeTextPayload(pathName, MessageType::UpdateSelectedPath);
	}

	void PathManager::setSelectedPath(const string& pathName)
	{
		if (_paths.contains(pathName))
		{
			_selectedPath.clear();
			_selectedPath = pathName;
		}
		else
		{
			MessageHandler::logError("PathManager::setSelectedPath::Path name is empty, not set");
		}
	}

	void PathManager::handleStopPathMessage()
	{
		switch (_pathManagerStatus)
		{
		case Idle:
			MessageHandler::logDebug("PathManager::handleStopPathMessage::Controller already Idle. Returning");
			break;
		case PlayingPath:
			updatePMState(Idle);
			_paths.at(_selectedPath).interpolateNodes(System::instance().getDT());
			(_pathManagerStatus == Idle) ?
				MessageHandler::logDebug("PathManager::handleStopPathMessage::Controller state set to Idle") :
				MessageHandler::logError("PathManager::handleStopPathMessage::Controller state change unsuccessful");
			break;
		case GotoNode:
			MessageHandler::logDebug("PathManager::handleStopPathMessage::gotoNodeIndex currently playing. Returning");
			break;
		default:
			break;
		}
	}

	bool PathManager::createPath(const std::string& pathName) 
	{
		if (ActionLock lock(_isProcessingAction); !lock) {
			MessageHandler::logDebug("PathManager::createPath::Action already in progress");
			return false;
		}

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
			MessageHandler::logError("PathManager::createPath::Failed to create path: %s", pathName.c_str());
			return false;
		}

	}

	bool PathManager::handleDeletePathMessage(const std::string& pathName) {
		if (ActionLock lock(_isProcessingAction); !lock) {
			MessageHandler::logDebug("PathManager::handleDeletePathMessage::Action already in progress");
			return false;
		}

		if (_paths.contains(pathName)) {

			D3DHookChecks();

			_paths.erase(_paths.find(pathName)); // Remove the path from the map

			// Check if _paths is empty after deletion
			if (_paths.empty()) {
				MessageHandler::logDebug("PathManager::handleDeletePathMessage::No paths left, clearing all paths in Reshade");
				clearAllReshadelPaths();
				//clear path vector to be sure we don't have any dangling pointers
				_paths.clear();
				// Also clear the path indices
				pathIndexMap.clear();
				indexPathMap.clear();
				pathOrder.clear();
				sendAllCameraPathsCombined();
				return true;
			}

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

	bool PathManager::handleDeletePathMessage() {
		if (ActionLock lock(_isProcessingAction); !lock) {
			MessageHandler::logDebug("PathManager::handleDeletePathMessage::Action already in progress");
			return false;
		}

		if (_paths.contains(_selectedPath)) {

			D3DHookChecks();

			_paths.erase(_paths.find(_selectedPath)); // Remove the path from the map
			MessageHandler::logLine("Path deleted: %s", _selectedPath.c_str());

			// Check if _paths is empty after deletion
			if (_paths.empty()) {
				MessageHandler::logDebug("PathManager::handleDeletePathMessage::No paths left, clearing all paths in Reshade");
				clearAllReshadelPaths();
				//clear path vector to be sure we don't have any dangling pointers
				_paths.clear();
				// Also clear the path indices
				pathIndexMap.clear();
				indexPathMap.clear();
				pathOrder.clear();
				sendAllCameraPathsCombined();
				return true;
			}

			// remove equivalent path in IgcsConnector
			if (reshadeAddonisConnected())
				removeReshadePath(_selectedPath);

			// Update path indices
			rebuildPathIndices();

			sendAllCameraPathsCombined();
			return true;
		}
		MessageHandler::logError("Path not found: %s", _selectedPath.c_str());
		return false;
	}

	uint8_t PathManager::handleAddNodeMessage()
	{
		if (ActionLock lock(_isProcessingAction); !lock) {
			MessageHandler::logDebug("PathManager::handleAddNodeMessage::Action already in progress");
			return false;
		}

		if (_paths.contains(_selectedPath) && !_selectedPath.empty())
		{
			const auto n = _paths.at(_selectedPath).addNode();
			
			sendAllCameraPathsCombined();
			
			//MessageHandler::logDebug("PathManager::handleAddNodeMessage::%s node added", _selectedPath.c_str());
			//MessageHandler::logLine("Node %zu added to path: %s", n, _selectedPath.c_str());
			return n;
		}
		MessageHandler::logError("PathManager::handleAddNodeMessage::%s not found in array", _selectedPath.c_str());
		return 0;
	}

	bool PathManager::handleInsertNodeBeforeMessage(uint8_t byteArray[], const DWORD arrayLength)
	{
		if (ActionLock lock(_isProcessingAction); !lock) {
			MessageHandler::logDebug("PathManager::handleInsertNodeBeforeMessage::Action already in progress");
			return false;
		}

		// Check if there is at least one byte delivered to us
		if (arrayLength < 1)
		{
			MessageHandler::logError("PathManager::handleInsertNodeBeforeMessage::Corrupted byteArray");
			return false;
		}

		// Read the byte from index 0 (the node index)
		const uint8_t nodeIndex = byteArray[2];

		if (!_paths.contains(_selectedPath))
		{
			MessageHandler::logError("PathManager::handleInsertNodeBeforeMessage::Path not found - exiting insertNodeBefore");
			return false;
		}

		//MessageHandler::logDebug("PathManager::handleInsertNodeBeforeMessage::Inserting Node before %zu", nodeIndex);
		_paths.at(_selectedPath).insertNodeBefore(nodeIndex);
		sendAllCameraPathsCombined();
		return true;
	}

	void PathManager::sendAllCameraPathsCombined()
	{
		if (ActionLock lock(_isProcessingAction); !lock) {
			MessageHandler::logDebug("PathManager::handleInsertNodeBeforeMessage::Action already in progress");
			return ;
		}

		//std::string jsonData = serializeAllCameraPaths(*this);
		//Console::WriteLine(jsonData);
		//NamedPipeManager::instance().writeTextPayload(jsonData, MessageType::CameraPathData);

		sendAllCameraPathsBinary();

		if (Globals::instance().settings().d3ddisabled)
			return;

		// Update visualization
		D3DHookChecks();
	}

	void PathManager::D3DHookChecks()
	{
		if (D3DMODE == D3DMODE::DX11 && !Globals::instance().settings().d3ddisabled && D3DHook::instance().isVisualizationEnabled()) {
			D3DHook::instance().markResourcesForUpdate();
		}

		if (D3DMODE == D3DMODE::DX12 && !Globals::instance().settings().d3ddisabled && D3D12Hook::instance().isVisualisationEnabled()) {
			D3D12Hook::instance().markResourcesForUpdate();
		}
	}

	void PathManager::handleAddPathMessage(uint8_t byteArray[], const DWORD arrayLength)
	{
		if (ActionLock lock(_isProcessingAction); !lock) {
			MessageHandler::logDebug("PathManager::handleAddPathMessage::Action already in progress");
			return;
		}

		// Check if there is at least one byte delivered to us
		if (arrayLength < 1)
		{
			MessageHandler::logError("PathManager::handleAddPathMessage::Corrupted byteArray");
			return;
		}

		const std::string pathName = Utils::stringFromBytes(byteArray, arrayLength, 2);
		if (createPath(pathName))
		{
			//MessageHandler::logDebug("PathManager::handleAddPathMessage::Path created: %s", pathName.c_str());
			if (_paths.contains(pathName))
			{
				//MessageHandler::logDebug("PathManager::handleAddPathMessage::Path found: %s", pathName.c_str());
				_paths.at(pathName).setTotalDuration(static_cast<float>(Globals::instance().settings().pathDurationSetting));
				_paths.at(pathName).addNode();
				sendAllCameraPathsCombined();

				sendSelectedPathUpdate(pathName);
			}
			else
			{
				MessageHandler::logError("PathManager::handleAddPathMessage::Path not found: %s", pathName.c_str());
			}
		}
		else
		{
			MessageHandler::logError("PathManager::handleAddPathMessage::Failed to create path: %s", pathName.c_str());
		}
	}

	std::string PathManager::handleAddPathMessage()
	{
		if (ActionLock lock(_isProcessingAction); !lock) {
			MessageHandler::logDebug("PathManager::handleAddPathMessage::Action already in progress");
			return "";
		}

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
		MessageHandler::logError("PathManager::handleAddPathMessage::Failed to create path: %s", pathName.c_str());
		return "";
	}

	std::string PathManager::generateDefaultPathName() const
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


	void PathManager::handleDeleteNodeMessage(uint8_t byteArray[], const DWORD arrayLength)
	{
		if (ActionLock lock(_isProcessingAction); !lock) {
			MessageHandler::logDebug("PathManager::handleDeleteNodeMessage::Action already in progress");
			return;
		}

		// Check if there is at least one byte delivered to us
		if (arrayLength < 1)
		{
			MessageHandler::logError("PathManager::decodeDeleteNotePayLoad::Corrupted byteArray");
			return;
		}

		// Read the byte from index 0.
		const uint8_t nodeIndex = byteArray[2];

		if (!_paths.contains(_selectedPath))
		{
			MessageHandler::logError("PathManager::decodeDeleteNotePayLoad::Path not found - exiting deleteNode");
			return;
		}

		//MessageHandler::logDebug("PathManager::decodeDeleteNotePayLoad::Deleting Node %zu", nodeIndex);
		_paths.at(_selectedPath).deleteNode(nodeIndex);
		sendAllCameraPathsCombined();
	}

	void PathManager::handleDeleteNodeMessage(const uint8_t nodeIndex)
	{
		if (ActionLock lock(_isProcessingAction); !lock) {
			MessageHandler::logDebug("PathManager::handleDeleteNodeMessage::Action already in progress");
			return;
		}

		if (!_paths.contains(_selectedPath))
		{
			MessageHandler::logError("PathManager::decodeDeleteNotePayLoad::Path not found - exiting deleteNode");
			return;
		}

		//MessageHandler::logDebug("PathManager::decodeDeleteNotePayLoad::Deleting Node %zu", nodeIndex);
		_paths.at(_selectedPath).deleteNode(nodeIndex);
		sendAllCameraPathsCombined();
	}

	void PathManager::refreshPath()
	{
		//CameraPath* path = getPath(pathName);
		if (!_paths.contains(_selectedPath))
		{
			MessageHandler::logDebug("PathManager::refreshPath::%s not found in array", _selectedPath.c_str());
			return;
		}
		_paths.at(_selectedPath).continuityCheck();
		_paths.at(_selectedPath).generateArcLengthTables();
	}

	void PathManager::updateNode(uint8_t byteArray[], DWORD arrayLength)
	{
		if (ActionLock lock(_isProcessingAction); !lock) {
			MessageHandler::logDebug("PathManager::handleDeleteNodeMessage::Action already in progress");
			return;
		}

		// Read the node index from the first byte.
		const uint8_t index = byteArray[2];

		if (!_paths.contains(_selectedPath))
		{
			MessageHandler::logError("PathManager::updateNode::%s not found in array", _selectedPath.c_str());
			return;
		}
		_paths.at(_selectedPath).updateNode(index);
		
		sendAllCameraPathsCombined();
	}

	void PathManager::goToNodeSetup(uint8_t byteArray[], const DWORD arrayLength)
	{
		// Check if there is at least one byte delivered to us
		if (arrayLength < 1) {
			MessageHandler::logDebug("PathManager::goToNodeSetup::Corrupted byteArray");
			return;
		}

		if (!_pathManagerStatus == Idle) {
			MessageHandler::logDebug("NamedPipeSubsystem::goToNodeSetup Case:Path Controller not idle - exiting gotoNodeIndex");
			return;
		}

		// Read the node index from the first byte.
		nodePayload.nodeIndex = byteArray[2];
		std::string pathName = Utils::stringFromBytes(byteArray, arrayLength, 3);// Decode the rest of the bytes as a string starting at index 3.
		//MessageHandler::logDebug("PathManager::goToNodeSetup::Index Extracted: %u", nodePayload.nodeIndex);


		if (!_paths.contains(pathName)) {
			MessageHandler::logDebug("PathManager::goToNodeSetup::No path found in array");
			MessageHandler::logError("Path: %s not found in array", pathName.c_str());
			return;
		}

		if (_selectedPath != pathName) {
			MessageHandler::logDebug("PathManager::goToNodeSetup::Path name mismatch - returning");
			return;
		}

		initNodePayload();
		updatePMState(GotoNode);

		MessageHandler::logLine("Going to Node %zu of path: %s", nodePayload.nodeIndex, _selectedPath.c_str());
	}

	void PathManager::initNodePayload()
	{
		nodePayload.currentQ = Camera::instance().getToolsQuaternion();
		nodePayload.currentP = Camera::instance().getInternalPosition();
		nodePayload.currentfov = GameSpecific::CameraManipulator::getCurrentFoV();
		nodePayload.interpolating = false;
		nodePayload.elapsedTime = 0.0f;
		nodePayload.totalDuration = 1.0f;
		nodePayload.path = getPath(_selectedPath);
	}


	void PathManager::gotoNodeIndex(const float deltaTime)
	{
		// Early validation checks
		if (_pathManagerStatus != PathManagerStatus::GotoNode) {
			MessageHandler::logDebug("PathManager::gotoNodeIndex: Path Controller not in correct mode - returning");
			updatePMState(Idle);
			return;
		}

		if (!_paths.contains(_selectedPath)) {
			MessageHandler::logDebug("PathManager::gotoNodeIndex: Invalid path - returning without applying node");
			updatePMState(Idle);
			return;
		}

		if (nodePayload.nodeIndex < 0 || nodePayload.nodeIndex >= _paths[_selectedPath].GetNodeCount()) {
			MessageHandler::logDebug("PathManager::gotoNodeIndex: Invalid node index - returning without applying node");
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

	void PathManager::sendAllCameraPathsBinary() const
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

	void PathManager::handlePathScrubbingMessage(uint8_t byteArray[], DWORD arrayLength)
	{
		if (arrayLength < 3)
		{
			MessageHandler::logError("PathManager::handlePathScrubbingMessage::Corrupted byteArray");
			return;
		}

		bool startScrubbing = (byteArray[2] == 1);

		if (startScrubbing)
		{
			_preScrubbingStatus = _pathManagerStatus;
			// Initialize scrubbing state
			if (_paths.contains(_selectedPath))
			{
				float currentProgress = _paths[_selectedPath].getProgress();
				_currentScrubbingProgress = currentProgress;
				_targetScrubbingProgress = currentProgress;
			}
			
			// Update UI
			updatePMState(Scrubbing);
			MessageHandler::logDebug("PathManagerEnabled: Entered scrubbing state");
		}
		else
		{
			// Handle exit from scrubbing based on where we were before
			if (_preScrubbingStatus == PlayingPath)
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
				MessageHandler::logDebug("PathManagerEnabled: Resumed playing from scrubbed position");
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

				MessageHandler::logDebug("PathManagerEnabled: Exited scrubbing state to Idle");
			}
		}
	}

	void PathManager::updatePMState(const PathManagerStatus status)
	{
		//Update status
		_pathManagerStatus = status;

		//Update UI
		NamedPipeManager::instance().writeBinaryPayload(_pathManagerStatus, MessageType::UpdatePathPlaying);
	}

	// ADD new method for smooth interpolation:
	void PathManager::updateScrubbingInterpolation(float deltaTime)
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

	void PathManager::scrubPath()
	{
		if (_pathManagerStatus == Scrubbing)
		{
			updateScrubbingInterpolation(System::instance().getDT());
		}
	}

	void PathManager::sendPathProgress(float progress)
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

	void PathManager::updateOffsetforVisualisation()
	{
		if (GameSpecific::CameraManipulator::getCameraStructAddress() == nullptr)
			return;

		if (!_pathManagerState)
			return;

		if (!_paths.contains(_selectedPath))
			return;

		if (!Globals::instance().settings().pathLookAtEnabled)
			return; 

		try
		{
			// Update the offset for visualization
			_paths[_selectedPath].updatePathLookAtTarget();
		}
		catch (exception& e)
		{
			MessageHandler::logError("PathManager::updateOffsetforVisualisation::Exception occurred: %s", e.what());
		}
		catch (...)
		{
			MessageHandler::logError("PathManager::updateOffsetforVisualisation::Unknown exception occurred");
		}

	}

	void PathManager::connectToAddon(const HMODULE hModule)
	{
		_reshadeResult = true;

		_addCameraPath = reinterpret_cast<IgcsConnector_addCameraPath>(GetProcAddress(hModule, "addCameraPath"));
		if (!_addCameraPath) {
			MessageHandler::logError("Failed to get addCameraPath function from IgcsConnector");
			_reshadeResult = false;
		}

		_appendStateSnapshotToPath = reinterpret_cast<IgcsConnector_appendStateSnapshotToPath>(GetProcAddress(hModule, "appendStateSnapshotToPath"));
		if (!_appendStateSnapshotToPath) {
			MessageHandler::logError("Failed to get appendStateSnapshotToPath function from IgcsConnector");
			_reshadeResult = false;
		}

		_appendStateSnapshotAfterSnapshotOnPath = reinterpret_cast<IgcsConnector_appendStateSnapshotAfterSnapshotOnPath>(GetProcAddress(hModule, "appendStateSnapshotAfterSnapshotOnPath"));
		if (!_appendStateSnapshotAfterSnapshotOnPath) {
			MessageHandler::logError("Failed to get appendStateSnapshotAfterSnapshotOnPath function from IgcsConnector");
			_reshadeResult = false;
		}

		_insertStateSnapshotBeforeSnapshotOnPath = reinterpret_cast<IgcsConnector_insertStateSnapshotBeforeSnapshotOnPath>(GetProcAddress(hModule, "insertStateSnapshotBeforeSnapshotOnPath"));
		if (!_insertStateSnapshotBeforeSnapshotOnPath) {
			MessageHandler::logError("Failed to get insertStateSnapshotBeforeSnapshotOnPath function from IgcsConnector");
			_reshadeResult = false;
		}

		_removeStateSnapshotFromPath = reinterpret_cast<IgcsConnector_removeStateSnapshotFromPath>(GetProcAddress(hModule, "removeStateSnapshotFromPath"));
		if (!_removeStateSnapshotFromPath) {
			MessageHandler::logError("Failed to get removeStateSnapshotFromPath function from IgcsConnector");
			_reshadeResult = false;
		}

		_removeCameraPath = reinterpret_cast<IgcsConnector_removeCameraPath>(GetProcAddress(hModule, "removeCameraPath"));
		if (!_removeCameraPath) {
			MessageHandler::logError("Failed to get removeCameraPath function from IgcsConnector");
			_reshadeResult = false;
		}

		_clearPaths = reinterpret_cast<IgcsConnector_clearPaths>(GetProcAddress(hModule, "clearPaths"));
		if (!_clearPaths) {
			MessageHandler::logError("Failed to get clearPaths function from IgcsConnector");
			_reshadeResult = false;
		}

		_setReshadeState = reinterpret_cast<IgcsConnector_setReshadeState>(GetProcAddress(hModule, "setReshadeState"));
		if (!_setReshadeState) {
			MessageHandler::logError("Failed to get setReshadeState function from IgcsConnector");
			_reshadeResult = false;
		}

		_setReshadeStateInterpolated = reinterpret_cast<IgcsConnector_setReshadeStateInterpolated>(GetProcAddress(hModule, "setReshadeStateInterpolated"));
		if (!_setReshadeStateInterpolated) {
			MessageHandler::logError("Failed to get setReshadeStateInterpolated function from IgcsConnector");
			_reshadeResult = false;
		}

		_updateStateSnapshotOnPath = reinterpret_cast<IgcsConnector_updateStateSnapshotOnPath>(GetProcAddress(hModule, "updateStateSnapshotOnPath"));
		if (!_updateStateSnapshotOnPath) {
			MessageHandler::logError("Failed to get updateStateSnapshotOnPath function from IgcsConnector");
			_reshadeResult = false;
		}

		// Log connection status
		if (_reshadeResult)
			MessageHandler::logLine("Successfully connected to IgcsConnector path management functions");
		else
			MessageHandler::logError("Failed to get all IgcsConnector path management functions");
	}

	// Path Management Helper Functions
	bool PathManager::reshadeAddonisConnected() const
	{
		return _reshadeResult;
	}

	void PathManager::addReshadePath() {
		if (!reshadeAddonisConnected()) {
			MessageHandler::logDebug("IGCSConnector::addReshadePath - Not connected to IgcsConnector addon");
			return;
		}

		_addCameraPath();
		MessageHandler::logDebug("IGCSConnector::addPath - Added new camera path");
	}

	void PathManager::appendStateToReshadePath(const std::string& pathName) {
		if (!reshadeAddonisConnected()) {
			MessageHandler::logDebug("IGCSConnector::appendStateToReshadePath - Not connected to IgcsConnector addon");
			return;
		}

		if (!pathIndexMap.contains(pathName)) {
			MessageHandler::logError("IGCSConnector::appendStateToReshadePath - Path not found: %s", pathName.c_str());
			return;
		}

		int pathIndex = pathIndexMap[pathName];

		_appendStateSnapshotToPath(pathIndex);
		MessageHandler::logDebug("IGCSConnector::appendStateToReshadePath - Added state to path %d", pathIndex);

	}

	//not used
	void PathManager::appendStateAfterReshadeSnapshot(const std::string& pathName, const int snapshotIndex) {
		if (!reshadeAddonisConnected()) {
			MessageHandler::logDebug("IGCSConnector::appendStateAfterReshadeSnapshot - Not connected to IgcsConnector addon");
			return;
		}

		if (!pathIndexMap.contains(pathName)) {
			MessageHandler::logError("IGCSConnector::appendStateAfterReshadeSnapshot - Path not found: %s", pathName.c_str());
			return;
		}

		int pathIndex = pathIndexMap[pathName];

		_appendStateSnapshotAfterSnapshotOnPath(pathIndex, snapshotIndex);
		MessageHandler::logDebug("IGCSConnector::appendStateAfterSnapshot - Added state after index %d on path %d",
			snapshotIndex, pathIndex);

	}

	void PathManager::insertStateBeforeReshadeSnapshot(const std::string& pathName, const int snapshotIndex) {
		if (!reshadeAddonisConnected()) {
			MessageHandler::logDebug("IGCSConnector::insertStateBeforeReshadeSnapshot - Not connected to IgcsConnector addon");
			return;
		}

		if (!pathIndexMap.contains(pathName)) {
			MessageHandler::logError("IGCSConnector::insertStateBeforeReshadeSnapshot - Path not found: %s", pathName.c_str());
			return;
		}

		int pathIndex = pathIndexMap[pathName];

		_insertStateSnapshotBeforeSnapshotOnPath(pathIndex, snapshotIndex);
		MessageHandler::logDebug("IGCSConnector::insertStateBeforeSnapshot - Inserted state before index %d on path %d",
			snapshotIndex, pathIndex);
	}

	void PathManager::removeReshadeState(const std::string& pathName, const int stateIndex) {
		if (!reshadeAddonisConnected()) {
			MessageHandler::logDebug("IGCSConnector::removeReshadeState - Not connected to IgcsConnector addon");
			return;
		}

		if (!pathIndexMap.contains(pathName)) {
			MessageHandler::logError("IGCSConnector::removeReshadeState - Path not found: %s", pathName.c_str());
			return;
		}

		int pathIndex = pathIndexMap[pathName];

		_removeStateSnapshotFromPath(pathIndex, stateIndex);
		MessageHandler::logDebug("IGCSConnector::removeState - Removed state %d from path %s (IGCS Path Index: %d)",
			stateIndex, pathName.c_str(), pathIndex);
	}

	void PathManager::removeReshadePath(const std::string& pathName) {
		if (!reshadeAddonisConnected()) {
			MessageHandler::logDebug("IGCSConnector::removeReshadePath - Not connected to IgcsConnector addon");
			return;
		}

		if (!pathIndexMap.contains(pathName)) {
			MessageHandler::logError("IGCSConnector::removeReshadePath - Path not found: %s", pathName.c_str());
			return;
		}

		int pathIndex = pathIndexMap[pathName];

		_removeCameraPath(pathIndex);
		MessageHandler::logDebug("IGCSConnector::removeReshadePath - Removed path: %s - IGCS index %d", pathName.c_str(), pathIndex);
	}

	void PathManager::clearAllReshadelPaths() {
		if (!reshadeAddonisConnected()) {
			MessageHandler::logDebug("IGCSConnector::clearAllPaths - Not connected to IgcsConnector addon");
			return;
		}

		_clearPaths();
		MessageHandler::logDebug("IGCSConnector::clearAllPaths - Cleared all paths");
	}

	void PathManager::applyReshadeState(const std::string& pathName, const int stateIndex) {
		if (!reshadeAddonisConnected()) {
			MessageHandler::logDebug("IGCSConnector::applyReshadeState - Not connected to IgcsConnector addon");
			return;
		}

		if (!pathIndexMap.contains(pathName)) {
			MessageHandler::logError("IGCSConnector::applyReshadeState - Path not found: %s", pathName.c_str());
			return;
		}

		const int pathIndex = pathIndexMap[pathName];

		_setReshadeState(pathIndex, stateIndex);
		MessageHandler::logDebug("IGCSConnector::applyReshadeState - Applied state %d from path %s (index: %d)",
			stateIndex, pathName.c_str(), pathIndex);
	}

	void PathManager::applyInterpolatedReshadeState(const std::string& pathName, const int fromState, const int toState, const float factor) {
		if (!reshadeAddonisConnected()) {
			MessageHandler::logDebug("IGCSConnector::applyInterpolatedReshadeState - Not connected to IgcsConnector addon");
			return;
		}

		if (!pathIndexMap.contains(pathName)) {
			MessageHandler::logError("IGCSConnector::applyState - Path not found: %s", pathName.c_str());
			return;
		}

		int pathIndex = pathIndexMap[pathName];
		_setReshadeStateInterpolated(pathIndex, fromState, toState, factor);
	}

	void PathManager::updateReshadeState(const std::string& pathName, const int stateIndex) {
		if (!reshadeAddonisConnected()) {
			MessageHandler::logDebug("IGCSConnector::updateReshadeState - Not connected to IgcsConnector addon");
			return;
		}

		if (!pathIndexMap.contains(pathName)) {
			MessageHandler::logError("IGCSConnector::applyState - Path not found: %s", pathName.c_str());
			return;
		}

		int pathIndex = pathIndexMap[pathName];

		_updateStateSnapshotOnPath(pathIndex, stateIndex);

		MessageHandler::logDebug("IGCSConnector::updateState - Updated state %d on path %d",
			stateIndex, pathIndex);
	}

	void PathManager::rebuildPathIndices() {
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
