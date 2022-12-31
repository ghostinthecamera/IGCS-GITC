////////////////////////////////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System
// Copyright(c) 2017, Frans Bouma
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
#include "CameraManipulator.h"
#include "GameConstants.h"
#include "Globals.h"
#include "Camera.h"
#include "GameCameraData.h"
#include "MessageHandler.h"

using namespace DirectX;
using namespace std;

extern "C" {
	LPBYTE g_cameraStructAddress = nullptr;
	LPBYTE g_dofstructaddress = nullptr;
	LPBYTE g_timescaleaddress = nullptr; }

namespace IGCS::GameSpecific::CameraManipulator
{
	static GameCameraData _originalData;
	float prevYaw = 0.0f;
	float prevPitch = 0.0f;
	float prevRoll = 0.0f;

	void updateCameraDataInGameData(Camera& camera)
	{
		if (!g_cameraEnabled)
		{
			return;
		}
		
		DirectX::XMVECTOR newLookQuaternion = camera.calculateLookQuaternion();
		DirectX::XMFLOAT3 currentCoords;
		DirectX::XMFLOAT3 newCoords;

		if (isCameraFound())
		{
				currentCoords = getCurrentCameraCoords();
				newCoords = camera.calculateNewCoords(currentCoords, newLookQuaternion);
				writeNewCameraValuesToGameData(newCoords, newLookQuaternion);
		}
	}

	XMFLOAT3 getCurrentCameraCoords()
	{

		float* coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		return XMFLOAT3(coordsInMemory[0], coordsInMemory[1], coordsInMemory[2]);
	}


	void getSettingsFromGameState()
	{
		Settings& currentSettings = Globals::instance().settings();
		// nop
	}


	void applySettingsToGameState()
	{
		Settings& currentSettings = Globals::instance().settings();
		// nop
	}


	// Resets the FOV to the one it got when we enabled the camera
	void resetFoV()
	{
		if (nullptr == g_cameraStructAddress)
		{
			return;
		}
		float* fovAddress = reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET);
		*fovAddress = _originalData._fov;
	}


	// changes the FoV with the specified amount
	void changeFoV(float amount)
	{
		if (g_cameraStructAddress == nullptr)
		{
			return;
		}

		float* fovAddress = reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET);
		float newValue = *fovAddress;

		if (newValue < 0.25f)
		{
			newValue = *fovAddress + (amount / 4);
		}
		else if (newValue > 0.8f)
		{
			newValue = *fovAddress + (amount * 4);
		}
		else newValue = *fovAddress + amount;

		if (newValue < 0.05f)
		{
			// clamp. Game will crash with negative fov
			newValue = 0.05f;
		}
		else if (newValue > 0.95f)
		{
			newValue = 0.95f;
		}

		*fovAddress = newValue;
	}

	void toggleDOF() //and bloom
	{

		if (g_dofstructaddress == nullptr)
		{
			MessageHandler::logError("DOF Struct not found");
			return;
		}

		uint8_t* dof = reinterpret_cast<uint8_t*>(g_dofstructaddress + DOF_OFFSET);
		*dof = (*dof == (uint8_t)4) ? (uint8_t)0 : (uint8_t)4;
	}

	void timestop()
	{
		if (g_timescaleaddress == nullptr)
		{
			MessageHandler::logError("Timescale not Found");
			return;
		}

		float* timescale = reinterpret_cast<float*>(g_timescaleaddress + TIMESCALE_OFFSET);
		*timescale = (*timescale > 0.95f ? 0.00001f : 1.0f);
	}

	void sloMoFunc(float amount)
	{
		if (nullptr == g_timescaleaddress)
		{
			MessageHandler::logError("Timescale not Found");
			return;
		}
		if (!g_cameraEnabled)
		{
			float* timescaleInMemory = reinterpret_cast<float*>(g_timescaleaddress + TIMESCALE_OFFSET);
			*timescaleInMemory = *timescaleInMemory > 0.95f ? amount : 1.0f;
		}
	}

	// newLookQuaternion: newly calculated quaternion of camera view space. Can be used to construct a 4x4 matrix if the game uses a matrix instead of a quaternion
	// newCoords are the new coordinates for the camera in worldspace.
	void writeNewCameraValuesToGameData(XMFLOAT3 newCoords, XMVECTOR newLookQuaternion)
	{
		if (!isCameraFound())
		{
			return;
		}

		XMFLOAT4X4 rotationMatrix;

		XMMATRIX rotationMatrixPacked = DirectX::XMMatrixRotationQuaternion(newLookQuaternion);
		XMStoreFloat4x4(&rotationMatrix, rotationMatrixPacked);

		float* coordsInMemory = nullptr;
		float* matrixInMemory = nullptr;

		
		matrixInMemory = reinterpret_cast<float*>(g_cameraStructAddress + MATRIX_INSTRUCT_OFFSET);
		matrixInMemory[0] = rotationMatrix._11;
		matrixInMemory[1] = rotationMatrix._12;
		matrixInMemory[2] = rotationMatrix._13;
		matrixInMemory[3] = 0.0f;
		matrixInMemory[4] = rotationMatrix._21;
		matrixInMemory[5] = rotationMatrix._22;
		matrixInMemory[6] = rotationMatrix._23;
		matrixInMemory[7] = 0.0f;
		matrixInMemory[8] = rotationMatrix._31;
		matrixInMemory[9] = rotationMatrix._32;
		matrixInMemory[10] = rotationMatrix._33;
		matrixInMemory[11] = 0.0f;
	
		coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		coordsInMemory[0] = newCoords.x;
		coordsInMemory[1] = newCoords.y;
		coordsInMemory[2] = newCoords.z;
		coordsInMemory[3] = 1.0f;
	}


	bool isCameraFound()
	{
		return nullptr != g_cameraStructAddress;
	}


	void displayCameraStructAddress()
	{
		MessageHandler::logDebug("Camera struct address: %p", (void*)g_cameraStructAddress);
		MessageHandler::logDebug("DOF struct address: %p", (void*)g_dofstructaddress);
		MessageHandler::logDebug("Timescale struct address: %p", (void*)g_timescaleaddress);
		//MessageHandler::logDebug("Bloom address: %p", (void*)g_bloomstructaddress);
	}


	void restoreGameCameraDataWithCachedData(GameCameraData& source)
	{
		if (!isCameraFound())
		{
			return;
		}
		source.RestoreData(reinterpret_cast<float*>(g_cameraStructAddress + MATRIX_INSTRUCT_OFFSET), reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET),
						   reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET));
	}


	void cacheGameCameraDataInCache(GameCameraData& destination)
	{
		if (!isCameraFound())
		{
			return;
		}
		destination.CacheData(reinterpret_cast<float*>(g_cameraStructAddress + MATRIX_INSTRUCT_OFFSET), reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET),
							  reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET));
	}


	void restoreOriginalValuesAfterCameraDisable()
	{
		restoreGameCameraDataWithCachedData(_originalData);
	}


	void cacheOriginalValuesBeforeCameraEnable()
	{
		cacheGameCameraDataInCache(_originalData);
	}
}