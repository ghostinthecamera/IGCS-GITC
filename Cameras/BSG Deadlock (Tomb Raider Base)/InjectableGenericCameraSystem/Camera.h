////////////////////////////////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System
// Copyright(c) 2016, Frans Bouma
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
#pragma once
#include "stdafx.h"

using namespace DirectX;

class Camera
{
public:
	Camera();
	~Camera(void);

	XMVECTOR CalculateLookQuaternion(XMVECTOR currentLookQ);
	XMVECTOR Camera::CalculateLookQuaternion();
	XMFLOAT3 CalculateNewCoords(const XMFLOAT3 currentCoords, const XMVECTOR lookQ);

	void ResetDeltas();
	void ResetAngles();
	void MoveForward(float amount);
	void MoveRight(float amount);
	void MoveUp(float amount);
	void Yaw(float amount);
	void Pitch(float amount);
	void Roll(float amount);
	void SetPitch(float angle);
	void SetYaw(float angle);
	void SetRoll(float angle);

private:
	XMFLOAT3 m_direction = XMFLOAT3(0.0f, 0.0f, 0.0f); // camera direction, defined by move methods. Reset by ResetDeltas
	float m_yaw;
	float m_pitch;
	float m_roll;
	float m_yawDelta;
	float m_rollDelta;
	float m_pitchDelta;
	bool m_movementOccurred;
	float m_movementSpeed;
	float m_rotationSpeed;

	float ClampAngle(float angle) const;
};
