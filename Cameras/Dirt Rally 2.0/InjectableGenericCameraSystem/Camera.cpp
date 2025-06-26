////////////////////////////////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System (refactored 2025)
////////////////////////////////////////////////////////////////////////////////////////////////////////
// ================== Camera.cpp ==================
#include "stdafx.h"
#include "Camera.h"
#include "GameConstants.h"
#include "Globals.h"
#include "CameraManipulator.h"
#include "PathUtils.h"

using namespace DirectX;

namespace IGCS
{
    // --------------------------------------------- Quaternion / movement helpers -------------------------------------
    XMVECTOR Camera::calculateLookQuaternion() noexcept
    {
        const XMVECTOR q = Utils::generateEulerQuaternion(getRotation());
        XMStoreFloat4(&_toolsQuaternion, q);
		_toolsMatrix = XMMatrixRotationQuaternion(q);
        return q;
    }

    // -----------------------------------------------------------------------------------------------------------------
    XMFLOAT3 Camera::calculateNewCoords(const XMFLOAT3& currentCoords, FXMVECTOR lookQ) noexcept
    {
        const XMVECTOR curr = XMLoadFloat3(&currentCoords);
        const XMVECTOR dir = XMLoadFloat3(&_direction);
        const XMVECTOR newDir = XMVectorAdd(curr, XMVector3Rotate(dir, lookQ));

        XMFLOAT3 result;
        XMStoreFloat3(&result, newDir);
        XMStoreFloat3(&_toolsCoordinates, newDir);
        return result;
    }

    // ---------------------------------------------- Movement state helpers -------------------------------------------
    void Camera::resetMovement() noexcept
    {
        _movementOccurred = false;
        _direction = { 0,0,0 };
    }

    void Camera::resetTargetMovement() noexcept
    {
        _movementOccurred = false;
        _targetdirection = { 0,0,0 };
    }

    void Camera::resetAngles() noexcept
    {
        setPitch(INITIAL_PITCH_RADIANS);
        setRoll(INITIAL_ROLL_RADIANS);
        setYaw(INITIAL_YAW_RADIANS);
        setTargetPitch(INITIAL_PITCH_RADIANS);
        setTargetRoll(INITIAL_ROLL_RADIANS);
        setTargetYaw(INITIAL_YAW_RADIANS);
    }

    // -----------------------------------------------------------------------------------------------------------------
    //                              Movement (camera?target)   ---------------------------------------------------------
    void Camera::moveForward(float amount, bool target) noexcept
    {
	    if (target)
			_targetdirection.z += kForwardSign * Globals::instance().settings().movementSpeed * amount;
		else
			_direction.z += kForwardSign * Globals::instance().settings().movementSpeed * amount;

        _movementOccurred = true;
    }

    void Camera::moveRight(float amount, bool target) noexcept
    {
	    if (target)
			_targetdirection.x += kRightSign * Globals::instance().settings().movementSpeed * amount;
		else
			_direction.x += kRightSign * Globals::instance().settings().movementSpeed * amount;

        _movementOccurred = true;
    }

    void Camera::moveUp(float amount, bool target) noexcept
    {
	    if (target)
			_targetdirection.y += kUpSign * Globals::instance().settings().movementSpeed * amount * Globals::instance().settings().movementUpMultiplier;
		else
			_direction.y += kUpSign * Globals::instance().settings().movementSpeed * amount * Globals::instance().settings().movementUpMultiplier;

        _movementOccurred = true;
    }

    // ----------------------------------------------- Rotation helpers -----------------------------------------------
    void Camera::targetYaw(float amount) noexcept
    {
        _targetyaw = clampAngle(_targetyaw + Globals::instance().settings().rotationSpeed * amount);
    }

    void Camera::targetPitch(float amount) noexcept
    {
        const float inverter = Globals::instance().settings().invertY ? -_lookDirectionInverter : _lookDirectionInverter;
        _targetpitch = clampAngle(_targetpitch + Globals::instance().settings().rotationSpeed * amount * inverter);
    }

    void Camera::targetRoll(float amount) noexcept
    {
        _targetroll = clampAngle(_targetroll + Globals::instance().settings().rotationSpeed * amount);
    }

