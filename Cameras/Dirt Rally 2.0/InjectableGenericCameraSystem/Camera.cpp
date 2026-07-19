////////////////////////////////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System (refactored 2025) - Modded with fpv features
////////////////////////////////////////////////////////////////////////////////////////////////////////
// ================== Camera.cpp ==================
#include "stdafx.h"
#include "Camera.h"
#include "GameConstants.h"
#include "Globals.h"
#include "CameraManipulator.h"
#include "PathUtils.h"
#pragma comment(lib, "Xinput.lib")

using namespace DirectX;

namespace IGCS
{
    static DirectX::XMFLOAT3 s_velocity = { 0.0f, 0.0f, 0.0f };
    static const float s_thrustPower = 60.0f;   // Strength of motors
    static const float s_gravity = -31.81f;      // Downward pull
    static const float s_drag = 0.985f;         // Air resistance (1.0 = no friction)
    static const float s_cameraTilt = -40.0f;
    
    // RAW CONTROLLER INTERCEPTS
    static float s_rawLeftStickY = 0.0f;  // Throttle
    static float s_rawLeftStickX = 0.0f;  // Yaw
    static float s_rawRightStickY = 0.0f; // Pitch
    static float s_rawRightStickX = 0.0f;


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
    //                            Movement (camera?target)   ---------------------------------------------------------
    void Camera::moveForward(float amount, bool target) noexcept
    {
        s_rawLeftStickY = amount;
        if (target)
            _targetdirection.z += kForwardSign * Globals::instance().settings().movementSpeed * amount;
        else
            _direction.z += kForwardSign * Globals::instance().settings().movementSpeed * amount;

        _movementOccurred = true;
    }

    void Camera::moveRight(float amount, bool target) noexcept
    {
        s_rawLeftStickX = amount;
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
        s_rawRightStickX = amount;
        _targetyaw = clampAngle(_targetyaw + Globals::instance().settings().rotationSpeed * amount);
    }

    void Camera::targetPitch(float amount) noexcept
    {
        s_rawRightStickY = amount;
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
            XMFLOAT3 dirFloat3;
            XMStoreFloat3(&dirFloat3, dir);
            const float explicitYMovement = dirFloat3.y;
            dirFloat3.y = 0.0f;

            const XMVECTOR horizontalDir = XMLoadFloat3(&dirFloat3);
            const XMVECTOR rotatedHorizontalDir = XMVector3Rotate(horizontalDir, lookQ);
            const XMVECTOR newPos = XMVectorAdd(curr, rotatedHorizontalDir);

            XMFLOAT3 result;
            XMStoreFloat3(&result, newPos);
            result.y = currentCoords.y + explicitYMovement;

            XMStoreFloat3(&_toolsCoordinates, XMLoadFloat3(&result));
            return result;
        }
        else
        {
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
            captureCurrentRelativeOffset();
        else
        {
            _targetpitch = NEGATE_PITCH ? -_pitch : _pitch;
            _targetyaw = NEGATE_YAW ? -_yaw : _yaw;
            _targetroll = NEGATE_ROLL ? -_roll : _roll;
        }
        _fixedCameraMountEnabled = !_fixedCameraMountEnabled;
    }

    void Camera::captureCurrentRelativeOffset() noexcept
    {
        const XMFLOAT3 cameraRotation = getRotation();
        const XMVECTOR playerPosVec = GameSpecific::CameraManipulator::getCurrentPlayerPosition();
        const XMVECTOR playerRotVec = GameSpecific::CameraManipulator::getCurrentPlayerRotation();

        const XMVECTOR camPosVec = XMLoadFloat3(&_toolsCoordinates);
        const XMVECTOR worldOffset = XMVectorSubtract(camPosVec, playerPosVec);

        const XMMATRIX playerRotMatrix = XMMatrixRotationQuaternion(playerRotVec);
        const XMMATRIX invPlayerRotMatrix = XMMatrixTranspose(playerRotMatrix);
        const XMVECTOR localOffset = XMVector3Transform(worldOffset, invPlayerRotMatrix);

        XMStoreFloat3(&_fixedMountPositionOffset, localOffset);

        const XMVECTOR cameraRotVec = generateEulerQuaternion(cameraRotation, MULTIPLICATION_ORDER, false, false, false);
        const XMVECTOR invPlayerRot = XMQuaternionInverse(playerRotVec);
        _fixedMountRelativeRotation = XMQuaternionMultiply(cameraRotVec, invPlayerRot);
    };

