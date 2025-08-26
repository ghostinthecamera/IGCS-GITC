#include "stdafx.h"
#include "CameraPath.h"
#include "Camera.h"
#include "Utils.h"
#include "CameraManipulator.h"
#include "Globals.h"
#include "MessageHandler.h"
#include <algorithm>
#include <cmath>
#include "Console.h"
#include "CatmullRom.h"
#include "CentripetalCatmullRom.h"
#include "PathUtils.h"
#include "RiemannCubic.h"
#include "Bezier.h"
#include "BSpline.h"
#include "Cubic.h"
#include "D3D12Hook.h"

namespace IGCS
{

	using namespace DirectX;

	//Initialise static members
	InterpolationMode CameraPath::_interpolationMode = InterpolationMode::CatmullRom;
	ParameterMode CameraPath::_parameterMode = ParameterMode::ArcLengthUniformSpeed;
	RotationMode CameraPath::_rotationMode = RotationMode::Standard;
	EasingType CameraPath::_easingType = EasingType::None;
	int CameraPath::_samplesize = 256;


	CameraPath::CameraPath(string pathName) :
		_pathName(move(pathName))
	{
	}

	CameraPath::CameraPath()
	{
	}

	void CameraPath::generateArcLengthTables()
	{
		_samplesize = Globals::instance().settings().sampleCountSetting;

		if (_nodes.size() < 2)
		{
			//MessageHandler::logDebug("Not enough nodes, returning from arc table generation");
			return;
		}

		CatmullRomGenerateArcTable(_arcLengthTable, _paramTable, _nodes, _samplesize, *this);
		CentripetalGenerateArcTable(_arcLengthTableCentripetal, _paramTableCentripetal, _nodes, _samplesize, *this);
		BezierGenerateArcLengthTable(_arcLengthTableBezier, _paramTableBezier, _nodes, _samplesize);
		CubicGenerateArcLengthTable(_arcLengthTableCubic, _paramTableCubic, _nodes, _samplesize, *this);

		if (_nodes.size() < 4)
		{
			//MessageHandler::logDebug("Not enough nodes for Bspline, returning from arc table generation");
			return;
		}

		BSplineGenerateArcLengthTable(_arcLengthTableBspline, _paramTableBspline, _nodes, _samplesize, _knotVector, _controlPointsPos, _controlPointsRot, _controlPointsFOV);

	}

	void CameraPath::addNode(const XMVECTOR position, const XMVECTOR rotation, float fov)
	{
		D3DHookChecks();

		_nodes.emplace_back(position, rotation, fov);
		PathManager::instance().appendStateToReshadePath(_pathName);
		generateArcLengthTables();
	}

	uint8_t CameraPath::addNode()
	{
		auto node = createNode();

		D3DHookChecks();

		// Add the new node
		_nodes.emplace_back(node);

		//update reshade state if connected
		PathManager::instance().appendStateToReshadePath(_pathName);

		// Update and log
		const uint8_t newNodeIndex = static_cast<uint8_t>(_nodes.size()) - 1;
		printNodeDetails(newNodeIndex);
		//MessageHandler::logDebug("CameraPath::addNode: Node added");
		MessageHandler::logLine("Node %zu added to path: %s", newNodeIndex, _pathName.c_str());

		continuityCheck();

		// Recalculate path metrics
		generateArcLengthTables();

		return newNodeIndex;
	}

	CameraNode CameraPath::createNode()
	{
		// Get current camera state
		const auto p = GameSpecific::CameraManipulator::getCurrentCameraCoordsVector();
		const auto r = Camera::instance().getToolsQuaternionVector();
		const auto f = GameSpecific::CameraManipulator::getCurrentFoV();

		return { p, r, f };
	}

	void CameraPath::continuityCheck()
	{
		if (_nodes.size() < 2)
		{
			MessageHandler::logDebug("Not enough nodes for continuity check");
			return;
		}
		for (size_t i = 1; i < _nodes.size(); ++i)
		{
			PathUtils::EnsureQuaternionContinuity(_nodes[i - 1].rotation, _nodes[i].rotation);
		}
	}

	void CameraPath::insertNodeBefore(const uint8_t nodeIndex)
	{
		// Check if path is empty
		if (_nodes.empty()) {
			MessageHandler::logDebug("CameraPath::insertNodeBefore: No nodes to insert before");
			MessageHandler::logLine("No nodes to insert before: %s", _pathName.c_str());
			return;
		}

		// Validate node index
		if (nodeIndex >= _nodes.size()) {
			MessageHandler::logDebug("CameraPath::insertNodeBefore: nodeIndex out of bounds. Returning");
			MessageHandler::logError("Selected node %d out of bounds for path: %s", nodeIndex, _pathName.c_str());
			return;
		}


		const auto node = createNode();

		D3DHookChecks();

		_nodes.insert(_nodes.begin() + nodeIndex, node);

		MessageHandler::logDebug("CameraPath::insertNodeBefore: Node inserted before %d", nodeIndex);
		MessageHandler::logLine("Node inserted before %d in path: %s", nodeIndex, _pathName.c_str());

		PathManager::instance().insertStateBeforeReshadeSnapshot(_pathName, nodeIndex);
		continuityCheck();
		generateArcLengthTables();
	}

	void CameraPath::deleteNode(uint8_t nodeIndex)
	{
		// Check if path is empty
		if (_nodes.empty()) {
			MessageHandler::logDebug("CameraPath::deleteNode: No nodes to delete from path");
			//MessageHandler::logLine("No nodes to delete from path: %s", _pathName.c_str());
			return;
		}

		// Convert special value 255 to last node index
		if (nodeIndex == 255) {
			nodeIndex = static_cast<uint8_t>(_nodes.size() - 1);
		}

		// Validate node index
		if (nodeIndex >= _nodes.size()) {
			MessageHandler::logDebug("CameraPath::deleteNode: nodeIndex out of bounds. Returning");
			MessageHandler::logError("Selected node %zu out of bounds for path: %s", nodeIndex, _pathName.c_str());
			return;
		}

		D3DHookChecks();

		_nodes.erase(_nodes.begin() + nodeIndex);
		MessageHandler::logDebug("CameraPath::deleteNode: Node %zu deleted", nodeIndex);

		//delete from reshade if available
		PathManager::instance().removeReshadeState(_pathName, nodeIndex);

		MessageHandler::logLine("Node %zu deleted from path: %s", nodeIndex, _pathName.c_str());
		continuityCheck();
		generateArcLengthTables();
	}

