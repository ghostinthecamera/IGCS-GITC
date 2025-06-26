// ================== Camera.h ==================
#pragma once

#include <DirectXMath.h>
#include <algorithm>
#include "CameraToolsData.h"
#include "GameCameraData.h"
#include "Utils.h"

namespace IGCS
{
	struct Settings; //forward declaration because

	class Camera
    {
    public:
        static Camera& instance() noexcept
        {
            static Camera instance;
            return instance;
        }

        // ---- Public interface ------------------------------------------------------------------
        XMVECTOR calculateLookQuaternion() noexcept;
        [[nodiscard]] XMFLOAT3 calculateNewCoords(const XMFLOAT3& currentCoords,
            FXMVECTOR lookQ) noexcept;

        void resetMovement() noexcept;
        void resetAngles()  noexcept;
        void resetTargetMovement() noexcept;

        //  Movement helpers (values are multiplied by engine?sign constants; see cpp file)
        void moveForward(float amount, bool target = true)  noexcept;   // camera?target movement
        void moveRight(float amount, bool target = true)  noexcept;
        void moveUp(float amount, bool target = true)  noexcept;

        //  Rotation helpers
        void targetYaw(float amount) noexcept;
        void targetPitch(float amount) noexcept;
        void targetRoll(float amount) noexcept;

        //  Direct setters (clamped) ---------------------------------------------------------------
        void setPitch(float angle) noexcept;
        void setYaw(float angle) noexcept;
        void setRoll(float angle) noexcept;

        //  Getters --------------------------------------------------------------------------------
        [[nodiscard]] float getPitch() const noexcept { return _pitch; }
        [[nodiscard]] float getYaw() const noexcept { return _yaw; }
        [[nodiscard]] float getRoll() const noexcept { return _roll; }
		[[nodiscard]] float getTargetFov() const noexcept { return _targetfov; }
		[[nodiscard]] float getTargetPitch() const noexcept { return _targetpitch; }
		[[nodiscard]] float getTargetYaw() const noexcept { return _targetyaw; }
		[[nodiscard]] float getTargetRoll() const noexcept { return _targetroll; }
		[[nodiscard]] float getFov() const noexcept { return _fov; }

        [[nodiscard]] float lookDirectionInverter() const noexcept { return _lookDirectionInverter; }
        void toggleLookDirectionInverter() noexcept { _lookDirectionInverter = -_lookDirectionInverter; }

        //  Game?owned vectors/quaternions ---------------------------------------------------------
        [[nodiscard]] XMFLOAT3 getUpVector()      const noexcept { return _upVector; }
        [[nodiscard]] XMFLOAT3 getRightVector()   const noexcept { return _rightVector; }
        [[nodiscard]] XMFLOAT3 getForwardVector() const noexcept { return _forwardVector; }
        [[nodiscard]] XMFLOAT3 getGameEulers()    const noexcept { return _gameEulers; }
        [[nodiscard]] XMFLOAT4 getGameQuaternion()const noexcept { return _gameQuaternion; }
        [[nodiscard]] XMFLOAT4 getToolsQuaternion() const noexcept { return _toolsQuaternion; }
		[[nodiscard]] XMVECTOR getToolsQuaternionVector() const noexcept { return XMLoadFloat4(&_toolsQuaternion); }
		[[nodiscard]] XMFLOAT3 getToolsCoordinates() const noexcept { return _toolsCoordinates; }
		[[nodiscard]] XMMATRIX getToolsMatrix() const noexcept { return _toolsMatrix; }

        void setUpVector(XMFLOAT3 v) noexcept { _upVector = v; }
        void setRightVector(XMFLOAT3 v) noexcept { _rightVector = v; }
        void setForwardVector(XMFLOAT3 v) noexcept { _forwardVector = v; }
        void setGameEulers(XMFLOAT3 v) noexcept { _gameEulers = v; }
        void setGameQuaternion(XMFLOAT4 v) noexcept { _gameQuaternion = v; }
        void setToolsQuaternion(XMFLOAT4 q) noexcept { _toolsQuaternion = q; }

