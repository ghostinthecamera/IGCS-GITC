// ================== Camera.h ==================
#pragma once

#include <DirectXMath.h>

#include "Utils.h"

namespace IGCS
{
    using namespace DirectX;
    class Camera
    {
    public:
        static Camera& instance() noexcept
        {
            static Camera instance;
            return instance;
        }

        // ---- Public interface ------------------------------------------------------------------
        DirectX::XMVECTOR calculateLookQuaternion() noexcept;
        [[nodiscard]] DirectX::XMFLOAT3 calculateNewCoords(const DirectX::XMFLOAT3& currentCoords,
            DirectX::FXMVECTOR lookQ) noexcept;

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
        [[nodiscard]] DirectX::XMFLOAT3 getUpVector()      const noexcept { return _upVector; }
        [[nodiscard]] DirectX::XMFLOAT3 getRightVector()   const noexcept { return _rightVector; }
        [[nodiscard]] DirectX::XMFLOAT3 getForwardVector() const noexcept { return _forwardVector; }
        [[nodiscard]] DirectX::XMFLOAT3 getGameEulers()    const noexcept { return _gameEulers; }
        [[nodiscard]] DirectX::XMFLOAT4 getGameQuaternion()const noexcept { return _gameQuaternion; }
        [[nodiscard]] DirectX::XMFLOAT4 getToolsQuaternion() const noexcept { return _toolsQuaternion; }
		[[nodiscard]] DirectX::XMVECTOR getToolsQuaternionVector() const noexcept { return XMLoadFloat4(&_toolsQuaternion); }
		[[nodiscard]] DirectX::XMFLOAT3 getToolsCoordinates() const noexcept { return _toolsCoordinates; }
		[[nodiscard]] DirectX::XMMATRIX getToolsMatrix() const noexcept { return _toolsMatrix; }

        void setUpVector(DirectX::XMFLOAT3 v) noexcept { _upVector = v; }
        void setRightVector(DirectX::XMFLOAT3 v) noexcept { _rightVector = v; }
        void setForwardVector(DirectX::XMFLOAT3 v) noexcept { _forwardVector = v; }
        void setGameEulers(DirectX::XMFLOAT3 v) noexcept { _gameEulers = v; }
        void setGameQuaternion(DirectX::XMFLOAT4 v) noexcept { _gameQuaternion = v; }
        void setToolsQuaternion(DirectX::XMFLOAT4 q) noexcept { _toolsQuaternion = q; }

        //  Euler bridging ------------------------------------------------------------------------
        void setAllRotation(DirectX::XMFLOAT3 eulers) noexcept;
        void setTargetEulers(DirectX::XMFLOAT3 eulers) noexcept;
        void setEulers(DirectX::XMFLOAT3 eulers) noexcept;

        //  Interpolation -------------------------------------------------------------------------
        [[nodiscard]] DirectX::XMFLOAT3 getDirection() const noexcept { return _direction; }
        [[nodiscard]] DirectX::XMFLOAT3 getTargetDirection() const noexcept { return _targetdirection; }
        void setDirection(DirectX::XMFLOAT3 dir) noexcept { _direction = dir; }
        void setTargetDirection(DirectX::XMFLOAT3 dir) noexcept { _targetdirection = dir; }

        [[nodiscard]] DirectX::XMFLOAT3 getRotation() const noexcept { return { _pitch, _yaw, _roll }; }
        [[nodiscard]] DirectX::XMFLOAT3 getTargetRotation() const noexcept { return { _targetpitch,_targetyaw,_targetroll }; }
        void setRotation(DirectX::XMFLOAT3 rot) noexcept { setPitch(rot.x); setYaw(rot.y); setRoll(rot.z); }
		void setInterpolatedQuaternation(XMVECTOR q) { _interpolatedQuaternion = q; }
		[[nodiscard]] DirectX::XMVECTOR getInterpolatedQuaternion() const { return _interpolatedQuaternion; }

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

    private:
        Camera() = default;
        ~Camera() = default;

        Camera(const Camera&) = delete;
        Camera& operator=(const Camera&) = delete;
        Camera(Camera&&) = delete;
        Camera& operator=(Camera&&) = delete;

        //  ------------------------------------------------- Data --------------------------------
        DirectX::XMVECTOR _interpolatedQuaternion{ XMQuaternionIdentity() };
    	float _yaw{ 0.0f };
        float _pitch{ 0.0f };
        float _roll{ 0.0f };
        DirectX::XMFLOAT3 _direction{ 0,0,0 }; // current movement

        //  Target values for smooth interpolation
        float _targetyaw{ 0.0f };
        float _targetpitch{ 0.0f };
        float _targetroll{ 0.0f };
        DirectX::XMFLOAT3 _targetdirection{ 0,0,0 };

        float _targetfov{ 0.0f };
        float _fov{ 0.0f };

        DirectX::XMFLOAT4 _toolsQuaternion{ 0,0,0,0 };
		DirectX::XMFLOAT3 _toolsCoordinates{ 0,0,0 };
		DirectX::XMMATRIX _toolsMatrix{};

    	DirectX::XMFLOAT3 _rightVector{};
        DirectX::XMFLOAT3 _upVector{};
        DirectX::XMFLOAT3 _forwardVector{};
        DirectX::XMFLOAT3 _gameEulers{};
        DirectX::XMFLOAT4 _gameQuaternion{};

        float _lookDirectionInverter{ 1.0f };
        bool  _movementOccurred{ false };

        //  Euler order constant (compile time selectable for debug)
#ifdef DEBUG
        Utils::EulerOrder _eulerOrder = Utils::EulerOrder::XYZ;
#else
        Utils::EulerOrder _eulerOrder = MULTIPLICATION_ORDER;
#endif

        //  Helpers -------------------------------------------------------------------------------
        static float clampAngle(float angle) noexcept;
    };
} // namespace IGCS