	void CameraPath::D3DHookChecks()
	{
		if (D3DMODE == D3DMODE::DX11 && !Globals::instance().settings().d3ddisabled && D3DHook::instance().isVisualizationEnabled()) {
			D3DHook::instance().markResourcesForUpdate();
		}

		if (D3DMODE == D3DMODE::DX12 && !Globals::instance().settings().d3ddisabled && D3D12Hook::instance().isVisualisationEnabled()) {
			//DX12 mode code here
		}
	}

	void CameraPath::updateNode(uint8_t nodeIndex)
	{
		if (nodeIndex < _nodes.size())
		{
			XMFLOAT3 pos = Camera::instance().getInternalPosition();
			XMFLOAT4 rot = Camera::instance().getToolsQuaternion();
			float fov = GameSpecific::CameraManipulator::getCurrentFoV();
			_nodes[nodeIndex].position = XMLoadFloat3(&pos);
			_nodes[nodeIndex].rotation = XMLoadFloat4(&rot);
			_nodes[nodeIndex].fov = fov;

			//update reshade state if connected
			PathManager::instance().updateReshadeState(_pathName, nodeIndex);
			continuityCheck();
			generateArcLengthTables();

			MessageHandler::logDebug("CameraPath::updateNode::Node %zu updated. Arc Tables Regenerated.", nodeIndex);
			MessageHandler::logLine("Node %zu updated of path: %s", nodeIndex, _pathName.c_str());
		}
		else
		{
			MessageHandler::logError("CameraPath::updateNode::nodeIndex out of bounds. Returning");
		}
	}


	void CameraPath::printNodeDetails(size_t i) const
	{
		XMFLOAT3 nPos;
		XMFLOAT4 nRot;

		XMStoreFloat3(&nPos, _nodes[i].position);
		XMStoreFloat4(&nRot, _nodes[i].rotation);
		const float nFov = _nodes[i].fov;

		//MessageHandler::logDebug("Path: %s", _pathName.c_str());
		//MessageHandler::logDebug("Node %zu created", i);
		//MessageHandler::logDebug("Position: %f, %f, %f", nPos.x, nPos.y, nPos.z);
		//MessageHandler::logDebug("Rotation: %f, %f, %f, %f", nRot.x, nRot.y, nRot.z, nRot.z);
		//MessageHandler::logDebug("FOV: %f", nFov);
	}

	void CameraPath::interpolateNodes(float deltaTime)
	{
		PathManager& currPM = PathManager::instance();
		getSettings();

		// Update vehicle speed tracking
		updateVehicleSpeedTracking(deltaTime);

		if (_nodes.size() < 2)
		{
			MessageHandler::logDebug("CameraPath::interpolatenodes:Not enough nodes. Exiting.");
			currPM.handleStopPathMessage();
			return;
		}

		setArcLengthTables();

		if (totalDist < 1e-6f)
		{
			MessageHandler::logDebug("CameraPath::interpolatenodes:Total path length too small. Exiting.");
			currPM.handleStopPathMessage();
			return;
		}

		if (_totalDuration < 1e-6f)
		{
			MessageHandler::logDebug("CameraPath::interpolatenodes:Invalid total duration. Exiting.");
			currPM.handleStopPathMessage();
			return;
		}

		// ----------------------------------------------------------------------
		// Update _currentArcLength by uniform speed in space
		// ----------------------------------------------------------------------

		//const float globalT = getGlobalT(deltaTime); // the final parameter we feed into the interpolation
		const float globalT = getGlobalT(deltaTime, currPM._pathManagerStatus);

		// ----------------------------------------------------------------------
		// Interpolate position, rotation, FOV
		// ----------------------------------------------------------------------

		performPositionInterpolation(globalT);
		performRotationInterpolation(globalT);

		// Apply path look-at override if enabled
		applyPathLookAt(deltaTime);

		updateReshadeStateController(globalT);

		// Apply shake if enabled
		if (_applyShake) {
			_shakeTime += deltaTime;
			applyShakeEffect();
		}

		// Apply handheld camera effect if enabled (our new implementation)
		if (_enableHandheldCamera) {
			_handheldTimeAccumulator += deltaTime;
			applyHandheldCameraEffect(deltaTime);
		}

		// ----------------------------------------------------------------------
		// Apply relative mode offset if enabled
		// ----------------------------------------------------------------------
		if (_relativeToPlayerEnabled)
		{
			if (_isFirstFrame) initializeRelativeTracking(); // Initialize relative tracking on first frame if not already done
			interpolatedPosition = applySmoothPlayerOffset(interpolatedPosition, deltaTime);
		}

		// Write to game
		GameSpecific::CameraManipulator::writeNewCameraValuesToGameData(interpolatedPosition, interpolatedRotation);
		GameSpecific::CameraManipulator::changeFoV(interpolatedFOV);

		_currentProgress = getProgress();

		if (PathManager::instance()._pathManagerStatus == Idle)
		{
			XMFLOAT3 position;
			const XMFLOAT3 eulers = Utils::QuaternionToEulerAngles(interpolatedRotation, MULTIPLICATION_ORDER);
			XMStoreFloat3(&position, interpolatedPosition);
			Camera::instance().setAllRotation(eulers);
			Camera::instance().setInternalPosition(position);
			Camera::instance().setFoV(interpolatedFOV, true);
			Camera::instance().resetMovement();
			Camera::instance().resetTargetMovement();
		}
	}

	void CameraPath::updateReshadeStateController(const float& globalT) const
	{
		if (!PathManager::instance().reshadeAddonisConnected()) 
			return;

		const int seg = getsegmentIndex(globalT);
		const int nextSeg = min(seg + 1, static_cast<int>(_nodes.size() - 1));
		float localt = globalT - static_cast<float>(seg);
		localt = max(0.0f, min(1.0f, localt));
		PathManager::instance().applyInterpolatedReshadeState(_pathName, seg, nextSeg, localt);
	}

