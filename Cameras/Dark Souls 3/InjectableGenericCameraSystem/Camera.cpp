////////////////////////////////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System (refactored 2025)
////////////////////////////////////////////////////////////////////////////////////////////////////////
// ================== Camera.cpp ==================
#include "stdafx.h"
#include "Camera.h"
#include "Defaults.h"
#include "GameConstants.h"
#include "Globals.h"
#include "CameraToolsData.h"
#include "CameraManipulator.h"
#include "PathUtils.h"

using namespace DirectX;

namespace IGCS
{
    // -----------------------------------------------------------------------------------------------------------------
    //  Compile?time engine?specific movement signs
    //  Modify these constants (?1.f or 1.f) to match your engine’s forward/right/up directions. 1.f == current behaviour.
    static constexpr float kForwardSign = 1.0f; // set to ?1.f if engine uses negative forward
    static constexpr float kRightSign = 1.0f; // set to ?1.f if engine uses negative right
    static constexpr float kUpSign = 1.0f; // set to ?1.f if engine uses negative up
    // -----------------------------------------------------------------------------------------------------------------

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

    // --------------------------------------------- Camera update ----------------------------------------------------
    void Camera::updateCamera(float delta) noexcept
    {
        const auto& s = Globals::instance().settings();
        const float pt = std::clamp(s.movementSmoothness * delta, 0.0f, 1.0f);
        const float rt = std::clamp(s.rotationSmoothness * delta, 0.0f, 1.0f);
        const float ft = std::clamp(s.fovSmoothness * delta, 0.0f, 1.0f);

        //  Rotation
        XMVECTOR cR = generateEulerQuaternion({ _pitch, _yaw, _roll }, MULTIPLICATION_ORDER, false, false, false);

    	//use defaults here, including any negation as we need our target values to have that incorporated
    	XMVECTOR tR = Utils::generateEulerQuaternion({ _targetpitch, _targetyaw, _targetroll }); 

        PathUtils::EnsureQuaternionContinuity(cR, tR);

        //XMVECTOR newRotQ = XMQuaternionSlerp(cR, tR, rt);
        XMVECTOR newRotQ = XMVectorCatmullRom(cR, cR, tR, tR, rt);
        newRotQ = XMQuaternionNormalize(newRotQ);
        setRotation(QuaternionToEulerAngles(newRotQ, MULTIPLICATION_ORDER));
		XMStoreFloat4(&_toolsQuaternion, newRotQ);

        //  Position (lerp)
        XMVECTOR newPos = XMVectorLerp(XMLoadFloat3(&_direction), XMLoadFloat3(&_targetdirection), pt);
        XMStoreFloat3(&_direction, newPos);

        //  FOV
        _fov = PathUtils::lerpFMA(_fov, _targetfov, ft);

        //  Push to game memory
        GameSpecific::CameraManipulator::updateCameraDataInGameData();
        GameSpecific::CameraManipulator::changeFoV(_fov);
    }

    // --------------------------------------------- Bridging ---------------------------------------------------------
    void Camera::setAllRotation(DirectX::XMFLOAT3 eulers) noexcept
    {
        _pitch = eulers.x;
    	_targetpitch = (NEGATE_PITCH ? -eulers.x : eulers.x);
    	_yaw = eulers.y;
    	_targetyaw = (NEGATE_YAW ? -eulers.y : eulers.y);
    	_roll = eulers.z;
    	_targetroll = (NEGATE_ROLL ? -eulers.z : eulers.z);
        initFOV();
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
} // namespace IGCS