    // --------------------------------------------- Camera update ----------------------------------------------------
    void Camera::updateCamera(const float delta) noexcept
    {
        static const auto& s = Globals::instance().settings();
        const float pt = std::clamp(s.movementSmoothness * delta, 0.0f, 1.0f);
        const float rt = std::clamp(s.rotationSmoothness * delta, 0.0f, 1.0f);
        const float ft = std::clamp(s.fovSmoothness * delta, 0.0f, 1.0f);

        updateCameraEffectSettings(s);

        if (System::instance().isIGCSSessionActive())
            return;

        if (_fixedCameraMountEnabled)
        {
            handleFixedMountMode();
        }
        else if (s.lookAtEnabled)
        {
            handleLookAtMode(pt, rt);
        }
        else if (s.fpvEnabled) {
            // FPV ACRO DRONE PHYSICS
            // ======================
            _hasValidLookAtTarget = false;

            // 1. MODE 2 CONTROLS & INVERSION FIXES
            // Throttle: Left Stick Y (Up). If it only works pulling back, use: max(0.0f, -s_rawLeftStickY)
            float inputThrottle = (std::max)(0.0f, s_rawLeftStickY);

            float acroRate = 0.68f; // Base rotation sensitivity
            float yawRate = 1.5f;

            // Yaw: Left Stick X (Left/Right)
            _yaw += -s_rawLeftStickX * yawRate * delta;
            _pitch += s_rawRightStickY * acroRate * delta;
            _roll += s_rawRightStickX * acroRate * delta;

            // Sync IGCS targets
            _targetpitch = _pitch;
            _targetyaw = _yaw;
            _targetroll = _roll;

            // 2. Drone Body Physics Rotation
            XMMATRIX droneRotation = XMMatrixRotationRollPitchYaw(_pitch, _yaw, _roll);

            // 3. Thrust vector calculation
            XMVECTOR localUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            XMVECTOR globalThrust = XMVector3TransformNormal(localUp, droneRotation);
            XMVECTOR thrust = XMVectorScale(globalThrust, inputThrottle * s_thrustPower);

            // 4. Velocity & Gravity Integration
            XMVECTOR gravity = XMVectorSet(0.0f, s_gravity, 0.0f, 0.0f);
            XMVECTOR currentVel = XMLoadFloat3(&s_velocity);

            XMVECTOR acceleration = XMVectorAdd(thrust, gravity);
            currentVel = XMVectorAdd(currentVel, XMVectorScale(acceleration, delta));
            currentVel = XMVectorScale(currentVel, s_drag); // Apply air resistance
            XMStoreFloat3(&s_velocity, currentVel);

            // 5. Position Integration
            XMVECTOR currentPos = XMLoadFloat3(&_toolsCoordinates);
            currentPos = XMVectorAdd(currentPos, XMVectorScale(currentVel, delta));
            XMStoreFloat3(&_toolsCoordinates, currentPos);

            // 6. Visual Camera Tilt
            float tiltRads = XMConvertToRadians(s_cameraTilt);
            XMMATRIX tiltMatrix = XMMatrixRotationX(tiltRads);
            XMMATRIX finalCamRot = XMMatrixMultiply(tiltMatrix, droneRotation);

            // Write final visual rotation to IGCS
            XMStoreFloat4(&_toolsQuaternion, XMQuaternionRotationMatrix(finalCamRot));

            // 7. CLEAR INPUTS FOR NEXT FRAME
            // If you let go of the thumbstick, IGCS stops calling moveForward/targetYaw.
            // We must manually zero these out so the drone doesn't ghost-fly.
            s_rawLeftStickY = 0.0f;
            s_rawLeftStickX = 0.0f;
            s_rawRightStickY = 0.0f;
            s_rawRightStickX = 0.0f;
        }
        else
        {
            handleNormalMode(pt, rt);
        }

        interpolateFOV(ft);
        applyFinalCameraTransform(delta);
    }