	void CameraPath::getSettings()
	{
		// Get settings reference to avoid repeated calls
		static auto& s = Globals::instance().settings();

		// Cache new values for comparison and use
		const auto newInterpMode = static_cast<InterpolationMode>(s.positionInterpMode);
		const auto newRotMode = static_cast<RotationMode>(s._rotationMode);
		const auto newParamMode = static_cast<ParameterMode>(s.uniformParamEnabled);
		const bool newRelativeToPlayerEnabled = s.isRelativePlayerEnabled;

		// Log changes if detected
		//if (_parameterMode != newParamMode)
		//	MessageHandler::logDebug("Parameter mode changing to %u", s.uniformParamEnabled);

		//if (_interpolationMode != newInterpMode)
		//	MessageHandler::logDebug("CameraPath::safeInterpolationModeChange: Switched to mode %d",
		//		static_cast<int>(s.positionInterpMode));

		//if (_rotationMode != newRotMode)
		//	MessageHandler::logDebug("CameraPath::safeInterpolationModeChange: Switched to mode %d",
		//		static_cast<int>(s._rotationMode));

		// Update path configuration settings
		_interpolationMode = newInterpMode;
		_rotationMode = newRotMode;
		Utils::setIfChanged(_parameterMode, newParamMode);
		_easingType = static_cast<EasingType>(s.easingtype);

		// Update camera shake settings
		_applyShake = s.isCameraShakeEnabled;
		_shakeAmplitude = s.shakeAmplitude;
		_shakeFrequency = s.shakeFrequency;

		// Update handheld camera settings
		_enableHandheldCamera = s.isHandheldEnabled;
		_handheldIntensity = s.handheldIntensity;
		_handheldDriftIntensity = s.handheldDriftIntensity;
		_handheldJitterIntensity = s.handheldJitterIntensity;
		_handheldBreathingIntensity = s.handheldBreathingIntensity;
		_handheldBreathingRate = s.handheldBreathingRate; 
		_handheldPositionEnabled = s.handheldPositionToggle;
		_handheldRotationEnabled = s.handheldRotationToggle;
		_handheldDriftSpeed = s.handheldDriftSpeed;
		_handheldRotationDriftSpeed = s.handheldRotationDriftSpeed;

		// Update player-related settings
		setPlayerPosition(GameSpecific::CameraManipulator::getCurrentPlayerPosition());
		Utils::callOnChange(_relativeToPlayerEnabled, newRelativeToPlayerEnabled, [this](auto t) {setRelativeToPlayerEnabled(t); });
		//if (_relativeToPlayerEnabled != newRelativeToPlayerEnabled)
		//	setRelativeToPlayerEnabled(newRelativeToPlayerEnabled);
		_offsetSmoothingFactor = s.relativePathSmoothness;

		_pathLookAtEnabled = s.pathLookAtEnabled;
		_pathLookAtOffset = { s.pathLookAtOffsetX, s.pathLookAtOffsetY, s.pathLookAtOffsetZ };
		_pathLookAtSmoothness = s.pathLookAtSmoothness;

		_pathSpeedMatchingEnabled = s.pathSpeedMatchingEnabled;
		_pathSpeedScale = s.pathSpeedScale;
		_pathSpeedSmoothness = s.pathSpeedSmoothness;
		_pathMinSpeed = s.pathMinSpeed;
		_pathMaxSpeed = s.pathMaxSpeed;
		_pathBaselineSpeed = s.pathBaselineSpeed;
	}

	float CameraPath::getGlobalT(const float& deltaTime, const PathManagerStatus currentState)
	{
		const float easingValue = Globals::instance().settings().easingValueSetting;

		// Apply speed multiplier to deltaTime for vehicle speed matching
		const float adjustedDeltaTime = deltaTime * _currentPathSpeedMultiplier;

		switch (_parameterMode)
		{
		case ParameterMode::UniformTime: // --- UNIFORM-TIME–BASED UPDATE ---
		{
			float progress;
			if (currentState == Scrubbing)
			{
				// Apply easing to scrubbing progress too
				progress = _currentProgress;
			}
			else
			{
				_currentTime += adjustedDeltaTime;  // USE ADJUSTED DELTA TIME
				progress = static_cast<float>(_currentTime) / static_cast<float>(_totalDuration);

				if (progress >= 1.0f)
				{
					progress = 1.0f;  // clamp
					PathManager::instance().handleStopPathMessage();
					XMFLOAT3 eulers = Utils::QuaternionToEulerAngles(getNodeRotation(_nodes.size() - 1), MULTIPLICATION_ORDER);
					Camera::instance().setAllRotation(eulers);
					GameSpecific::CameraManipulator::changeFoV(_nodes.at(_nodes.size() - 1).fov);
				}
			}

			// Apply easing to both scrubbing and normal playback
			const float easedT = PathUtils::applyEasing(progress, _easingType, easingValue);
			const float maxParam = static_cast<float>(_nodes.size() - 1);
			return easedT * maxParam;
		}

		case ParameterMode::ArcLengthUniformSpeed: // --- ARC-LENGTH–BASED UPDATE ---
		{
			float progress;
			if (currentState == Scrubbing)
			{
				// Apply easing to scrubbing progress too
				progress = _currentProgress;
			}
			else
			{
				const float speed = totalDist / static_cast<float>(_totalDuration);
				_currentArcLength += speed * adjustedDeltaTime;  // USE ADJUSTED DELTA TIME

				if (_currentArcLength >= totalDist)
				{
					_currentArcLength = totalDist;
					PathManager::instance().handleStopPathMessage();
					XMFLOAT3 eulers = Utils::QuaternionToEulerAngles(getNodeRotation(_nodes.size() - 1), MULTIPLICATION_ORDER);
					Camera::instance().setAllRotation(eulers);
					GameSpecific::CameraManipulator::changeFoV(_nodes.at(_nodes.size() - 1).fov);
				}

				progress = _currentArcLength / totalDist;
			}

			// Apply easing to both scrubbing and normal playback
			const float easedT = PathUtils::applyEasing(progress, _easingType, easingValue);
			return getGlobalTForDistance(easedT * totalDist, *arcTable, *paramTable);
		}
		}
		return 0.0f;
	}

