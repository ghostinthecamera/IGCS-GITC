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
#include "SimpleMath.h"
#include "GameConstants.h"

namespace IGCS
{
	using namespace DirectX::SimpleMath;
	// Simple struct which is used to cache game data. 
	struct GameCameraData
	{
		float _coords[COORD_SIZE];
		float _matrix[MATRIX_SIZE];
		float _hfov;
		float _vfov;
		float _AspectRatio;
		float _bloom;
		DirectX::XMFLOAT3 _initialisedCoords;

		void CacheData(float* matrixInMemory, float* coordsInMemory, float* hfovInMemory, float* vfovInMemory, float* ARinMemory, float* bloominMemory)
		{
			if (nullptr != matrixInMemory)
			{
				memcpy(_matrix, matrixInMemory, MATRIX_SIZE * sizeof(float));
			}
			if (nullptr != coordsInMemory)
			{
				memcpy(_coords, coordsInMemory, COORD_SIZE * sizeof(float));
			}
			if (nullptr != hfovInMemory)
			{
				_hfov = *hfovInMemory;
			}
			if (nullptr != vfovInMemory)
			{
				_vfov = *vfovInMemory;
			}
			if (nullptr != ARinMemory)
			{
				_AspectRatio = *ARinMemory;
			}
			if (nullptr != bloominMemory)
			{
				_bloom = *bloominMemory;
			}
		}

		void RestoreData(float* matrixInMemory, float* coordsInMemory, float* hfovInMemory, float* vfovInMemory, float* ARinMemory, float* bloominMemory)
		{
			if (nullptr != matrixInMemory)
			{
				memcpy(matrixInMemory, _matrix, MATRIX_SIZE * sizeof(float));
			}
			if (nullptr != coordsInMemory)
			{
				memcpy(coordsInMemory, _coords, COORD_SIZE * sizeof(float));
			}
			if (nullptr != hfovInMemory)
			{
				*hfovInMemory = _hfov;
			}
			if (nullptr != vfovInMemory)
			{
				*vfovInMemory = _vfov;
			}
			if (nullptr != ARinMemory)
			{
				*ARinMemory = _AspectRatio;
			}
			if (nullptr != bloominMemory)
			{
				*bloominMemory = _bloom;
			}
		}

		void storeInitCoords(DirectX::XMFLOAT3 initCoords)
		{
			_initialisedCoords = initCoords;
		}

		DirectX::XMFLOAT3 getInitCoords()
		{
			return _initialisedCoords;
		}
	};

	struct GameAddressData
	{
		LPBYTE cameraAddress;
		LPBYTE timescaleAddress;
		LPBYTE ARAddress;
		LPBYTE bloomAddress;
	};

	/// <summary>
	/// IGCSDOF
	/// </summary>

	struct igcsSessionCacheData
	{
		DirectX::XMFLOAT4 quaternion;
		DirectX::XMFLOAT3 eulers;
		DirectX::XMFLOAT3 Coordinates;
		float fov;
	};
}