    // --------------------------------------------- Direct angle setters ---------------------------------------------
    void Camera::setPitch(float angle) noexcept { _pitch = clampAngle(angle); }
    void Camera::setYaw(float angle) noexcept { _yaw = clampAngle(angle); }
    void Camera::setRoll(float angle) noexcept { _roll = clampAngle(angle); }

    float Camera::clampAngle(float angle) noexcept
    {
        while (angle > XM_PI) { angle -= XM_2PI; }
        while (angle < -XM_PI) { angle += XM_2PI; }
        return angle;
    }

    // --------------------------------------------- FOV helpers ------------------------------------------------------
    void Camera::initFOV() noexcept
    {
        if (_fov <= 0.01f)
        {
            _fov = IGCS::GameSpecific::CameraManipulator::getCurrentFoV();
            _targetfov = _fov;
        }
    }

    void Camera::changeFOV(float amount) noexcept
    {
        _targetfov = Utils::rangeClamp(_targetfov + amount, 0.01f, 3.0f);
    }

    void Camera::setFoV(float fov, bool fullreset) noexcept
    {
        if (fullreset)
            _fov = _targetfov = fov;
        else
            _targetfov = fov;
    }

    // --------------------------------------------- Angle helpers ----------------------------------------------------
    float Camera::shortestAngleDifference(float current, float target) noexcept
    {
        float diff = target - current;
        if (diff > XM_PI)      diff -= XM_2PI;
        else if (diff < -XM_PI) diff += XM_2PI;
        return diff;
    }

    // ----------------------------------------- PLayer look at
    DirectX::XMFLOAT3 Camera::calculateLookAtRotation(const DirectX::XMFLOAT3& cameraPos,
        const DirectX::XMFLOAT3& targetPos) const noexcept
    {
        // Calculate direction vector from camera to target
        const XMVECTOR camPos = XMLoadFloat3(&cameraPos);
        const XMVECTOR targPos = XMLoadFloat3(&targetPos);
        XMVECTOR direction = XMVectorSubtract(targPos, camPos);

        // Normalize the direction vector
        direction = XMVector3Normalize(direction);

        XMFLOAT3 dir;
        XMStoreFloat3(&dir, direction);

        // Calculate yaw (rotation around Y-axis)
        // atan2(-x, z) gives us the yaw angle where forward is +Z
        float yaw = atan2f(-dir.x, dir.z);

        // Calculate pitch (rotation around X-axis)
        // Try positive dir.y first (inverted from original)
        float pitch = asinf(dir.y);

        // Apply the same inversion logic that the camera uses for manual input
        const float inverter = Globals::instance().settings().invertY ? -_lookDirectionInverter : _lookDirectionInverter;
        pitch *= inverter;

        // Apply angle offsets only if we're in angle offset mode
        if (!_useTargetOffsetMode)
        {
            pitch = clampAngle(pitch + _lookAtPitchOffset);
            yaw = clampAngle(yaw + _lookAtYawOffset);
        }

        // Roll handling - always maintain current roll and apply roll offset
        float roll = clampAngle(_lookAtRollOffset);

        return { pitch, yaw, roll };
    }

    DirectX::XMFLOAT3 Camera::calculateOffsetTargetPosition(const DirectX::XMFLOAT3& playerPos,
        const DirectX::XMVECTOR& playerRotation) const noexcept
    {
        // Convert player position to vector
        const XMVECTOR playerPosVec = XMLoadFloat3(&playerPos);

        // Load the local offset
        const XMVECTOR localOffset = XMLoadFloat3(&_lookAtTargetOffset);

        // Create rotation matrix from player's quaternion
        const XMMATRIX playerRotMatrix = XMMatrixRotationQuaternion(playerRotation);

        // Transform the local offset to world space using player's rotation
        const XMVECTOR worldOffset = XMVector3Transform(localOffset, playerRotMatrix);

        // Add the world space offset to player position
        const XMVECTOR targetPosVec = XMVectorAdd(playerPosVec, worldOffset);

        // Convert back to XMFLOAT3
        XMFLOAT3 result;
        XMStoreFloat3(&result, targetPosVec);

        return result;
    }