        //  Euler bridging ------------------------------------------------------------------------
        void setAllRotation(XMFLOAT3 eulers) noexcept;
        void resetDirection() noexcept;
        void setTargetEulers(XMFLOAT3 eulers) noexcept;
        void setEulers(XMFLOAT3 eulers) noexcept;

        //  Interpolation -------------------------------------------------------------------------
        [[nodiscard]] XMFLOAT3 getDirection() const noexcept { return _direction; }
        [[nodiscard]] XMFLOAT3 getTargetDirection() const noexcept { return _targetdirection; }
        void setDirection(XMFLOAT3 dir) noexcept { _direction = dir; }
        void setTargetDirection(XMFLOAT3 dir) noexcept { _targetdirection = dir; }

        [[nodiscard]] XMFLOAT3 getRotation() const noexcept { return { _pitch, _yaw, _roll }; }
        [[nodiscard]] XMFLOAT3 getTargetRotation() const noexcept { return { _targetpitch,_targetyaw,_targetroll }; }
        void setRotation(XMFLOAT3 rot) noexcept { setPitch(rot.x); setYaw(rot.y); setRoll(rot.z); }
		void setInterpolatedQuaternation(XMVECTOR q) { _interpolatedQuaternion = q; }
		[[nodiscard]] XMVECTOR getInterpolatedQuaternion() const { return _interpolatedQuaternion; }

        void setTargetPitch(float angle) noexcept;  // target setters (clamped)
        void setTargetYaw(float angle) noexcept;
        void setTargetRoll(float angle) noexcept;
        void prepareCamera() noexcept;

        float shortestAngleDifference(float current, float target) noexcept;

        //  Main update ---------------------------------------------------------------------------
        void updateCamera(float delta) noexcept;

        //  Field of view -------------------------------------------------------------------------
        void changeFOV(float amount) noexcept;
        void setFoV(float fov, bool fullreset) noexcept;
        void initFOV() noexcept;

        [[nodiscard]] auto getEulerOrder() const noexcept { return _eulerOrder; }

        // Look-at functionality
        void setLookAtPlayer(bool enabled) noexcept { _lookAtPlayerEnabled = enabled; }
        [[nodiscard]] bool isLookAtPlayerEnabled() const noexcept { return _lookAtPlayerEnabled; }
        void toggleLookAtPlayer() noexcept { _lookAtPlayerEnabled = !_lookAtPlayerEnabled; }

        // Toggle between offset modes
        void setUseTargetOffsetMode(bool useTargetOffset) noexcept { _useTargetOffsetMode = useTargetOffset; }
        [[nodiscard]] bool getUseTargetOffsetMode() const noexcept { return _useTargetOffsetMode; }
        void toggleOffsetMode() noexcept { _useTargetOffsetMode = !_useTargetOffsetMode; }

        // Height-locked movement (maintains camera Y position during horizontal movement)
        void setHeightLockedMovement(bool heightLocked) noexcept { _heightLockedMovement = heightLocked; }
        [[nodiscard]] bool getHeightLockedMovement() const noexcept { return _heightLockedMovement; }
        void toggleHeightLockedMovement() noexcept { _heightLockedMovement = !_heightLockedMovement; }

        // Fixed camera mount (camera locked to relative position/rotation from player like physically mounted)
        void setFixedCameraMount(bool enabled) noexcept { _fixedCameraMountEnabled = enabled; }
        [[nodiscard]] bool getFixedCameraMount() const noexcept { return _fixedCameraMountEnabled; }
        void toggleFixedCameraMount() noexcept;
        void captureCurrentRelativeOffset() noexcept;  // Captures current camera offset relative to player

        // Look-at angle offset functionality (original system)
        void addLookAtPitchOffset(float amount) noexcept { _lookAtPitchOffset = clampAngle(_lookAtPitchOffset + amount); }
        void addLookAtYawOffset(float amount) noexcept { _lookAtYawOffset = clampAngle(_lookAtYawOffset + amount); }
        void addLookAtRollOffset(float amount) noexcept { _lookAtRollOffset = clampAngle(_lookAtRollOffset + amount); }