	void CameraPath::setArcLengthTables()
	{
		// Select appropriate tables based on interpolation mode
		const std::vector<float>* selectedArcTable;
		const std::vector<float>* selectedParamTable;

		switch (_interpolationMode)
		{
		case InterpolationMode::Centripetal:
			selectedArcTable = &_arcLengthTableCentripetal;
			selectedParamTable = &_paramTableCentripetal;
			break;
		case InterpolationMode::Bezier:
			selectedArcTable = &_arcLengthTableBezier;
			selectedParamTable = &_paramTableBezier;
			break;
		case InterpolationMode::BSpline:
			selectedArcTable = &_arcLengthTableBspline;
			selectedParamTable = &_paramTableBspline;
			break;
		case InterpolationMode::Cubic:
			selectedArcTable = &_arcLengthTableCubic;
			selectedParamTable = &_paramTableCubic;
			break;
		case InterpolationMode::CatmullRom:
		default:
			selectedArcTable = &_arcLengthTable;
			selectedParamTable = &_paramTable;
			break;
		}

		// Check if the selected arc table is empty (only do this once)
		if (selectedArcTable->empty()) {
			// Could add logging here if needed
			return;
		}

		// Set the active tables and calculate total distance (only do this once)
		arcTable = selectedArcTable;
		paramTable = selectedParamTable;
		totalDist = arcTable->back();
	}

	void CameraPath::performPositionInterpolation(float globalT)
	{
		// If we're in BSpline mode but have fewer than 4 nodes, fallback to CatmullRom.
		if (_interpolationMode == InterpolationMode::BSpline && _nodes.size() < 4)
		{
			MessageHandler::logLine("PathController - Not enough nodes for B-Spline playback, falling back to CatmullRom");
			interpolatedPosition = CatmullRomPositionInterpolation(globalT, _nodes, *this);
			interpolatedFOV = CatmullRomFoV(globalT, _nodes, *this);
			return;
		}

		switch (_interpolationMode)
		{
		case InterpolationMode::Cubic:
			interpolatedPosition = CubicPositionInterpolation_Smooth(globalT, _nodes, *this);
			interpolatedFOV = CubicFoV_Smooth(globalT, _nodes, *this);
			break;

		case InterpolationMode::BSpline:
			// We already handled the “not enough nodes” case above.
			interpolatedPosition = BSplinePositionInterpolation(globalT, _knotVector, _controlPointsPos);
			interpolatedFOV = BSplineFoV(globalT, _knotVector, _controlPointsFOV);
			break;

		case InterpolationMode::Bezier:
			interpolatedPosition = BezierPositionInterpolation(globalT, _nodes);
			interpolatedFOV = BezierFoV(globalT, _nodes);
			break;

		case InterpolationMode::Centripetal:
			interpolatedPosition = CentripetalPositionInterpolation(globalT, _nodes, *this);
			interpolatedFOV = CatmullRomFoV(globalT, _nodes, *this);
			break;

			// CatmullRom is also the default fallback
		case InterpolationMode::CatmullRom:
		default:
			interpolatedPosition = CatmullRomPositionInterpolation(globalT, _nodes, *this);
			interpolatedFOV = CatmullRomFoV(globalT, _nodes, *this);
			break;
		}
	}

	void CameraPath::performRotationInterpolation(float globalT)
	{
		// If rotation mode is Standard, interpolation is BSpline, but < 4 nodes, fallback to CatmullRom
		if (_rotationMode == RotationMode::Standard &&
			_interpolationMode == InterpolationMode::BSpline &&
			_nodes.size() < 4)
		{
			MessageHandler::logLine("PathController - Not enough nodes for B-Spline playback");
			interpolatedRotation = CatmullRomRotationInterpolation(globalT, _nodes, *this);
			return;
		}

		switch (_rotationMode)
		{
		case RotationMode::Standard:
		{
			switch (_interpolationMode)
			{
			case InterpolationMode::CatmullRom:
				interpolatedRotation = CatmullRomRotationInterpolation(globalT, _nodes, *this);
				break;

			case InterpolationMode::Centripetal:
				interpolatedRotation = CentripetalRotationInterpolation(globalT, _nodes, *this);
				break;

			case InterpolationMode::Bezier:
				interpolatedRotation = BezierRotationInterpolation(globalT, _nodes);
				break;

			case InterpolationMode::BSpline:
				// Already handled the “not enough nodes” fallback above
				interpolatedRotation = BSplineRotationInterpolation(globalT, _knotVector, _controlPointsRot);
				break;

			case InterpolationMode::Cubic:
				interpolatedRotation = CubicRotationInterpolation(globalT, _nodes, *this);
				break;

			default:
				// Default fallback to CatmullRom
				interpolatedRotation = CatmullRomRotationInterpolation(globalT, _nodes, *this);
				break;
			}
			break;
		}
		case RotationMode::SQUAD:
			interpolatedRotation = PathUtils::SquadRotationInterpolation(globalT, _nodes);
			break;

		case RotationMode::RiemannCubic:
			interpolatedRotation = RiemannCubicRotationMonotonic(globalT, _nodes, *this);
			break;

		default:
			// Default to CatmullRom
			interpolatedRotation = CatmullRomRotationInterpolation(globalT, _nodes, *this);
			break;
		}
	}


	int CameraPath::getsegmentIndex(float globalT) const
	{
		float epsilon = 1e-5f;

		// 1) Calculate floor with a small epsilon
		int segmentIndex = static_cast<int>(floorf(globalT - epsilon));

		// 2) Compute how many segments we have
		int totalSegments = static_cast<int>(_nodes.size()) - 1;

		// 3) Clamp segmentIndex to [0..(totalSegments - 1)]
		segmentIndex = max(0, min(segmentIndex, totalSegments - 1));

		return segmentIndex;
	}

	void CameraPath::getnodeSequence(int& i0, int& i1, int& i2, int& i3, float globalT) const
	{
		int i = getsegmentIndex(globalT);
		// Figure out which segment [i..i+1], plus local t
		i0 = max(i - 1, 0);
		i1 = i;
		i2 = min(i + 1, (int)_nodes.size() - 1);
		i3 = min(i + 2, (int)_nodes.size() - 1);
	}