    DirectX::XMFLOAT3 Camera::calculateNewCoordsHeightLocked(const DirectX::XMFLOAT3& currentCoords,
        FXMVECTOR lookQ, bool preserveHeight) noexcept
    {
        const XMVECTOR curr = XMLoadFloat3(&currentCoords);
        const XMVECTOR dir = XMLoadFloat3(&_direction);

        if (preserveHeight && _heightLockedMovement)
        {
            // Separate explicit vertical movement from horizontal movement
            XMFLOAT3 dirFloat3;
            XMStoreFloat3(&dirFloat3, dir);

            // Save the explicit Y movement (from moveUp calls)
            const float explicitYMovement = dirFloat3.y;

            // Zero out Y component for horizontal movement rotation
            dirFloat3.y = 0.0f;

            // Apply rotation only to horizontal movement (X and Z)
            const XMVECTOR horizontalDir = XMLoadFloat3(&dirFloat3);
            const XMVECTOR rotatedHorizontalDir = XMVector3Rotate(horizontalDir, lookQ);

            // Apply horizontal movement to current position
            const XMVECTOR newPos = XMVectorAdd(curr, rotatedHorizontalDir);

            // Store result but preserve original Y and add explicit Y movement
            XMFLOAT3 result;
            XMStoreFloat3(&result, newPos);
            result.y = currentCoords.y + explicitYMovement;  // Preserve height + allow explicit vertical movement

            XMStoreFloat3(&_toolsCoordinates, XMLoadFloat3(&result));
            return result;
        }
        else
        {
            // Normal movement - apply all components
            const XMVECTOR newDir = XMVector3Rotate(dir, lookQ);
            const XMVECTOR newPos = XMVectorAdd(curr, newDir);

            XMFLOAT3 result;
            XMStoreFloat3(&result, newPos);
            XMStoreFloat3(&_toolsCoordinates, newPos);
            return result;
        }
    }

    void Camera::toggleFixedCameraMount() noexcept
    {
        if (!_fixedCameraMountEnabled)
        {
            // Enabling fixed mount - capture current relative offset
            captureCurrentRelativeOffset();
        }
        else
        {
            // Disabling fixed mount - adjust target rotations to account for negation constants
            _targetpitch = NEGATE_PITCH ? -_pitch : _pitch;
            _targetyaw = NEGATE_YAW ? -_yaw : _yaw;
            _targetroll = NEGATE_ROLL ? -_roll : _roll;
        }
        _fixedCameraMountEnabled = !_fixedCameraMountEnabled;
    }

    void Camera::captureCurrentRelativeOffset() noexcept
    {
        // Get current camera position and rotation
        const XMFLOAT3 cameraPos = _toolsCoordinates;
        const XMFLOAT3 cameraRotation = getRotation();

        // Get player position and rotation
        const XMVECTOR playerPosVec = GameSpecific::CameraManipulator::getCurrentPlayerPosition();
        XMFLOAT3 playerPos;
        XMStoreFloat3(&playerPos, playerPosVec);

        const XMVECTOR playerRotVec = GameSpecific::CameraManipulator::getCurrentPlayerRotation();

        // Calculate relative position in player's local coordinate system
        const XMVECTOR camPosVec = XMLoadFloat3(&cameraPos);
        const XMVECTOR playerPosVec3 = XMLoadFloat3(&playerPos);
        const XMVECTOR worldOffset = XMVectorSubtract(camPosVec, playerPosVec3);

        // Transform world offset to player's local space (inverse transform)
        const XMMATRIX playerRotMatrix = XMMatrixRotationQuaternion(playerRotVec);
        const XMMATRIX invPlayerRotMatrix = XMMatrixTranspose(playerRotMatrix);  // Inverse of rotation matrix is its transpose
        const XMVECTOR localOffset = XMVector3Transform(worldOffset, invPlayerRotMatrix);

        XMStoreFloat3(&_fixedMountPositionOffset, localOffset);

        // Calculate relative transformation matrix
        // Create transformation matrices for both player and camera
        const XMMATRIX playerTransform = XMMatrixMultiply(playerRotMatrix, XMMatrixTranslationFromVector(playerPosVec));

        const XMVECTOR cameraRotVec = generateEulerQuaternion(cameraRotation, MULTIPLICATION_ORDER, false, false, false);
        const XMMATRIX cameraRotMatrix = XMMatrixRotationQuaternion(cameraRotVec);
        const XMMATRIX cameraTransform = XMMatrixMultiply(cameraRotMatrix, XMMatrixTranslationFromVector(camPosVec));

        // Calculate relative transform: camera = relativeTransform * player
        // So: relativeTransform = camera * inverse(player)
        const XMMATRIX invPlayerTransform = XMMatrixInverse(nullptr, playerTransform);
        _fixedMountRelativeTransform = XMMatrixMultiply(cameraTransform, invPlayerTransform);
    };