    void Camera::handleFixedMountMode() noexcept
    {
        _hasValidLookAtTarget = false;
        const XMVECTOR playerPosVec = GameSpecific::CameraManipulator::getCurrentPlayerPosition();
        const XMVECTOR playerRotVec = GameSpecific::CameraManipulator::getCurrentPlayerRotation();
        applyFixedMountTransformation(playerPosVec, playerRotVec);
        _direction = { 0.0f, 0.0f, 0.0f };
        _targetdirection = { 0.0f, 0.0f, 0.0f };
    }

    void Camera::handleLookAtMode(float positionLerpTime, float rotationLerpTime) noexcept
    {
        const XMFLOAT3 cameraPos = _toolsCoordinates;
        const XMVECTOR playerPosVec = GameSpecific::CameraManipulator::getCurrentPlayerPosition();
        XMFLOAT3 playerPos;
        XMStoreFloat3(&playerPos, playerPosVec);
        const XMVECTOR playerRotation = GameSpecific::CameraManipulator::getCurrentPlayerRotation();
        applyLookAtRotation(cameraPos, playerPos, playerRotation);
        interpolatePosition(positionLerpTime);
        interpolateRotation(rotationLerpTime);
    }

    void Camera::handleNormalMode(float positionLerpTime, float rotationLerpTime) noexcept
    {
        // NO LONGER USED - BYPASSED IN updateCamera() FOR FPV PHYSICS
    }

    void Camera::interpolatePosition(float lerpTime) noexcept
    {
        const XMVECTOR newPos = XMVectorLerp(XMLoadFloat3(&_direction), XMLoadFloat3(&_targetdirection), lerpTime);
        XMStoreFloat3(&_direction, newPos);
    }

    void Camera::interpolateRotation(float lerpTime) noexcept
    {
        const XMVECTOR cR = generateEulerQuaternion({ _pitch, _yaw, _roll }, MULTIPLICATION_ORDER, false, false, false);
        XMVECTOR tR = Utils::generateEulerQuaternion({ _targetpitch, _targetyaw, _targetroll });
        PathUtils::EnsureQuaternionContinuity(cR, tR);
        XMVECTOR newRotQ = XMQuaternionSlerp(cR, tR, lerpTime);
        newRotQ = XMQuaternionNormalize(newRotQ);
        setRotation(QuaternionToEulerAngles(newRotQ, MULTIPLICATION_ORDER));
        XMStoreFloat4(&_toolsQuaternion, newRotQ);
    }

    inline void Camera::interpolateFOV(const float lerpTime) noexcept
    {
        _fov = PathUtils::lerpFMA(_fov, _targetfov, lerpTime);
    }

    void Camera::applyFixedMountTransformation(const XMVECTOR& playerPosVec, const XMVECTOR& playerRotVec) noexcept
    {
        const XMVECTOR localOffset = XMLoadFloat3(&_fixedMountPositionOffset);
        const XMVECTOR worldOffset = XMVector3Rotate(localOffset, playerRotVec);
        const XMVECTOR newCameraPos = XMVectorAdd(playerPosVec, worldOffset);
        XMStoreFloat3(&_toolsCoordinates, newCameraPos);

        const XMVECTOR newCameraRotQuat = XMQuaternionMultiply(_fixedMountRelativeRotation, playerRotVec);
        XMStoreFloat4(&_toolsQuaternion, newCameraRotQuat);

        const XMFLOAT3 newEulers = Utils::QuaternionToEulerAngles(newCameraRotQuat, MULTIPLICATION_ORDER);
        _pitch = _targetpitch = newEulers.x;
        _yaw = _targetyaw = newEulers.y;
        _roll = _targetroll = newEulers.z;
    }

    void Camera::applyLookAtRotation(const XMFLOAT3& cameraPos, const XMFLOAT3& playerPos, const XMVECTOR& playerRotation) noexcept
    {
        XMFLOAT3 lookAtTarget;
        if (_useTargetOffsetMode)
        {
            lookAtTarget = calculateOffsetTargetPosition(playerPos, playerRotation);
            _currentLookAtTargetPosition = lookAtTarget;
            _hasValidLookAtTarget = true;
        }
        else
        {
            lookAtTarget = playerPos;
            _hasValidLookAtTarget = false;
        }

        const XMFLOAT3 lookAtRotation = calculateLookAtRotation(cameraPos, lookAtTarget);
        setTargetPitch(lookAtRotation.x);
        setTargetYaw(lookAtRotation.y);
        setTargetRoll(lookAtRotation.z);
    }

