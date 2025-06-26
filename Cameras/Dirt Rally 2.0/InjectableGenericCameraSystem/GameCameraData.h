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
#pragma once
#include "stdafx.h"
#include "DirectXMath.h"
#include "GameConstants.h"

namespace IGCS
{
	// Simple struct which is used to cache game data. 
	struct GameCameraData
	{
		float _coords[sizeof(DirectX::XMFLOAT3)];
		//float _matrix[sizeof(DirectX::XMFLOAT3X3)];
		float _quaternion[sizeof(DirectX::XMFLOAT4)];
		float _fov;

		void CacheData(const float* qInMemory, const float* coordsInMemory, const float* fovInMemory)
		{
			if (nullptr != qInMemory)
			{
				memcpy(_quaternion, qInMemory, sizeof(DirectX::XMFLOAT4));
			}
			if (nullptr != coordsInMemory)
			{
				memcpy(_coords, coordsInMemory, sizeof(DirectX::XMFLOAT3));
			}
			if (nullptr != fovInMemory)
			{
				_fov = *fovInMemory;
			}
		}

		void RestoreData(float* pInMemory, float* coordsInMemory, float* fovInMemory) const
		{
			if (nullptr != pInMemory)
			{
				memcpy(pInMemory, _quaternion, sizeof(DirectX::XMFLOAT4));
			}
			if (nullptr != coordsInMemory)
			{
				memcpy(coordsInMemory, _coords, sizeof(DirectX::XMFLOAT3));
			}
			if (nullptr != fovInMemory)
			{
				*fovInMemory = _fov;
			}
		}
	};

	struct GameAddressData
	{
		uint8_t* cameraAddress = nullptr;
		uint8_t* camQuaternionAddress = nullptr;
		uint8_t* camPositionAddress = nullptr;
		uint8_t* carPositionAddress = nullptr;
		uint8_t* timescaleAddress = nullptr;
		uint8_t* dofStrengthAddress = nullptr;
	};

	/// <summary>
	/// IGCSDOF
	/// </summary>

	struct IGCSSessionCacheData
	{
		DirectX::XMFLOAT4 quaternion;
		DirectX::XMFLOAT3 eulers;
		DirectX::XMFLOAT3 Coordinates;
		float fov;
	};
}