    // --------------------------------------------- Camera update ----------------------------------------------------
    void Camera::updateCamera(const float delta) noexcept
    {
        static const auto& s = Globals::instance().settings();
        const float pt = std::clamp(s.movementSmoothness * delta, 0.0f, 1.0f);
        const float rt = std::clamp(s.rotationSmoothness * delta, 0.0f, 1.0f);
        const float ft = std::clamp(s.fovSmoothness * delta, 0.0f, 1.0f);

        // Update camera effect settings
        updateCameraEffectSettings(s);

        // Handle IGCS session - early return if active
        if (System::instance().isIGCSSessionActive())
            return;

        // Handle camera modes - each mode is self-contained and handles both position and rotation
        if (_fixedCameraMountEnabled)
        {
            handleFixedMountMode();
        }
        else if (s.lookAtEnabled)
        {
            handleLookAtMode(pt, rt);
        }
        else
        {
            handleNormalMode(pt, rt);
        }

        // Apply FOV interpolation
        interpolateFOV(ft);

        // Apply final camera transformations and effects
        applyFinalCameraTransform(delta);
    }

    void Camera::handleFixedMountMode() noexcept
    {
        _hasValidLookAtTarget = false;

        // Get player transform data
        const XMVECTOR playerPosVec = GameSpecific::CameraManipulator::getCurrentPlayerPosition();
        const XMVECTOR playerRotVec = GameSpecific::CameraManipulator::getCurrentPlayerRotation();

        // Apply fixed mount transformation
        applyFixedMountTransformation(playerPosVec, playerRotVec);

        // Clear movement since position is controlled by mount
        _direction = { 0.0f, 0.0f, 0.0f };
        _targetdirection = { 0.0f, 0.0f, 0.0f };

        // Update quaternion directly from current euler angles (no interpolation)
        const XMVECTOR currentRot = generateEulerQuaternion({ _pitch, _yaw, _roll },
            MULTIPLICATION_ORDER, false, false, false);
        XMStoreFloat4(&_toolsQuaternion, currentRot);
    }

    void Camera::handleLookAtMode(float positionLerpTime, float rotationLerpTime) noexcept
    {
        // Get current positions
        const XMFLOAT3 cameraPos = _toolsCoordinates;
        const XMVECTOR playerPosVec = GameSpecific::CameraManipulator::getCurrentPlayerPosition();
        XMFLOAT3 playerPos;
        XMStoreFloat3(&playerPos, playerPosVec);

        // Get player rotation for target offset mode
        const XMVECTOR playerRotation = GameSpecific::CameraManipulator::getCurrentPlayerRotation();

        // Apply look-at rotation
        applyLookAtRotation(cameraPos, playerPos, playerRotation);

        // Apply position interpolation
        interpolatePosition(positionLerpTime);

        // Apply rotation interpolation
        interpolateRotation(rotationLerpTime);
    }

    void Camera::handleNormalMode(float positionLerpTime, float rotationLerpTime) noexcept
    {
        _hasValidLookAtTarget = false;

        // Apply standard position interpolation
        interpolatePosition(positionLerpTime);

        // Apply rotation interpolation
        interpolateRotation(rotationLerpTime);
    }

