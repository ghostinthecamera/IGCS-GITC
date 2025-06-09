#include "stdafx.h"
#include "CameraPath.h"
#include "Camera.h"
#include "Utils.h"
#include "CameraManipulator.h"
#include "Globals.h"
#include "MessageHandler.h"
#include <algorithm>
#include <cmath>
#include "CatmullRom.h"
#include "CentripetalCatmullRom.h"
#include "PathUtils.h"
#include "RiemannCubic.h"
#include "Bezier.h"
#include "BSpline.h"
#include "Cubic.h"

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
		//_samplesize = Globals::instance().settings().sampleCountSetting;

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
		if (!Globals::instance().settings().d3ddisabled && D3DHook::instance().isVisualizationEnabled()) {
			D3DHook::instance().markResourcesForUpdate();
		}

		_nodes.emplace_back(position, rotation, fov);
		CameraPathManager::instance().appendStateToReshadePath(_pathName);
		generateArcLengthTables();
	}

	uint8_t CameraPath::addNode()
	{
		auto node = createNode();

		if (!Globals::instance().settings().d3ddisabled && D3DHook::instance().isVisualizationEnabled()) {
			D3DHook::instance().markResourcesForUpdate();
		}

		// Add the new node
		_nodes.emplace_back(node);

		//update reshade state if connected
		CameraPathManager::instance().appendStateToReshadePath(_pathName);

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

		if (!Globals::instance().settings().d3ddisabled && D3DHook::instance().isVisualizationEnabled()) {
			D3DHook::instance().markResourcesForUpdate();
		}
		_nodes.insert(_nodes.begin() + nodeIndex, node);

		MessageHandler::logDebug("CameraPath::insertNodeBefore: Node inserted before %d", nodeIndex);
		MessageHandler::logLine("Node inserted before %d in path: %s", nodeIndex, _pathName.c_str());

		CameraPathManager::instance().insertStateBeforeReshadeSnapshot(_pathName, nodeIndex);
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

		if (!Globals::instance().settings().d3ddisabled && D3DHook::instance().isVisualizationEnabled()) {
			D3DHook::instance().markResourcesForUpdate();
		}

		_nodes.erase(_nodes.begin() + nodeIndex);
		MessageHandler::logDebug("CameraPath::deleteNode: Node %zu deleted", nodeIndex);

		//delete from reshade if available
		CameraPathManager::instance().removeReshadeState(_pathName, nodeIndex);

		MessageHandler::logLine("Node %zu deleted from path: %s", nodeIndex, _pathName.c_str());
		continuityCheck();
		// Update arc length tables after successful deletion
		generateArcLengthTables();
	}

	void CameraPath::updateNode(uint8_t nodeIndex)
	{
		if (nodeIndex < _nodes.size())
		{
			XMFLOAT3 pos = GameSpecific::CameraManipulator::getCurrentCameraCoords();
			XMFLOAT4 rot = Camera::instance().getToolsQuaternion();
			float fov = GameSpecific::CameraManipulator::getCurrentFoV();
			_nodes[nodeIndex].position = XMLoadFloat3(&pos);
			_nodes[nodeIndex].rotation = XMLoadFloat4(&rot);
			_nodes[nodeIndex].fov = fov;

			//update reshade state if connected
			CameraPathManager::instance().updateReshadeState(_pathName, nodeIndex);
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
		CameraPathManager& currPM = CameraPathManager::instance();
		getSettings();

		if (_nodes.size() < 2)
		{
			MessageHandler::logDebug("Not enough nodes. Exiting.");
			currPM.handleStopPathMessage();
			return;
		}

		setArcLengthTables();

		if (totalDist < 1e-6f)
		{
			MessageHandler::logDebug("Total path length too small. Exiting.");
			currPM.handleStopPathMessage();
			return;
		}

		if (_totalDuration < 1e-6f)
		{
			MessageHandler::logDebug("Invalid total duration. Exiting.");
			currPM.handleStopPathMessage();
			return;
		}

		// ----------------------------------------------------------------------
		// Update _currentArcLength by uniform speed in space
		// ----------------------------------------------------------------------

		//const float globalT = getGlobalT(deltaTime); // the final parameter we feed into the interpolation
		const float globalT = getGlobalT(deltaTime, currPM._pathManagerState);

		// ----------------------------------------------------------------------
		// Interpolate position, rotation, FOV
		// ----------------------------------------------------------------------

		performPositionInterpolation(globalT);
		performRotationInterpolation(globalT);
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

		if (CameraPathManager::instance()._pathManagerState == Idle)
		{
			XMFLOAT3 position;
			const XMFLOAT3 eulers = QuaternionToEulerAngles(interpolatedRotation, MULTIPLICATION_ORDER);
			XMStoreFloat3(&position, interpolatedPosition);
			Camera::instance().setAllRotation(eulers);
			GameSpecific::CameraManipulator::setCurrentCameraCoords(position);
			Camera::instance().setFoV(interpolatedFOV, true);
			Camera::instance().resetMovement();
			Camera::instance().resetTargetMovement();
		}
	}

	void CameraPath::updateReshadeStateController(const float& globalT) const
	{
		if (!CameraPathManager::instance().reshadeAddonisConnected()) 
			return;

		const int seg = getsegmentIndex(globalT);
		const int nextSeg = min(seg + 1, static_cast<int>(_nodes.size() - 1));
		float localt = globalT - static_cast<float>(seg);
		localt = max(0.0f, min(1.0f, localt));
		CameraPathManager::instance().applyInterpolatedReshadeState(_pathName, seg, nextSeg, localt);
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
		_handheldBreathingRate = s.handheldBreathingRate;  // Breathing cycles per second
		_handheldPositionEnabled = s.handheldPositionToggle;
		_handheldRotationEnabled = s.handheldRotationToggle;
		_handheldDriftSpeed = s.handheldDriftSpeed;  // Drift speed in Hz
		_handheldRotationDriftSpeed = s.handheldRotationDriftSpeed;  // Drift speed in Hz

		// Update player-related settings
		setPlayerPosition(GameSpecific::CameraManipulator::getCurrentPlayerPosition());
		Utils::onChange(_relativeToPlayerEnabled, newRelativeToPlayerEnabled, [this](auto t) {setRelativeToPlayerEnabled(t); });
		//if (_relativeToPlayerEnabled != newRelativeToPlayerEnabled)
		//	setRelativeToPlayerEnabled(newRelativeToPlayerEnabled);
		_offsetSmoothingFactor = s.relativePathSmoothness;
	}

	float CameraPath::getGlobalT(const float& deltaTime, const PathManagerStatus currentState)
	{
		const float easingValue = Globals::instance().settings().easingValueSetting;

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
				_currentTime += deltaTime;  // track elapsed time
				progress = static_cast<float>(_currentTime) / static_cast<float>(_totalDuration);

				if (progress >= 1.0f)
				{
					progress = 1.0f;  // clamp
					CameraPathManager::instance().handleStopPathMessage();
					XMFLOAT3 eulers = QuaternionToEulerAngles(getNodeRotation(_nodes.size() - 1), MULTIPLICATION_ORDER);
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
				_currentArcLength += speed * deltaTime;

				if (_currentArcLength >= totalDist)
				{
					_currentArcLength = totalDist;
					CameraPathManager::instance().handleStopPathMessage();
					XMFLOAT3 eulers = QuaternionToEulerAngles(getNodeRotation(_nodes.size() - 1), MULTIPLICATION_ORDER);
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

		// Generate smooth noise-based offsets with subtle jitter blending
		float noiseX = PathUtils::smoothNoise(_shakeTime, _shakeFrequency) + (PathUtils::randomJitter() * 0.2f);
		float noiseY = PathUtils::smoothNoise(_shakeTime + 12.3f, _shakeFrequency * 1.2f) + (PathUtils::randomJitter() * 0.2f);
		float noiseZ = PathUtils::smoothNoise(_shakeTime + 25.7f, _shakeFrequency * 0.8f) + (PathUtils::randomJitter() * 0.2f);

		// Apply shake offset with balanced randomness
		XMVECTOR shakeOffset = XMVectorSet(
			noiseX * easedShakeAmplitude,
			noiseY * easedShakeAmplitude,
			noiseZ * easedShakeAmplitude,
			0.0f
		);

		// Apply slightly different noise to rotation for more organic movement
		XMVECTOR shakeRotation = Utils::generateEulerQuaternion(XMFLOAT3(
			(PathUtils::smoothNoise(_shakeTime, _shakeFrequency) + (PathUtils::randomJitter() * 0.1f)) * easedShakeAmplitude * 0.4f,
			(PathUtils::smoothNoise(_shakeTime + 5.0f, _shakeFrequency * 1.5f) + (PathUtils::randomJitter() * 0.1f)) * easedShakeAmplitude * 0.4f,
			(PathUtils::smoothNoise(_shakeTime + 15.0f, _shakeFrequency * 0.7f) + (PathUtils::randomJitter() * 0.1f)) * easedShakeAmplitude * 0.4f)
		);

		//	// Apply to final interpolated position
		interpolatedPosition = XMVectorAdd(interpolatedPosition, shakeOffset);
		// Multiply quaternions to apply shake rotation
		interpolatedRotation = XMQuaternionMultiply(interpolatedRotation, shakeRotation);
	}

	void CameraPath::applyHandheldCameraEffect(const float deltaTime)
	{
		// Determine progress based on parameter mode
		float progress = (_parameterMode == ParameterMode::UniformTime) ?
			static_cast<float>(_currentTime / _totalDuration) : (_currentArcLength / totalDist);
		progress = max(0.0f, min(progress, 1.0f));

		// Apply easing to progress based on the current easing type
		float easedProgress = PathUtils::applyEasing(progress, _easingType, Globals::instance().settings().easingValueSetting);

		// Create an intensity curve that's stronger in the middle of the path
		// and gentler at the beginning and end for a more cinematic feel
		//if (_easingType == EasingType::None) {
		//	// For no easing, use a bell curve for intensity (more natural)
		//	intensityCurve = 1.0f - (2.0f * fabsf(progress - 0.5f));
		//	intensityCurve = 0.4f + (intensityCurve * 0.6f); // Range from 0.4 to 1.0
		//}
		//else {
		//	// For other easing types, use the eased progress directly
			float intensityCurve = easedProgress;
		//}

		// Dynamic intensity modulation based on camera motion
		// Calculate camera path movement speed to adjust handheld intensity
		static XMVECTOR prevPosition = interpolatedPosition;
		float movementMagnitude = XMVectorGetX(XMVector3Length(XMVectorSubtract(interpolatedPosition, prevPosition))) / deltaTime;
		prevPosition = interpolatedPosition;

		// Adjust intensity based on camera movement - more shake during faster movements
		float movementMultiplier = 1.0f + min(movementMagnitude * 0.05f, 1.0f);

		// Base intensity affects overall effect
		float effectiveIntensity = _handheldIntensity * intensityCurve * movementMultiplier;

		// Separate component intensities properly
		float scaledDriftIntensity = _handheldDriftIntensity * effectiveIntensity * 0.03f;
		float scaledJitterIntensity = _handheldJitterIntensity * effectiveIntensity * 0.01f;
		float scaledBreathingIntensity = _handheldBreathingIntensity * effectiveIntensity * 0.02f;

		// Handle position effects - apply position offset with proper drift vs jitter balance
		if (_handheldPositionEnabled) {
			// Use physics-based simulation for position with position drift speed parameter
			XMVECTOR positionOffset = PathUtils::simulateHandheldCamera(
				deltaTime,
				scaledDriftIntensity,     // Drift (low frequency) component 
				scaledJitterIntensity,    // Jitter (high frequency) component
				scaledBreathingIntensity, // Breathing component
				_handheldPositionVelocity,
				_handheldDriftSpeed       // Position drift speed parameter
			);

			// Apply position offset
			interpolatedPosition = XMVectorAdd(interpolatedPosition, positionOffset);
		}

		// Handle rotation effects with dedicated rotation drift speed
		if (_handheldRotationEnabled) {
			// Separate rotational intensities to allow more independent control
			float rotDriftIntensity = _handheldDriftIntensity * effectiveIntensity;
			float rotJitterIntensity = _handheldJitterIntensity * effectiveIntensity * 0.3f;

			// Create blended intensity that weights drift higher than jitter for rotation
			float blendedRotIntensity = (rotDriftIntensity * 0.8f) + (rotJitterIntensity * 0.2f); // Further favor drift vs jitter

			// Generate rotation noise with rotation-specific drift speed parameter
			XMVECTOR handRotation = PathUtils::generateHandheldRotationNoise(
				_handheldTimeAccumulator,
				blendedRotIntensity,           // Blended intensity favoring drift over jitter
				_handheldBreathingRate,
				_handheldRotationDriftSpeed    // Separate rotation drift speed parameter
			);

			// Apply rotation - multiply quaternions
			interpolatedRotation = XMQuaternionMultiply(interpolatedRotation, handRotation);

			// Ensure the quaternion is normalized
			interpolatedRotation = XMQuaternionNormalize(interpolatedRotation);
		}

		// Add subtle FOV breathing effect - simulates eye focus changes
		float fovVariation = sin(_handheldTimeAccumulator * _handheldBreathingRate * 0.5f) *
			(scaledBreathingIntensity * 0.1f);

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
			MessageHandler::logError("Progress calculation resulted in invalid value!");
			progress = 0.0f;
		}

		return progress;
	}
}
