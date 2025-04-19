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


	XMVECTOR Camera::getEulerVector()
	{
		XMFLOAT3 _currentEuler = XMFLOAT3(_pitch, _roll, _yaw);
		//float convertFactor = 180.0f / XM_PI;
		XMVECTOR currentEulerToReturn = XMLoadFloat3(&_currentEuler);
		return currentEulerToReturn;
	}

	XMVECTOR Camera::calculateLookQuaternion(const std::string& order)
	{
		XMVECTOR xQ = XMQuaternionRotationNormal(XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f), -_pitch);
		XMVECTOR yQ = XMQuaternionRotationNormal(XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f), -_roll);  // -
		XMVECTOR zQ = XMQuaternionRotationNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f), -_yaw);

		XMVECTOR qToReturn = XMQuaternionMultiply(XMQuaternionMultiply(xQ, zQ),yQ);
		//XMVECTOR qToReturn = XMQuaternionMultiply(zQ, tmpQ);
	   // XMQuaternionInverse(qToReturn);
		XMQuaternionNormalize(qToReturn);

		//XMFLOAT3 _currentEuler = XMFLOAT3(-_pitch, -_roll, -_yaw);
		//XMVECTOR angles = XMLoadFloat3(&_currentEuler);

		//XMVECTOR qToReturn = XMQuaternionRotationRollPitchYawFromVector(XMVectorSet(-_pitch,-_roll,-_yaw,0.0f));

		//	 //Create quaternions for yaw, pitch, and roll
		//XMVECTOR qPitch = XMQuaternionRotationAxis(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), -_pitch);
		//XMVECTOR qYaw = XMQuaternionRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), -_roll);
		//XMVECTOR qRoll = XMQuaternionRotationAxis(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), -_yaw);

		//// Multiply quaternions based on the specified order
		//XMVECTOR q;
		//if (order == "YPR")
		//	q = XMQuaternionMultiply(XMQuaternionMultiply(qYaw, qPitch), qRoll);
		//else if (order == "YRP")
		//	q = XMQuaternionMultiply(XMQuaternionMultiply(qYaw, qRoll), qPitch);
		//else if (order == "PRY")
		//	q = XMQuaternionMultiply(XMQuaternionMultiply(qPitch, qRoll), qYaw);
		//else if (order == "PYR")
		//	q = XMQuaternionMultiply(XMQuaternionMultiply(qPitch, qYaw), qRoll);
		//else if (order == "RYP")
		//	q = XMQuaternionMultiply(XMQuaternionMultiply(qRoll, qYaw), qPitch);
		//else if (order == "RPY")
		//	q = XMQuaternionMultiply(XMQuaternionMultiply(qRoll, qPitch), qYaw);
		//else
		//	throw std::invalid_argument("Invalid order string. Use one of: YPR, YRP, PRY, PYR, RYP, RPY.");

		//// Extract quaternion components
		//float qx = XMVectorGetX(q);
		//float qy = XMVectorGetY(q);
		//float qz = XMVectorGetZ(q);
		//float qw = XMVectorGetW(q);

		//// Reorder to match qx, -qz, qy, qw
		//XMFLOAT4 quat;
		//quat.x = qx;
		//quat.y = qy;
		//quat.z = -qz; // Negate qz to match the desired structure
		//quat.w = qw;

		//XMVECTOR qToReturn = XMLoadFloat4(&quat);
		//XMQuaternionNormalize(qToReturn);

		////////////////////////////
		// Half angles
		//float hy = _yaw * 0.5f;
		//float hp = _pitch * 0.5f;
		//float hr = _roll * 0.5f;

		//// Calculate cosines and sines of the half angles
		//float cy = cosf(hy);
		//float sy = sinf(hy);
		//float cp = cosf(hp);
		//float sp = sinf(hp);
		//float cr = cosf(hr);
		//float sr = sinf(hr);

		//// Calculate the quaternion components
		//float qw = cr * cp * cy + sr * sp * sy;
		//float qx = sr * cp * cy - cr * sp * sy;
		//float qy = cr * sp * cy + sr * cp * sy;
		//float qz = cr * cp * sy - sr * sp * cy;

		//// Return the quaternion in the desired order: qx, -qz, qy, qw
		//XMFLOAT4 quat = XMFLOAT4(qx, qy, -qz, qw);

		//XMVECTOR qToReturn = XMLoadFloat4(&quat);
		//XMQuaternionNormalize(qToReturn);
		//////////////////

		return qToReturn;
	}

	XMVECTOR Camera::calculateLookQuaternionSecond()
	{
		XMVECTOR xQ = XMQuaternionRotationNormal(XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f), _pitch); //-
		XMVECTOR yQ = XMQuaternionRotationNormal(XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f), _roll);  //-
		XMVECTOR zQ = XMQuaternionRotationNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f), _yaw);   //-

		XMVECTOR tmpQ = XMQuaternionMultiply(xQ, yQ);
		XMVECTOR qToReturn = XMQuaternionMultiply(zQ, tmpQ);
		XMQuaternionNormalize(qToReturn);
		//XMQuaternionInverse(qToReturn);
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

	XMFLOAT3 Camera::calculateNewCoordsSecond(const XMFLOAT3 currentCoords, const XMVECTOR lookQ)
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
		_direction.z += (Globals::instance().settings().movementSpeed * amount);		// y out of the screen, z up // used to be -
		_movementOccurred = true;
	}

	void Camera::moveRight(float amount)
	{
		_direction.x -= (Globals::instance().settings().movementSpeed * amount);		// x is right
		_movementOccurred = true;
	}

	void Camera::moveUp(float amount)
	{
		_direction.y += (Globals::instance().settings().movementSpeed * amount * Globals::instance().settings().movementUpMultiplier);		// z is up //used to be -
		_movementOccurred = true;
	}

	void Camera::yaw(float amount)
	{
		_roll += (Globals::instance().settings().rotationSpeed * amount);
		//_yaw = clampAngle(_yaw);
	}

	void Camera::pitch(float amount)
	{
		float lookDirectionInverter = _lookDirectionInverter;
		if (Globals::instance().settings().invertY)
		{
			lookDirectionInverter = -lookDirectionInverter;
		}
		_pitch += (Globals::instance().settings().rotationSpeed * amount * lookDirectionInverter);			// y is left, so inversed
		//_pitch = clampAngle(_pitch);
	}

	void Camera::roll(float amount)
	{
		_yaw += (Globals::instance().settings().rotationSpeed * amount);
		//_roll = clampAngle(_roll);
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