        void resetLookAtOffsets() noexcept {
            _lookAtPitchOffset = 0.0f;
            _lookAtYawOffset = 0.0f;
            _lookAtRollOffset = 0.0f;
            _lookAtTargetOffset = { 0.0f, 0.0f, 0.0f };
        }

        [[nodiscard]] float getLookAtPitchOffset() const noexcept { return _lookAtPitchOffset; }
        [[nodiscard]] float getLookAtYawOffset() const noexcept { return _lookAtYawOffset; }
        [[nodiscard]] float getLookAtRollOffset() const noexcept { return _lookAtRollOffset; }

        // Look-at target position offset functionality (new system - in player's local coordinate system)
        void addLookAtTargetForward(float amount) noexcept { _lookAtTargetOffset.z += amount; }   // Forward/backward relative to player
        void addLookAtTargetRight(float amount) noexcept { _lookAtTargetOffset.x += amount; }     // Right/left relative to player  
        void addLookAtTargetUp(float amount) noexcept { _lookAtTargetOffset.y += amount; }        // Up/down relative to player

        [[nodiscard]] XMFLOAT3 getLookAtTargetOffset() const noexcept { return _lookAtTargetOffset; }
        void setLookAtTargetOffset(const XMFLOAT3& offset) noexcept { _lookAtTargetOffset = offset; }

        // Calculate look-at rotation from camera position to target position (with optional offset)
        [[nodiscard]] XMFLOAT3 calculateLookAtRotation(const XMFLOAT3& cameraPos,
            const XMFLOAT3& targetPos) const noexcept;

        // Calculate the actual look-at target position with offset applied
        [[nodiscard]] XMFLOAT3 calculateOffsetTargetPosition(const XMFLOAT3& playerPos,
            const XMVECTOR& playerRotation) const noexcept;

        // Height-locked coordinate calculation (maintains Y position during horizontal movement)
        [[nodiscard]] XMFLOAT3 calculateNewCoordsHeightLocked(const XMFLOAT3& currentCoords,
            FXMVECTOR lookQ, bool preserveHeight) noexcept;

        void updateCameraEffectSettings(const Settings& s) noexcept;
        void applyShakeEffect(XMFLOAT3& position, XMVECTOR& rotation, float deltaTime) noexcept;
        void applyHandheldCameraEffect(XMFLOAT3& position, XMVECTOR& rotation, float& fov, float deltaTime) noexcept;

        // Get the current look-at target position for visualization (only valid in target offset mode)
        [[nodiscard]] XMFLOAT3 getCurrentLookAtTargetPosition() const noexcept { return _currentLookAtTargetPosition; }
        [[nodiscard]] bool hasValidLookAtTarget() const noexcept { return _hasValidLookAtTarget; }
		[[nodiscard]] bool getCameraLookAtVisualizationEnabled() const noexcept { return _freeCameraLookAtVisualization; }
		void setCameraLookAtVisualization(bool enabled) noexcept { _freeCameraLookAtVisualization = enabled; }

        XMFLOAT3 getInternalPosition() const { return _toolsCoordinates; }

    private:
        Camera() = default;
        ~Camera() = default;

        Camera(const Camera&) = delete;
        Camera& operator=(const Camera&) = delete;
        Camera(Camera&&) = delete;
        Camera& operator=(Camera&&) = delete;

        // Camera mode handlers (each handles position and rotation)
        void handleFixedMountMode() noexcept;
        void handleLookAtMode(float positionLerpTime, float rotationLerpTime) noexcept;
        void handleNormalMode(float positionLerpTime, float rotationLerpTime) noexcept;

        // Interpolation helpers
        void interpolatePosition(float lerpTime) noexcept;
        void interpolateRotation(float lerpTime) noexcept;
        void interpolateFOV(float lerpTime) noexcept;

        // Camera transformation helpers
        void applyFixedMountTransformation(const XMVECTOR& playerPos,
            const XMVECTOR& playerRot) noexcept;
        void applyLookAtRotation(const XMFLOAT3& cameraPos,
            const XMFLOAT3& playerPos,
            const XMVECTOR& playerRot) noexcept;