	void CameraPath::setTotalDuration(float duration)
	{
		_totalDuration = duration;
		// rebuild table if nodes exist
		generateArcLengthTables();
	}

	void CameraPath::resetPath()
	{
		_currentNode = 0;
		_currentTime = 0.0;
		_currentArcLength = 0.0f;
		totalDist = 0.0f;
		std::vector<float>* arcTable = nullptr;
		std::vector<float>* paramTable = nullptr;
		_shakeTime = 0.0f;

		// Reset handheld camera values
		_handheldTimeAccumulator = 0.0f;
		_handheldPositionVelocity = XMVectorZero();
		_handheldRotationVelocity = XMVectorZero();

		// Reset relative mode tracking
		_isFirstFrame = true;
		_initialPlayerPosition = XMVectorZero();
		_currentPlayerOffset = XMVectorZero(); // Reset the smooth offset

		_pathLookAtInitialized = false;
		_currentPathLookAtRotation = XMQuaternionIdentity();

		resetSpeedMatching();
	}

	XMFLOAT3 CameraPath::getNodePositionFloat(size_t i) const
	{
		XMFLOAT3 pos;
		XMStoreFloat3(&pos, _nodes.at(i).position);
		return pos;
	}


	float CameraPath::getGlobalTForDistance(float distance, const std::vector<float>& arcTable, const std::vector<float>& paramTable)
	{
		// Ensure the tables are not empty
		if (arcTable.empty() || paramTable.empty())
			return 0.0f;

		float totalDist = arcTable.back();

		// Clamp the distance to the bounds of the table
		if (distance <= 0.0f)
			return paramTable.front();
		if (distance >= totalDist)
			return paramTable.back();

		// Use binary search to find the interval for the given distance
		auto it = ranges::lower_bound(arcTable.begin(), arcTable.end(), distance);
		size_t idx = std::distance(arcTable.begin(), it);

		// If the index is 0, return the first parameter value
		if (idx == 0)
			return paramTable.front();

		// Get the surrounding distance and parameter values
		float d0 = arcTable[idx - 1];
		float d1 = arcTable[idx];
		float p0 = paramTable[idx - 1];
		float p1 = paramTable[idx];

		// Calculate the fractional position between d0 and d1
		float segLen = d1 - d0;
		float frac = (segLen < 1e-6f) ? 0.0f : (distance - d0) / segLen;

		// Return the interpolated parameter
		//return p0 + frac * (p1 - p0);
		return PathUtils::lerpFMA(p0, p1, frac);
	}

	//Camerapath.cpp
	//// Apply shake effect to position and rotation
	void CameraPath::applyShakeEffect()
	{
		// Determine progress based on parameter mode
		float progress = (_parameterMode == ParameterMode::UniformTime) ? static_cast<float>(_currentTime / _totalDuration)
			: (_currentArcLength / totalDist);
		progress = max(0.0f, min(progress, 1.0f));

		// Apply easing to progress
		float easingFactor = PathUtils::applyEasing(progress, _easingType, Globals::instance().settings().easingValueSetting);

		// Ensure shake is never fully disabled
		float minShake = 0.3f;
		float easedShakeAmplitude = _shakeAmplitude * (minShake + (easingFactor * (1.0f - minShake)));

		// POSITION SCALE FACTOR - matching Camera.cpp
		const float positionAmplitudeScale = 0.0005f;
		const float positionFrequencyScale = 10.0f;

		// Scale the amplitude and frequency for position
		float scaledAmplitude = easedShakeAmplitude * positionAmplitudeScale;
		float scaledFrequency = _shakeFrequency * positionFrequencyScale;

		// Generate smooth noise-based offsets with subtle jitter blending
		float noiseX = PathUtils::smoothNoise(_shakeTime, scaledFrequency) + (PathUtils::randomJitter() * 0.2f);
		float noiseY = PathUtils::smoothNoise(_shakeTime + 12.3f, scaledFrequency * 1.2f) + (PathUtils::randomJitter() * 0.2f);
		float noiseZ = PathUtils::smoothNoise(_shakeTime + 25.7f, scaledFrequency * 0.8f) + (PathUtils::randomJitter() * 0.2f);

		// Apply shake offset with balanced randomness
		XMVECTOR shakeOffset = XMVectorSet(
			noiseX * scaledAmplitude,
			noiseY * scaledAmplitude,
			noiseZ * scaledAmplitude,
			0.0f
		);

		// Apply to final interpolated position
		interpolatedPosition = XMVectorAdd(interpolatedPosition, shakeOffset);

		// ROTATION SCALE FACTOR - matching Camera.cpp
		const float rotationAmplitudeScale = 0.1f;
		const float rotationFrequencyScale = 10.0f;

		float rotationAmplitude = easedShakeAmplitude * rotationAmplitudeScale;
		float rotationFrequency = _shakeFrequency * rotationFrequencyScale;

		// Apply slightly different noise to rotation for more organic movement
		XMVECTOR shakeRotation = Utils::generateEulerQuaternion(XMFLOAT3(
			(PathUtils::smoothNoise(_shakeTime, rotationFrequency) + (PathUtils::randomJitter() * 0.1f)) * rotationAmplitude * 0.55f,
			(PathUtils::smoothNoise(_shakeTime + 5.0f, rotationFrequency * 1.5f) + (PathUtils::randomJitter() * 0.1f)) * rotationAmplitude * 0.4f,
			(PathUtils::smoothNoise(_shakeTime + 15.0f, rotationFrequency * 0.7f) + (PathUtils::randomJitter() * 0.1f)) * rotationAmplitude * 0.05f)
		);

		// Multiply quaternions to apply shake rotation
		interpolatedRotation = XMQuaternionMultiply(interpolatedRotation, shakeRotation);
		interpolatedRotation = XMQuaternionNormalize(interpolatedRotation);
	}