    void Camera::interpolatePosition(float lerpTime) noexcept
    {
        const XMVECTOR newPos = XMVectorLerp(XMLoadFloat3(&_direction),
            XMLoadFloat3(&_targetdirection),
            lerpTime);
        XMStoreFloat3(&_direction, newPos);
    }

    void Camera::interpolateRotation(float lerpTime) noexcept
    {
        const XMVECTOR cR = generateEulerQuaternion({ _pitch, _yaw, _roll },
            MULTIPLICATION_ORDER, false, false, false);
        XMVECTOR tR = Utils::generateEulerQuaternion({ _targetpitch, _targetyaw, _targetroll });

        PathUtils::EnsureQuaternionContinuity(cR, tR);

        XMVECTOR newRotQ = XMVectorCatmullRom(cR, cR, tR, tR, lerpTime);
        newRotQ = XMQuaternionNormalize(newRotQ);
        setRotation(QuaternionToEulerAngles(newRotQ, MULTIPLICATION_ORDER));
        XMStoreFloat4(&_toolsQuaternion, newRotQ);
    }

    void Camera::interpolateFOV(float lerpTime) noexcept
    {
        _fov = PathUtils::lerpFMA(_fov, _targetfov, lerpTime);
    }

    void Camera::applyFixedMountTransformation(const XMVECTOR& playerPosVec,
        const XMVECTOR& playerRotVec) noexcept
    {
        // Create player transformation matrix
        const XMMATRIX playerRotMatrix = XMMatrixRotationQuaternion(playerRotVec);
        const XMMATRIX playerTransform = XMMatrixMultiply(playerRotMatrix,
            XMMatrixTranslationFromVector(playerPosVec));

        // Calculate new camera transform
        const XMMATRIX newCameraTransform = XMMatrixMultiply(_fixedMountRelativeTransform, playerTransform);

        // Extract and apply position
        const XMVECTOR newCameraPos = XMVectorSet(
            XMVectorGetX(newCameraTransform.r[3]),
            XMVectorGetY(newCameraTransform.r[3]),
            XMVectorGetZ(newCameraTransform.r[3]),
            1.0f
        );

        // Apply position to our internal value
        XMStoreFloat3(&_toolsCoordinates, newCameraPos);

        // Extract and apply rotation
        const XMVECTOR newCameraRotQuat = XMQuaternionRotationMatrix(newCameraTransform);
        const XMFLOAT3 newCameraRotEuler = Utils::QuaternionToEulerAngles(newCameraRotQuat, MULTIPLICATION_ORDER);

        // Set rotations directly (no interpolation for mount mode)
        setTargetPitch(newCameraRotEuler.x);
        setTargetYaw(newCameraRotEuler.y);
        setTargetRoll(newCameraRotEuler.z);

        setPitch(newCameraRotEuler.x);
        setYaw(newCameraRotEuler.y);
        setRoll(newCameraRotEuler.z);
    }

    void Camera::applyLookAtRotation(const XMFLOAT3& cameraPos,
        const XMFLOAT3& playerPos,
        const XMVECTOR& playerRotation) noexcept
    {
        XMFLOAT3 lookAtTarget;

        if (_useTargetOffsetMode)
        {
            // Calculate offset target position in player's local space
            lookAtTarget = calculateOffsetTargetPosition(playerPos, playerRotation);
            _currentLookAtTargetPosition = lookAtTarget;
            _hasValidLookAtTarget = true;
        }
        else
        {
            // Look directly at player (angle offsets applied in calculateLookAtRotation)
            lookAtTarget = playerPos;
            _hasValidLookAtTarget = false;
        }

        // Calculate and apply look-at rotation
        const XMFLOAT3 lookAtRotation = calculateLookAtRotation(cameraPos, lookAtTarget);

        setTargetPitch(lookAtRotation.x);
        setTargetYaw(lookAtRotation.y);
        setTargetRoll(lookAtRotation.z);
    }