    void Camera::applyFinalCameraTransform(float deltaTime) noexcept
    {
        GameSpecific::CameraManipulator::updateCameraDataInGameData();
        GameSpecific::CameraManipulator::changeFoV(_fov);

        XMFLOAT3 finalPosition = _toolsCoordinates;
        XMVECTOR finalRotation = XMLoadFloat4(&_toolsQuaternion);
        float finalFov = _fov;

        applyShakeEffect(finalPosition, finalRotation, deltaTime);
        applyHandheldCameraEffect(finalPosition, finalRotation, finalFov, deltaTime);

        XMStoreFloat4(&_toolsQuaternion, finalRotation);
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

        // Reset our FPV velocity if we reset direction!
        // This prevents the drone from carrying momentum if you toggle the camera off and on.
        s_velocity = { 0.0f, 0.0f, 0.0f };
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

    void Camera::prepareCamera() noexcept
    {
        _toolsCoordinates = GameSpecific::CameraManipulator::getCurrentCameraCoords();
        setFoV(GameSpecific::CameraManipulator::getCurrentFoV(), true);
        setAllRotation(GameSpecific::CameraManipulator::getEulers());
        resetDirection();
        initFOV();
    }

    void Camera::updateCameraEffectSettings(const Settings& s) noexcept
    {
        _shakeEnabled = s.isCameraShakeEnabledB;
        _shakeAmplitude = s.shakeAmplitudeB;
        _shakeFrequency = s.shakeFrequencyB;
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

    void Camera::applyShakeEffect(DirectX::XMFLOAT3& position, DirectX::XMVECTOR& rotation, float deltaTime) noexcept
    {
        if (!_shakeEnabled || _shakeAmplitude <= 0.0f) return;
        _shakeTime += deltaTime;
        const float positionAmplitudeScale = 0.0005f;
        const float positionFrequencyScale = 10.0f;
        const float scaledAmplitude = _shakeAmplitude * positionAmplitudeScale;
        const float scaledFrequency = _shakeFrequency * positionFrequencyScale;

        const float noiseX = PathUtils::smoothNoise(_shakeTime, scaledFrequency) + (PathUtils::randomJitter() * 0.2f);
        const float noiseY = PathUtils::smoothNoise(_shakeTime + 12.3f, scaledFrequency * 1.2f) + (PathUtils::randomJitter() * 0.2f);
        const float noiseZ = PathUtils::smoothNoise(_shakeTime + 25.7f, scaledFrequency * 0.8f) + (PathUtils::randomJitter() * 0.2f);

        const XMVECTOR shakeOffset = XMVectorSet(noiseX * scaledAmplitude, noiseY * scaledAmplitude, noiseZ * scaledAmplitude, 0.0f);
        XMVECTOR currentPos = XMLoadFloat3(&position);
        currentPos = XMVectorAdd(currentPos, shakeOffset);
        XMStoreFloat3(&position, currentPos);

        const float rotationAmplitudeScale = 0.1f;
        const float rotationFrequencyScale = 10.0f;
        const float rotationAmplitude = _shakeAmplitude * rotationAmplitudeScale;
        const float rotationFrequency = _shakeFrequency * rotationFrequencyScale;

        const XMVECTOR shakeRotation = Utils::generateEulerQuaternion(XMFLOAT3(
            (PathUtils::smoothNoise(_shakeTime, rotationFrequency) + (PathUtils::randomJitter() * 0.1f)) * rotationAmplitude * 0.55f,
            (PathUtils::smoothNoise(_shakeTime + 5.0f, rotationFrequency * 1.5f) + (PathUtils::randomJitter() * 0.1f)) * rotationAmplitude * 0.4f,
            (PathUtils::smoothNoise(_shakeTime + 15.0f, rotationFrequency * 0.7f) + (PathUtils::randomJitter() * 0.1f)) * rotationAmplitude * 0.05f)
        );

        rotation = XMQuaternionMultiply(rotation, shakeRotation);
        rotation = XMQuaternionNormalize(rotation);
    }

    void Camera::applyHandheldCameraEffect(DirectX::XMFLOAT3& position, DirectX::XMVECTOR& rotation, float& fov, float deltaTime) noexcept
    {
        if (!_handheldEnabled) return;
        _handheldTimeAccumulator += deltaTime;

        static XMVECTOR prevPosition = XMLoadFloat3(&position);
        XMVECTOR currentPosVec = XMLoadFloat3(&position);
        const float movementMagnitude = XMVectorGetX(XMVector3Length(XMVectorSubtract(currentPosVec, prevPosition))) / deltaTime;
        prevPosition = currentPosVec;

        const float movementMultiplier = 1.0f + min(movementMagnitude * 0.05f, 1.0f);
        const float posBaseIntensityScalar = 7.0f;
        const float posEffectiveIntensity = _handheldIntensity * movementMultiplier * posBaseIntensityScalar;

        const float positionDriftIntensityScale = 0.005f;
        const float positionJitterIntensityScale = 0.0003f;
        const float positionBreathingIntensityScale = 0.01f;
        const float positionDriftSpeedScale = 10.0f;

        const float scaledDriftIntensity = _handheldDriftIntensity * posEffectiveIntensity * positionDriftIntensityScale;
        const float scaledJitterIntensity = _handheldJitterIntensity * posEffectiveIntensity * positionJitterIntensityScale;
        const float scaledBreathingIntensity = _handheldBreathingIntensity * positionBreathingIntensityScale;
        const float scaledDriftSpeed = _handheldDriftSpeed * posEffectiveIntensity * positionDriftSpeedScale;

        if (_handheldPositionEnabled) {
            const XMVECTOR driftOffset = PathUtils::simulateHandheldCamera(
                deltaTime, scaledDriftIntensity, 0.0f, scaledBreathingIntensity, _handheldPositionVelocity, scaledDriftSpeed
            );

            static XMVECTOR jitterVelocity = XMVectorZero();
            const XMVECTOR jitterOffset = PathUtils::simulateHandheldCamera(
                deltaTime * 5.0f, 0.0f, scaledJitterIntensity, 0.0f, jitterVelocity, 20.0f
            );

            const XMVECTOR totalOffset = XMVectorAdd(driftOffset, jitterOffset);
            currentPosVec = XMVectorAdd(currentPosVec, totalOffset);
            XMStoreFloat3(&position, currentPosVec);
        }

        const float rotBaseIntensityScalar = 25.0f;
        const float rotEffectiveIntensity = _handheldIntensity * (movementMultiplier * 0.2f) * rotBaseIntensityScalar;

        if (_handheldRotationEnabled) {
            const float rotationDriftIntensityScale = 1.0f;
            const float rotationJitterIntensityScale = 0.02f;
            const float rotationBreathingRateScale = 25.0f;
            const float rotationDriftSpeedScale = 1.0f;

            const float rotDriftIntensity = _handheldDriftIntensity * rotEffectiveIntensity * rotationDriftIntensityScale;
            const float rotJitterIntensity = _handheldJitterIntensity * rotEffectiveIntensity * rotationJitterIntensityScale;
            const float breathingRateIntensity = _handheldBreathingRate * rotationBreathingRateScale;
            const float rotDriftSpeedIntensity = _handheldRotationDriftSpeed * rotationDriftSpeedScale;

            const XMVECTOR driftRotation = PathUtils::generateHandheldRotationNoise(
                _handheldTimeAccumulator * 0.5f, rotDriftIntensity, breathingRateIntensity, rotDriftSpeedIntensity
            );

            const XMVECTOR jitterRotation = PathUtils::generateHandheldRotationNoise(
                _handheldTimeAccumulator * 2.0f, rotJitterIntensity, 0.0f, 5.0f
            );

            rotation = XMQuaternionMultiply(rotation, driftRotation);
            rotation = XMQuaternionMultiply(rotation, jitterRotation);
            rotation = XMQuaternionNormalize(rotation);
        }

        const float fovVariation = sin(_handheldTimeAccumulator * _handheldBreathingRate * 0.5f) * (scaledBreathingIntensity * 10.0f);
        fov += fovVariation * 0.005f * fov;
    }
} // namespace IGCS