	// CameraPath.cpp - Updated applyHandheldCameraEffect
	void CameraPath::applyHandheldCameraEffect(const float deltaTime)
	{
		// Determine progress based on parameter mode
		float progress = (_parameterMode == ParameterMode::UniformTime) ?
			static_cast<float>(_currentTime / _totalDuration) : (_currentArcLength / totalDist);
		progress = max(0.0f, min(progress, 1.0f));

		// Apply easing to progress based on the current easing type
		float easedProgress = PathUtils::applyEasing(progress, _easingType, Globals::instance().settings().easingValueSetting);
		float intensityCurve = easedProgress;

		// Dynamic intensity modulation based on camera motion
		static XMVECTOR prevPosition = interpolatedPosition;
		float movementMagnitude = XMVectorGetX(XMVector3Length(XMVectorSubtract(interpolatedPosition, prevPosition))) / deltaTime;
		prevPosition = interpolatedPosition;

		// Adjust intensity based on camera movement
		float movementMultiplier = 1.0f + min(movementMagnitude * 0.05f, 1.0f);

		// Base intensity affects overall effect - matching Camera.cpp
		const float posBaseIntensityScalar = 7.0f;
		float posEffectiveIntensity = _handheldIntensity * intensityCurve * movementMultiplier * posBaseIntensityScalar;

		// POSITION SCALE FACTORS - matching Camera.cpp
		const float positionDriftIntensityScale = 0.005f;
		const float positionJitterIntensityScale = 0.0003f;
		const float positionBreathingIntensityScale = 0.01f;
		const float positionDriftSpeedScale = 10.0f;

		// Separate component intensities with proper scaling
		const float scaledDriftIntensity = _handheldDriftIntensity * posEffectiveIntensity * positionDriftIntensityScale;
		const float scaledJitterIntensity = _handheldJitterIntensity * posEffectiveIntensity * positionJitterIntensityScale;
		const float scaledBreathingIntensity = _handheldBreathingIntensity * positionBreathingIntensityScale;
		const float scaledDriftSpeed = _handheldDriftSpeed * posEffectiveIntensity * positionDriftSpeedScale;

		// Handle position effects - matching Camera.cpp approach
		if (_handheldPositionEnabled) {
			// DRIFT: Use simulateHandheldCamera with only drift enabled
			XMVECTOR driftOffset = PathUtils::simulateHandheldCamera(
				deltaTime,
				scaledDriftIntensity,
				0.0f,                         // No jitter in drift pass
				scaledBreathingIntensity,
				_handheldPositionVelocity,
				scaledDriftSpeed
			);

			// JITTER: Use simulateHandheldCamera with only jitter enabled
			static XMVECTOR jitterVelocity = XMVectorZero();
			XMVECTOR jitterOffset = PathUtils::simulateHandheldCamera(
				deltaTime * 5.0f,             // Speed up time for jitter
				0.0f,                         // No drift in jitter pass
				scaledJitterIntensity,
				0.0f,                         // No breathing in jitter
				jitterVelocity,
				20.0f                         // High frequency
			);

			// Combine both offsets
			XMVECTOR totalOffset = XMVectorAdd(driftOffset, jitterOffset);

			// Apply position offset
			interpolatedPosition = XMVectorAdd(interpolatedPosition, totalOffset);
		}

		// Rotation base intensity - matching Camera.cpp
		const float rotBaseIntensityScalar = 25.0f;
		float rotEffectiveIntensity = _handheldIntensity * intensityCurve * (movementMultiplier * 0.2f) * rotBaseIntensityScalar;

		// Handle rotation effects - matching Camera.cpp approach
		if (_handheldRotationEnabled) {
			// ROTATION SCALE FACTORS - matching Camera.cpp
			const float rotationDriftIntensityScale = 1.0f;
			const float rotationJitterIntensityScale = 0.02f;
			const float rotationBreathingRateScale = 25.0f;
			const float rotationDriftSpeedScale = 1.0f;

			// Separate rotational intensities
			float rotDriftIntensity = _handheldDriftIntensity * rotEffectiveIntensity * rotationDriftIntensityScale;
			float rotJitterIntensity = _handheldJitterIntensity * rotEffectiveIntensity * rotationJitterIntensityScale;
			float breathingRateIntensity = _handheldBreathingRate * rotationBreathingRateScale;
			float rotDriftSpeedIntensity = _handheldRotationDriftSpeed * rotationDriftSpeedScale;

			// Generate DRIFT with slow time accumulation
			XMVECTOR driftRotation = PathUtils::generateHandheldRotationNoise(
				_handheldTimeAccumulator * 0.5f,  // SLOW time for drift
				rotDriftIntensity,
				breathingRateIntensity,
				rotDriftSpeedIntensity
			);

			// Generate JITTER with fast time accumulation
			XMVECTOR jitterRotation = PathUtils::generateHandheldRotationNoise(
				_handheldTimeAccumulator * 2.0f,  // FAST time for jitter
				rotJitterIntensity,
				0.0f,  // No breathing for jitter
				5.0f   // High frequency for jitter
			);

			// Combine both rotations
			interpolatedRotation = XMQuaternionMultiply(interpolatedRotation, driftRotation);
			interpolatedRotation = XMQuaternionMultiply(interpolatedRotation, jitterRotation);
			interpolatedRotation = XMQuaternionNormalize(interpolatedRotation);
		}

		// Add subtle FOV breathing effect
		const float fovVariation = sin(_handheldTimeAccumulator * _handheldBreathingRate * 0.5f) *
			(scaledBreathingIntensity * 10.0f);

		// Apply very subtle FOV changes
		interpolatedFOV += fovVariation * 0.005f * interpolatedFOV;
	}

	XMVECTOR CameraPath::applySmoothPlayerOffset(XMVECTOR position, float deltaTime)
	{
		if (!_relativeToPlayerEnabled) {
			return position; // No change if relative mode is disabled
		}

		// Calculate target player offset (current player position - initial player position)
		XMVECTOR targetOffset = XMVectorSubtract(_playerPosition, _initialPlayerPosition);

		// Smoothly interpolate the current offset toward the target offset
		// Lower smoothing factor = slower, more gradual transitions
		// Higher smoothing factor = faster, more responsive transitions
		float t = min(1.0f, deltaTime * _offsetSmoothingFactor);

		// Use SmoothStep for more natural acceleration/deceleration
		float smoothT = t * t * (3.0f - 2.0f * t);

		// Interpolate current offset toward target
		_currentPlayerOffset = XMVectorLerp(_currentPlayerOffset, targetOffset, smoothT);

		// Apply the smoothed offset to the position
		return XMVectorAdd(position, _currentPlayerOffset);
	}