    void Camera::applyFinalCameraTransform(float deltaTime) noexcept
    {
        // Update camera data in game memory
        GameSpecific::CameraManipulator::updateCameraDataInGameData();
        GameSpecific::CameraManipulator::changeFoV(_fov);

        // Get final position and rotation
        XMFLOAT3 finalPosition = _toolsCoordinates;
        XMVECTOR finalRotation = XMLoadFloat4(&_toolsQuaternion);
        float finalFov = _fov;

        // Apply camera effects in order
        applyShakeEffect(finalPosition, finalRotation, deltaTime);
        applyHandheldCameraEffect(finalPosition, finalRotation, finalFov, deltaTime);

        // Update tools quaternion with final rotation
        XMStoreFloat4(&_toolsQuaternion, finalRotation);

        // Write final values to game memory
        GameSpecific::CameraManipulator::writeNewCameraValuesToGameData(finalPosition, finalRotation);
        GameSpecific::CameraManipulator::changeFoV(finalFov);
    }

    // --------------------------------------------- Bridging ---------------------------------------------------------
    void Camera::setAllRotation(DirectX::XMFLOAT3 eulers) noexcept
    {
        _targetpitch = (NEGATE_PITCH ? -eulers.x : eulers.x);
        _targetyaw = (NEGATE_YAW ? -eulers.y : eulers.y);
        _targetroll = (NEGATE_ROLL ? -eulers.z : eulers.z);

    	_pitch = eulers.x;
    	_yaw = eulers.y;
    	_roll = eulers.z;

        initFOV();
    }

	void Camera::resetDirection() noexcept
	{
		_direction = { 0.0f, 0.0f, 0.0f };
		_targetdirection = { 0.0f, 0.0f, 0.0f };
		_movementOccurred = false;
	}

    void Camera::setTargetEulers(DirectX::XMFLOAT3 eulers) noexcept
    {
        _targetpitch = eulers.x;
        _targetyaw = eulers.y;
        _targetroll = eulers.z;
    }

    void Camera::setEulers(DirectX::XMFLOAT3 eulers) noexcept
    {
        _pitch = eulers.x;
        _yaw = eulers.y;
        _roll = eulers.z;
    }

    void Camera::setTargetPitch(float a) noexcept { _targetpitch = clampAngle(a); }
    void Camera::setTargetYaw(float a) noexcept { _targetyaw = clampAngle(a); }
    void Camera::setTargetRoll(float a) noexcept { _targetroll = clampAngle(a); }


    //--------------------------- Camera Prep
	void Camera::prepareCamera() noexcept
	{
        // Initialize internal position from game memory
        _toolsCoordinates = GameSpecific::CameraManipulator::getCurrentCameraCoords();
    	// Set camera fov to game fov
        setFoV(GameSpecific::CameraManipulator::getCurrentFoV(), true);
		// Set initial values
		setAllRotation(GameSpecific::CameraManipulator::getEulers());
		_direction = { 0.0f, 0.0f, 0.0f };
		_targetdirection = { 0.0f, 0.0f, 0.0f };
		_movementOccurred = false;
		// Initialize FOV
		initFOV();
	}

    void Camera::updateCameraEffectSettings(const Settings& s) noexcept
    {
        // Shake settings
        _shakeEnabled = s.isCameraShakeEnabledB;
        _shakeAmplitude = s.shakeAmplitudeB;
        _shakeFrequency = s.shakeFrequencyB;

        // Handheld settings
        _handheldEnabled = s.isHandheldEnabledB;
        _handheldIntensity = s.handheldIntensityB;
        _handheldDriftIntensity = s.handheldDriftIntensityB;
        _handheldJitterIntensity = s.handheldJitterIntensityB;
        _handheldBreathingIntensity = s.handheldBreathingIntensityB;
        _handheldBreathingRate = s.handheldBreathingRateB;
        _handheldDriftSpeed = s.handheldDriftSpeedB;
        _handheldRotationDriftSpeed = s.handheldRotationDriftSpeedB;
        _handheldPositionEnabled = s.handheldPositionToggleB;
        _handheldRotationEnabled = s.handheldRotationToggleB;
    }

