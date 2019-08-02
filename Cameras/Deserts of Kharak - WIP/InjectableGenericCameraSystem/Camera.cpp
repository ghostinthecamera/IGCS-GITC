////////////////////////////////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System
// Copyright(c) 2019, Frans Bouma
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
#include "Camera.h"
#include "Defaults.h"
#include "GameConstants.h"
#include "Globals.h"

using namespace DirectX;

namespace IGCS
{
	Camera::Camera() : _yaw(0), _pitch(0), _roll(0), _movementOccurred(false), _lookDirectionInverter(1.0f), _direction(XMFLOAT3(0.0f, 0.0f, 0.0f))
	{
	}


	Camera::~Camera(void)
	{
	}


	XMVECTOR Camera::calculateLookQuaternion()
	{
		XMVECTOR xQ = XMQuaternionRotationNormal(XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f), -_pitch);
		XMVECTOR yQ = XMQuaternionRotationNormal(XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f), _roll);
		XMVECTOR zQ = XMQuaternionRotationNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f), -_yaw);

		XMVECTOR tmpQ = XMQuaternionMultiply(xQ, zQ);
		XMVECTOR qToReturn = XMQuaternionMultiply(yQ, tmpQ);
		XMQuaternionNormalize(qToReturn);
		return qToReturn;
	}


	void Camera::resetMovement()
	{
		_movementOccurred = false;
		_direction.x = 0.0f;
		_direction.y = 0.0f;
		_direction.z = 0.0f;
	}


	void Camera::resetAngles()
	{
		setPitch(INITIAL_PITCH_RADIANS);
		setRoll(INITIAL_ROLL_RADIANS);
		setYaw(INITIAL_YAW_RADIANS);
	}


	XMFLOAT3 Camera::calculateNewCoords(const XMFLOAT3 currentCoords, const XMVECTOR lookQ)
	{
		XMFLOAT3 toReturn;
		toReturn.x = currentCoords.x;
		toReturn.y = currentCoords.y;
		toReturn.z = currentCoords.z;
		if (_movementOccurred)
		{
			XMVECTOR directionAsQ = XMVectorSet(_direction.x, _direction.y, _direction.z, 0.0f);
			XMVECTOR newDirection = XMVector3Rotate(directionAsQ, lookQ);
			toReturn.x += XMVectorGetX(newDirection);
			toReturn.y += XMVectorGetY(newDirection);
			toReturn.z += XMVectorGetZ(newDirection);
		}
		return toReturn;
	}


	void Camera::moveForward(float amount)
	{
		_direction.y += (Globals::instance().settings().movementSpeed * amount);		// y out of the screen, z up
		_movementOccurred = true;
	}

	void Camera::moveRight(float amount)
	{
		_direction.x += (Globals::instance().settings().movementSpeed * amount);		// x is right
		_movementOccurred = true;
	}

	void Camera::moveUp(float amount)
	{
		_direction.z += (Globals::instance().settings().movementSpeed * amount * Globals::instance().settings().movementUpMultiplier);		// z is up
		_movementOccurred = true;
	}

	void Camera::yaw(float amount)
	{
		_yaw += (Globals::instance().settings().rotationSpeed * amount);
		_yaw = clampAngle(_yaw);
	}

	void Camera::pitch(float amount)
	{
		float lookDirectionInverter = _lookDirectionInverter;
		if (Globals::instance().settings().invertY)
		{
			lookDirectionInverter = -lookDirectionInverter;
		}
		_pitch -= (Globals::instance().settings().rotationSpeed * amount * lookDirectionInverter);			// y is left, so inversed
		_pitch = clampAngle(_pitch);
	}

	void Camera::roll(float amount)
	{
		_roll += (Globals::instance().settings().rotationSpeed * amount);
		_roll = clampAngle(_roll);
	}

	void Camera::setPitch(float angle)
	{
		_pitch = clampAngle(angle);
	}

	void Camera::setYaw(float angle)
	{
		_yaw = clampAngle(angle);
	}

	void Camera::setRoll(float angle)
	{
		_roll = clampAngle(angle);
	}

	// Keep the angle in the range 0 to 360 (2*PI)
	float Camera::clampAngle(float angle) const
	{
		while (angle > XM_2PI)
		{
			angle -= XM_2PI;
		}
		while (angle < 0)
		{
			angle += XM_2PI;
		}
		return angle;
	}
}