        // Final camera update
        void applyFinalCameraTransform(float deltaTime) noexcept;

        //  ------------------------------------------------- Data --------------------------------
        XMVECTOR _interpolatedQuaternion{ XMQuaternionIdentity() };
    	float _yaw{ 0.0f };
        float _pitch{ 0.0f };
        float _roll{ 0.0f };
        XMFLOAT3 _direction{ 0,0,0 }; // current movement

        //  Target values for smooth interpolation
        float _targetyaw{ 0.0f };
        float _targetpitch{ 0.0f };
        float _targetroll{ 0.0f };
        XMFLOAT3 _targetdirection{ 0,0,0 };

        float _targetfov{ 0.0f };
        float _fov{ 0.0f };

        XMFLOAT4 _toolsQuaternion{ 0,0,0,0 };
		XMFLOAT3 _toolsCoordinates{ 0,0,0 };
		XMMATRIX _toolsMatrix{};

    	XMFLOAT3 _rightVector{};
        XMFLOAT3 _upVector{};
        XMFLOAT3 _forwardVector{};
        XMFLOAT3 _gameEulers{};
        XMFLOAT4 _gameQuaternion{};

        float _lookDirectionInverter{ 1.0f };
        bool  _movementOccurred{ false };

        bool _lookAtPlayerEnabled{ false };

        // Look-at offset angles from stick input
        float _lookAtPitchOffset{ 0.0f };
        float _lookAtYawOffset{ 0.0f };
        float _lookAtRollOffset{ 0.0f };

        XMFLOAT3 _lookAtTargetOffset{ 0.0f, 0.0f, 0.0f };  // x=right, y=up, z=forward relative to player
        bool _useTargetOffsetMode{ true };  // true = offset target position, false = offset angles
        // Height-locked movement (prevents camera Y position changes during horizontal movement)
        bool _heightLockedMovement{ false };  // true = maintain Y position during movement

        // Fixed camera mount (camera locked to relative position/rotation from player like physically mounted)
        bool _fixedCameraMountEnabled{ false };  // true = camera is virtually mounted to player
        XMFLOAT3 _fixedMountPositionOffset{ 0.0f, 0.0f, 0.0f };  // relative position in player's local space
        XMMATRIX _fixedMountRelativeTransform{ XMMatrixIdentity() };  // relative transformation matrix from player to camera


        //  Euler order constant (compile time selectable for debug)
#ifdef DEBUG
        Utils::EulerOrder _eulerOrder = Utils::EulerOrder::XYZ;
#else
        Utils::EulerOrder _eulerOrder = MULTIPLICATION_ORDER;
#endif

        //  Helpers -------------------------------------------------------------------------------
        static float clampAngle(float angle) noexcept;

        // Camera shake (simple effect)
        bool _shakeEnabled{ false };
        float _shakeTime{ 0.0f };
        float _shakeAmplitude{ 0.0f };
        float _shakeFrequency{ 0.0f };

        // Handheld camera (complex effect)
        bool _handheldEnabled{ false };
        float _handheldTimeAccumulator{ 0.0f };
        float _handheldIntensity{ 1.0f };
        float _handheldDriftIntensity{ 1.0f };
        float _handheldJitterIntensity{ 1.0f };
        float _handheldBreathingIntensity{ 0.0f };
        float _handheldBreathingRate{ 0.0f };
        float _handheldDriftSpeed{ 0.05f };
        float _handheldRotationDriftSpeed{ 0.03f };
        bool _handheldPositionEnabled{ true };
        bool _handheldRotationEnabled{ true };
        XMVECTOR _handheldPositionVelocity{ XMVectorZero() };
        XMVECTOR _handheldRotationVelocity{ XMVectorZero() };

        // Store the current look-at target position for visualization (only valid in target offset mode)
        XMFLOAT3 _currentLookAtTargetPosition{ 0.0f, 0.0f, 0.0f };
        bool _hasValidLookAtTarget{ false };
        bool _freeCameraLookAtVisualization = false;
    };
} // namespace IGCS