    // Camera.cpp
    void Camera::applyShakeEffect(DirectX::XMFLOAT3& position, DirectX::XMVECTOR& rotation, float deltaTime) noexcept
    {
        if (!_shakeEnabled || _shakeAmplitude <= 0.0f)
            return;

        // Update shake time
        _shakeTime += deltaTime;

        // POSITION SCALE FACTOR - same as you discovered
        const float positionAmplitudeScale = 0.0005f;
        const float positionFrequencyScale = 10.0f;

        // Scale the amplitude and frequency for position
        const float scaledAmplitude = _shakeAmplitude * positionAmplitudeScale;
        const float scaledFrequency = _shakeFrequency * positionFrequencyScale;

        // Generate smooth noise-based offsets with subtle jitter blending
        const float noiseX = PathUtils::smoothNoise(_shakeTime, scaledFrequency) + (PathUtils::randomJitter() * 0.2f);
        const float noiseY = PathUtils::smoothNoise(_shakeTime + 12.3f, scaledFrequency * 1.2f) + (PathUtils::randomJitter() * 0.2f);
        const float noiseZ = PathUtils::smoothNoise(_shakeTime + 25.7f, scaledFrequency * 0.8f) + (PathUtils::randomJitter() * 0.2f);

        // Apply shake offset with balanced randomness
        const XMVECTOR shakeOffset = XMVectorSet(
            noiseX * scaledAmplitude,
            noiseY * scaledAmplitude,
            noiseZ * scaledAmplitude,
            0.0f
        );

        // Apply to position
        XMVECTOR currentPos = XMLoadFloat3(&position);
        currentPos = XMVectorAdd(currentPos, shakeOffset);
        XMStoreFloat3(&position, currentPos);

        // ROTATION SCALE FACTOR
        const float rotationAmplitudeScale = 0.1f;
        const float rotationFrequencyScale = 10.0f;

        const float rotationAmplitude = _shakeAmplitude * rotationAmplitudeScale;
        const float rotationFrequency = _shakeFrequency * rotationFrequencyScale;

        // Apply slightly different noise to rotation for more organic movement
        const XMVECTOR shakeRotation = Utils::generateEulerQuaternion(XMFLOAT3(
            (PathUtils::smoothNoise(_shakeTime, rotationFrequency) + (PathUtils::randomJitter() * 0.1f)) * rotationAmplitude * 0.55f,
            (PathUtils::smoothNoise(_shakeTime + 5.0f, rotationFrequency * 1.5f) + (PathUtils::randomJitter() * 0.1f)) * rotationAmplitude * 0.4f,
            (PathUtils::smoothNoise(_shakeTime + 15.0f, rotationFrequency * 0.7f) + (PathUtils::randomJitter() * 0.1f)) * rotationAmplitude * 0.05f)
        );

        // Multiply quaternions to apply shake rotation
        rotation = XMQuaternionMultiply(rotation, shakeRotation);
        rotation = XMQuaternionNormalize(rotation);
    }