	void CameraPath::setProgressPosition(float progress)
	{
		if (_nodes.size() < 2)
			return;

		// Ensure progress is clamped
		progress = max(0.0f, min(1.0f, progress));
		_currentProgress = progress;

		// Update time/arc length based on progress
		switch (_parameterMode)
		{
		case ParameterMode::UniformTime:
			_currentTime = progress * _totalDuration;
			// Reset time accumulator for proper resume
			//MessageHandler::logDebug("Set time to: %.2f (%.1f%%)", _currentTime, progress * 100.0f);
			break;
		case ParameterMode::ArcLengthUniformSpeed:
			if (totalDist > 0.0f) _currentArcLength = progress * totalDist;
			// Reset arc length accumulator for proper resume
			//MessageHandler::logDebug("Set arc length to: %.2f (%.1f%%)", _currentArcLength, progress * 100.0f);
			break;
		}
	}

	float CameraPath::getProgress() const
	{
		if (_nodes.size() < 2)
			return 0.0f;

		float progress = 0.0f;

		switch (_parameterMode)
		{
		case ParameterMode::UniformTime:
			if (_totalDuration > 0.0)
			{
				progress = static_cast<float>(_currentTime / _totalDuration);
				//MessageHandler::logDebug("UniformTime progress: time=%.2f, duration=%.2f, progress=%.4f",
					//_currentTime, _totalDuration, progress);
			}
			break;
		case ParameterMode::ArcLengthUniformSpeed:
			if (totalDist > 0.0f)
			{
				progress = _currentArcLength / totalDist;
				//MessageHandler::logDebug("ArcLength progress: arcLength=%.2f, totalDist=%.2f, progress=%.4f",
					//_currentArcLength, totalDist, progress);
			}
			break;
		}

		// Ensure progress is clamped and valid
		progress = max(0.0f, min(1.0f, progress));

		// Prevent NaN or infinity
		if (!std::isfinite(progress))
		{
			MessageHandler::logError("Progress calculation resulted in invalid value! Path Manager State reset to Idle");
			progress = 0.0f;
		}

		return progress;
	}

	XMFLOAT3 CameraPath::calculatePathOffsetTargetPosition(const XMFLOAT3& playerPos,
		const XMVECTOR& playerRotation) const noexcept
	{
		// Convert player position to vector
		XMVECTOR playerPosVec = XMLoadFloat3(&playerPos);

		// Load the local offset from settings
		XMVECTOR localOffset = XMLoadFloat3(&_pathLookAtOffset);

		// Create rotation matrix from player's quaternion
		XMMATRIX playerRotMatrix = XMMatrixRotationQuaternion(playerRotation);

		// Transform the local offset to world space using player's rotation
		XMVECTOR worldOffset = XMVector3Transform(localOffset, playerRotMatrix);

		// Add the world space offset to player position
		XMVECTOR targetPosVec = XMVectorAdd(playerPosVec, worldOffset);

		// Convert back to XMFLOAT3
		XMFLOAT3 result;
		XMStoreFloat3(&result, targetPosVec);

		return result;
	}

	XMFLOAT3 CameraPath::calculatePathLookAtRotation(const XMFLOAT3& cameraPos,
		const XMFLOAT3& targetPos) const noexcept
	{
		// Calculate direction vector from camera to target
		XMVECTOR camPos = XMLoadFloat3(&cameraPos);
		XMVECTOR targPos = XMLoadFloat3(&targetPos);
		XMVECTOR direction = XMVectorSubtract(targPos, camPos);

		// Normalize the direction vector
		direction = XMVector3Normalize(direction);

		XMFLOAT3 dir;
		XMStoreFloat3(&dir, direction);

		// Calculate yaw (rotation around Y-axis)
		// atan2(-x, z) gives us the yaw angle where forward is +Z
		float yaw = atan2f(kRightSign * (-dir.x), kForwardSign * dir.z);  // Apply kForwardSign here

		// Calculate pitch (rotation around X-axis)
		// Try positive dir.y first (inverted from original)
		float pitch = asinf(kUpSign * dir.y);

		// Roll is typically 0 for look-at, but could be made configurable later
		float roll = 0.0f;

		return { pitch, yaw, roll };
	}