    void Camera::applyHandheldCameraEffect(DirectX::XMFLOAT3& position, DirectX::XMVECTOR& rotation, float& fov, float deltaTime) noexcept
    {
        if (!_handheldEnabled)
            return;

        // Update time accumulator
        _handheldTimeAccumulator += deltaTime;

        // Dynamic intensity modulation based on camera motion
        static XMVECTOR prevPosition = XMLoadFloat3(&position);
        XMVECTOR currentPosVec = XMLoadFloat3(&position);
        const float movementMagnitude = XMVectorGetX(XMVector3Length(XMVectorSubtract(currentPosVec, prevPosition))) / deltaTime;
        prevPosition = currentPosVec;

        // Adjust intensity based on camera movement
        const float movementMultiplier = 1.0f + min(movementMagnitude * 0.05f, 1.0f);

        // Base intensity affects overall effect
        const float posBaseIntensityScalar = 7.0f;
        const float posEffectiveIntensity = _handheldIntensity * movementMultiplier * posBaseIntensityScalar;


        // POSITION SCALE FACTOR - adjust based on game's coordinate system
        const float positionDriftIntensityScale = 0.005f;
        const float positionJitterIntensityScale = 0.0003f;
        const float positionBreathingIntensityScale = 0.01f;
        const float positionDriftSpeedScale = 10.0f;


        // Separate component intensities with proper scaling
        const float scaledDriftIntensity = _handheldDriftIntensity * posEffectiveIntensity * positionDriftIntensityScale;// *5.0f;
        const float scaledJitterIntensity = _handheldJitterIntensity * posEffectiveIntensity * positionJitterIntensityScale;// *8.0f; // Jitter should be more noticeable
        const float scaledBreathingIntensity = _handheldBreathingIntensity * positionBreathingIntensityScale;// *2.0f;
        const float scaledDriftSpeed = _handheldDriftSpeed * posEffectiveIntensity * positionDriftSpeedScale;

        // Handle position effects
        if (_handheldPositionEnabled) {
            // DRIFT: Use simulateHandheldCamera with only drift enabled
            const XMVECTOR driftOffset = PathUtils::simulateHandheldCamera(
                deltaTime,              // Slow down time for drift
                scaledDriftIntensity,          // Drift intensity
                0.0f,                         // No jitter in drift pass
                scaledBreathingIntensity,     // Keep breathing
                _handheldPositionVelocity,    // Use main velocity state
                scaledDriftSpeed
            );

            // JITTER: Use simulateHandheldCamera with only jitter enabled
            static XMVECTOR jitterVelocity = XMVectorZero(); // Separate velocity state for jitter
            const XMVECTOR jitterOffset = PathUtils::simulateHandheldCamera(
                deltaTime * 5.0f,             // Speed up time for jitter
                0.0f,                         // No drift in jitter pass
                scaledJitterIntensity,          // Jitter intensity
                0.0f,                         // No breathing in jitter
                jitterVelocity,               // Separate velocity state
                20.0f                         // High frequency
            );

            // Combine both offsets
            const XMVECTOR totalOffset = XMVectorAdd(driftOffset, jitterOffset);

            // Apply position offset
            currentPosVec = XMVectorAdd(currentPosVec, totalOffset);
            XMStoreFloat3(&position, currentPosVec);
        }


        const float rotBaseIntensityScalar = 25.0f;
        const float rotEffectiveIntensity = _handheldIntensity * (movementMultiplier * 0.2f) * rotBaseIntensityScalar;

        // Handle rotation effects
        if (_handheldRotationEnabled) {
            // ROTATION SCALE FACTOR
            const float rotationDriftIntensityScale = 1.0f;
			const float rotationJitterIntensityScale = 0.02f;
            const float rotationBreathingRateScale = 25.0f;
            const float rotationDriftSpeedScale = 1.0f;

            // Separate rotational intensities
            const float rotDriftIntensity = _handheldDriftIntensity * rotEffectiveIntensity * rotationDriftIntensityScale;
            const float rotJitterIntensity = _handheldJitterIntensity * rotEffectiveIntensity * rotationJitterIntensityScale;
            const float breathingRateIntensity = _handheldBreathingRate * rotationBreathingRateScale;
            const float rotDriftSpeedIntensity = _handheldRotationDriftSpeed * rotationDriftSpeedScale;

            // Generate DRIFT with slow time accumulation
            const XMVECTOR driftRotation = PathUtils::generateHandheldRotationNoise(
                _handheldTimeAccumulator * 0.5f,  // SLOW time for drift
                rotDriftIntensity,
                breathingRateIntensity,
                rotDriftSpeedIntensity
            );

            // Generate JITTER with normal/fast time accumulation  
            const XMVECTOR jitterRotation = PathUtils::generateHandheldRotationNoise(
                _handheldTimeAccumulator * 2.0f,  // FAST time for jitter
                rotJitterIntensity,
                0.0f,  // No breathing for jitter
                5.0f   // High frequency for jitter
            );

            // Combine both rotations
            rotation = XMQuaternionMultiply(rotation, driftRotation);
            rotation = XMQuaternionMultiply(rotation, jitterRotation);
            rotation = XMQuaternionNormalize(rotation);
        }

        // Add subtle FOV breathing effect
        const float fovVariation = sin(_handheldTimeAccumulator * _handheldBreathingRate * 0.5f) *
            (scaledBreathingIntensity * 10.0f); 

        // Apply very subtle FOV changes
        fov += fovVariation * 0.005f * fov;
    }
} // namespace IGCS