	void CameraPath::applyPathLookAt(float deltaTime) noexcept
	{
		if (GameSpecific::CameraManipulator::getCameraStructAddress() == nullptr)
		{
			return;
		}

		if (!_pathLookAtEnabled) {
			_hasValidPathLookAtTarget = false;
			return;
		}

		__try {

			// Get current interpolated camera position
			XMFLOAT3 cameraPos;
			XMStoreFloat3(&cameraPos, interpolatedPosition);

			// Get player position and rotation
			XMVECTOR playerPosVec = GameSpecific::CameraManipulator::getCurrentPlayerPosition();
			XMFLOAT3 playerPos;
			XMStoreFloat3(&playerPos, playerPosVec);
			XMVECTOR playerRotation = GameSpecific::CameraManipulator::getCurrentPlayerRotation();

			// Calculate the offset target position in player's local coordinate system
			XMFLOAT3 lookAtTarget = calculatePathOffsetTargetPosition(playerPos, playerRotation);
			_currentPathLookAtTarget = lookAtTarget;  // Store for visualization
			_hasValidPathLookAtTarget = true;  // Mark as valid

			// Calculate target look-at rotation
			XMFLOAT3 targetLookAtRotation = calculatePathLookAtRotation(cameraPos, lookAtTarget);

			// Create quaternion from look-at rotation, applying negation constants
			targetLookAtRotation = {
				NEGATE_PITCH ? -targetLookAtRotation.x : targetLookAtRotation.x,
				NEGATE_YAW ? -targetLookAtRotation.y : targetLookAtRotation.y,
				NEGATE_ROLL ? -targetLookAtRotation.z : targetLookAtRotation.z
			};

			XMVECTOR targetRotationQuat = Utils::generateEulerQuaternion(targetLookAtRotation, MULTIPLICATION_ORDER, NEGATE_PITCH,NEGATE_YAW,NEGATE_ROLL);
			targetRotationQuat = XMQuaternionNormalize(targetRotationQuat);

			// Check if we're in scrubbing mode - if so, apply immediate look-at without smoothing
			bool isScrubbing = PathManager::instance()._pathManagerStatus == Scrubbing;

			if (isScrubbing)
			{
				// During scrubbing, apply immediate look-at for responsive feedback
				interpolatedRotation = targetRotationQuat;
				// Update our smoothed state to match so resuming playback is smooth
				_currentPathLookAtRotation = targetRotationQuat;
				_pathLookAtInitialized = true;
				return;
			}

			// Initialize current rotation on first frame or after scrubbing
			if (!_pathLookAtInitialized)
			{
				_currentPathLookAtRotation = targetRotationQuat;
				_pathLookAtInitialized = true;
			}

			// Ensure quaternion continuity for smooth interpolation
			PathUtils::EnsureQuaternionContinuity(_currentPathLookAtRotation, targetRotationQuat);

			// Smooth interpolation between current and target rotation (normal playback)
			const float rt = std::clamp(_pathLookAtSmoothness * deltaTime, 0.0f, 1.0f);
			_currentPathLookAtRotation = XMVectorCatmullRom(_currentPathLookAtRotation, _currentPathLookAtRotation,
				targetRotationQuat, targetRotationQuat, rt);
			_currentPathLookAtRotation = XMQuaternionNormalize(_currentPathLookAtRotation);

			// Override the interpolated rotation with smoothed look-at rotation
			interpolatedRotation = _currentPathLookAtRotation;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			MessageHandler::logError("Exception in CameraPath::applyPathLookAt");
			_hasValidPathLookAtTarget = false;
			return;
		}
	}

	void CameraPath::updateVehicleSpeedTracking(float deltaTime) noexcept
	{
		if (!_pathSpeedMatchingEnabled)
		{
			// If speed matching is disabled, ensure we use normal speed
			_currentPathSpeedMultiplier = 1.0f;
			_targetPathSpeedMultiplier = 1.0f;
			return;
		}

		// Get current vehicle position
		XMVECTOR currentVehiclePos = GameSpecific::CameraManipulator::getCurrentPlayerPosition();

		// Initialize on first frame
		if (!_vehicleSpeedInitialized)
		{
			_previousVehiclePosition = currentVehiclePos;
			_vehicleSpeedInitialized = true;
			_currentVehicleSpeed = 0.0f;
			_currentPathSpeedMultiplier = 1.0f;
			_targetPathSpeedMultiplier = 1.0f;
			return;
		}

		// Calculate current vehicle speed (3D velocity magnitude)
		_currentVehicleSpeed = calculateVehicleSpeed(currentVehiclePos, _previousVehiclePosition, deltaTime);

		// Map vehicle speed to path speed multiplier
		_targetPathSpeedMultiplier = mapVehicleSpeedToPathMultiplier(_currentVehicleSpeed);

		// Smooth interpolation between current and target speed multiplier
		const float speedSmoothingRate = std::clamp(_pathSpeedSmoothness * deltaTime, 0.0f, 1.0f);
		_currentPathSpeedMultiplier = PathUtils::lerpFMA(_currentPathSpeedMultiplier, _targetPathSpeedMultiplier, speedSmoothingRate);

		// Update previous position for next frame
		_previousVehiclePosition = currentVehiclePos;
	}

	float CameraPath::calculateVehicleSpeed(const DirectX::XMVECTOR& currentPos,
		const DirectX::XMVECTOR& previousPos, float deltaTime) const noexcept
	{
		if (deltaTime <= 0.0f)
			return 0.0f;

		// Calculate 3D displacement vector
		XMVECTOR displacement = XMVectorSubtract(currentPos, previousPos);

		// Calculate distance moved (magnitude of displacement)
		float distance = XMVectorGetX(XMVector3Length(displacement));

		// Calculate speed as distance over time
		float speed = distance / deltaTime;

		return speed;
	}

	float CameraPath::mapVehicleSpeedToPathMultiplier(float vehicleSpeed) const noexcept
	{
		// Calculate base multiplier based on vehicle speed relative to baseline
		float baseMultiplier = (vehicleSpeed / _pathBaselineSpeed) * _pathSpeedScale;

		// Apply scaling factor from settings
		float scaledMultiplier = baseMultiplier;

		// Clamp to min/max bounds
		float clampedMultiplier = std::clamp(scaledMultiplier, _pathMinSpeed, _pathMaxSpeed);

		return clampedMultiplier;
	}

	void CameraPath::resetSpeedMatching() noexcept
	{
		_vehicleSpeedInitialized = false;
		_previousVehiclePosition = XMVectorZero();
		_currentVehicleSpeed = 0.0f;
		_targetPathSpeedMultiplier = 1.0f;
		_currentPathSpeedMultiplier = 1.0f;
	}

	void CameraPath::updatePathLookAtTarget() noexcept
	{
		if (!Globals::instance().settings().pathLookAtEnabled){
			_hasValidPathLookAtTarget = false;
			return;
		}

		// Get player position and rotation for current frame
		XMVECTOR playerPosVec = GameSpecific::CameraManipulator::getCurrentPlayerPosition();
		XMFLOAT3 playerPos;
		XMStoreFloat3(&playerPos, playerPosVec);
		XMVECTOR playerRotation = GameSpecific::CameraManipulator::getCurrentPlayerRotation();

		// set offset
		_pathLookAtOffset = XMFLOAT3(Globals::instance().settings().pathLookAtOffsetX, Globals::instance().settings().pathLookAtOffsetY, Globals::instance().settings().pathLookAtOffsetZ);

		// Calculate the offset target position in player's local coordinate system
		XMFLOAT3 lookAtTarget = calculatePathOffsetTargetPosition(playerPos, playerRotation);

		// Store for visualization
		_currentPathLookAtTarget = lookAtTarget;
		_hasValidPathLookAtTarget = true;
	}
}
