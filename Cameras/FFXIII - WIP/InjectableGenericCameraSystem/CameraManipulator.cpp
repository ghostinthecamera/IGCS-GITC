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
	LPBYTE g_ARvalueaddress = nullptr;
	LPBYTE g_bloomstructaddress = nullptr; }

namespace IGCS::GameSpecific::CameraManipulator
{
	static GameCameraData _originalData;
	static float OGfardof = 0.0f;
	static float OGneardof = 0.0f;
	static float* AR = nullptr;
	static float OGBloom = 0.0f;
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
				currentCoords = initialiseCamera();
				newCoords = camera.calculateNewCoords(currentCoords, newLookQuaternion);
				writeNewCameraValuesToGameData(newCoords, newLookQuaternion);
		}
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
		float* fovAddress = reinterpret_cast<float*>(g_cameraStructAddress + HFOV_IN_STRUCT_OFFSET);
		float* vfovAddress = reinterpret_cast<float*>(g_cameraStructAddress + VFOV_IN_STRUCT_OFFSET);
		*fovAddress = _originalData._hfov;
		*vfovAddress = _originalData._vfov;
	}


	// changes the FoV with the specified amount
	void changeFoV(float amount)
	{
		if (g_cameraStructAddress == nullptr)
		{
			return;
		}
		if (g_ARvalueaddress == nullptr)
		{
			MessageHandler::logLine("AR not found - using default AR value of 1.78");
		}

		float* hfovAddress = reinterpret_cast<float*>(g_cameraStructAddress + HFOV_IN_STRUCT_OFFSET);
		float* vfovAddress = reinterpret_cast<float*>(g_cameraStructAddress + VFOV_IN_STRUCT_OFFSET);
		float hnewValue = *hfovAddress;

		if (hnewValue < 0.9f)
		{
			hnewValue = *hfovAddress - (amount / 4);
		}
		else if (hnewValue > 3.0f)
		{
			hnewValue = *hfovAddress - (amount * 4);
		}
		else hnewValue = *hfovAddress - amount;

		float vnewValue = hnewValue * *AR;

		if (hnewValue < 0.2f)
		{
			// clamp. Game will crash with negative fov
			hnewValue = 0.2f;
		}
		*hfovAddress = hnewValue;
		*vfovAddress = vnewValue;
	}

	void getAR()
	{
		if (g_ARvalueaddress == nullptr)
		{
			MessageHandler::logError("AR not found - using default AR value of 1.78");
			*AR = 1.78f;
			MessageHandler::logLine("AR set to default: %f", *AR);
			return;
		}
		AR = reinterpret_cast<float*>(g_ARvalueaddress + AR_OFFSET);
		MessageHandler::logDebug("AR found is: %f", *AR);
	}

	void killBloom()
	{
		if (g_bloomstructaddress == nullptr)
		{
			return;
		}
		float* bloom = reinterpret_cast<float*>(g_bloomstructaddress + BLOOM_OFFSET);
		OGBloom = *bloom;
		*bloom = 0.0f;

	}

	void restoreBloom()
	{
		if (g_bloomstructaddress == nullptr)
		{
			return;
		}
		float* bloom = reinterpret_cast<float*>(g_bloomstructaddress + BLOOM_OFFSET);
		*bloom = OGBloom;

	}

	void killDOF() //and bloom
	{

		if (g_dofstructaddress == nullptr)
		{
			return;
		}
		OGfardof = 0.0f;
		OGneardof = 0.0f;

		float* fardof = reinterpret_cast<float*>(g_dofstructaddress + DOF_FAR_AMOUNT);
		float* neardof = reinterpret_cast<float*>(g_dofstructaddress + DOF_NEAR_AMOUNT);
		
		OGfardof = *fardof;
		OGneardof = *neardof;
		
		*fardof = 0.0f;
		*neardof = 0.0f;
		
	}

	void restoreDOF()
	{

		if (g_dofstructaddress == nullptr)
		{
			return;
		}
		float* fardof = reinterpret_cast<float*>(g_dofstructaddress + DOF_FAR_AMOUNT);
		float* neardof = reinterpret_cast<float*>(g_dofstructaddress + DOF_NEAR_AMOUNT);

		*fardof = OGfardof;
		*neardof = OGneardof;
		
	}
	


	XMFLOAT3 initialiseCamera()
	{
		XMFLOAT4X4 _viewMatrix;

		float* matrixInMemory = reinterpret_cast<float*>(g_cameraStructAddress);
		XMMATRIX viewMatrix = XMMATRIX(matrixInMemory);

		
		XMMATRIX transposeMatrix = XMMatrixTranspose(viewMatrix); //transpose so it is in the format expected by DirectX for inversion (row major/row vectors) as the matrix is currently column major, column vectors
		XMMATRIX invertMatrix = XMMatrixInverse(nullptr, transposeMatrix); //inverse matrix to retrieve real camera position to feed towards the construction of our own quaternion
		XMStoreFloat4x4(&_viewMatrix,invertMatrix); //convert to FLOAT 4x4 for easy access
		XMFLOAT3 realPos(_viewMatrix._14, _viewMatrix._24, _viewMatrix._34); //extract coordinates into an XMFLOAT3 to be used to calculate our own matrix

		return realPos;
	}

	float calcvecdot(XMVECTOR vec1, XMVECTOR vec2)
	{
		float dot = ((XMVectorGetX(vec1) * XMVectorGetX(vec2)) + ((XMVectorGetY(vec1) * XMVectorGetY(vec2)) + ((XMVectorGetZ(vec1) * XMVectorGetZ(vec2)))));
		return dot;
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
		XMFLOAT3 Coords2;

		XMMATRIX rotationMatrixPacked = DirectX::XMMatrixRotationQuaternion(newLookQuaternion);
		XMVECTOR newViewCoords = XMLoadFloat3(&newCoords);
		XMStoreFloat4x4(&rotationMatrix, rotationMatrixPacked);


		XMVECTOR xAxis = XMLoadFloat3(&XMFLOAT3(rotationMatrix._11, rotationMatrix._12, rotationMatrix._13));
		XMVECTOR yAxis = XMLoadFloat3(&XMFLOAT3(rotationMatrix._21, rotationMatrix._22, rotationMatrix._23));
		XMVECTOR zAxis = XMLoadFloat3(&XMFLOAT3(rotationMatrix._31, rotationMatrix._32, rotationMatrix._33));

		XMStoreFloat(&Coords2.x, -XMVector3Dot(xAxis, newViewCoords));
		XMStoreFloat(&Coords2.y, -XMVector3Dot(yAxis, newViewCoords));
		XMStoreFloat(&Coords2.z, -XMVector3Dot(zAxis, newViewCoords));
		//XMFLOAT3 Coords(-calcvecdot(xAxis, newViewCoords), -calcvecdot(yAxis, newViewCoords), -calcvecdot(zAxis, newViewCoords));


		float* coordsInMemory = nullptr;
		float* matrixInMemory = nullptr;

		
		matrixInMemory = reinterpret_cast<float*>(g_cameraStructAddress);
		matrixInMemory[0] = rotationMatrix._11;
		matrixInMemory[1] = rotationMatrix._21;
		matrixInMemory[2] = rotationMatrix._31;
		matrixInMemory[3] = 0.0f;
		matrixInMemory[4] = rotationMatrix._12;
		matrixInMemory[5] = rotationMatrix._22;
		matrixInMemory[6] = rotationMatrix._32;
		matrixInMemory[7] = 0.0f;
		matrixInMemory[8] = rotationMatrix._13;
		matrixInMemory[9] = rotationMatrix._23;
		matrixInMemory[10] = rotationMatrix._33;
		matrixInMemory[11] = 0.0f;
	
	coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		coordsInMemory[0] = Coords2.x;
		coordsInMemory[1] = Coords2.y;
		coordsInMemory[2] = Coords2.z;
		coordsInMemory[3] = 1.0f;
	}


	bool isCameraFound()
	{
		return nullptr != g_cameraStructAddress;
	}


	void displayCameraStructAddress()
	{
		MessageHandler::logDebug("Camera struct address: %p", (void*)g_cameraStructAddress);
		MessageHandler::logDebug("AR address: %p", (void*)g_ARvalueaddress);
		MessageHandler::logDebug("DOF address: %p", (void*)g_dofstructaddress);
		MessageHandler::logDebug("Bloom address: %p", (void*)g_bloomstructaddress);
	}


	void restoreGameCameraDataWithCachedData(GameCameraData& source)
	{
		if (!isCameraFound())
		{
			return;
		}
		source.RestoreData(reinterpret_cast<float*>(g_cameraStructAddress), reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET),
						   reinterpret_cast<float*>(g_cameraStructAddress + HFOV_IN_STRUCT_OFFSET), reinterpret_cast<float*>(g_cameraStructAddress + VFOV_IN_STRUCT_OFFSET));
	}


	void cacheGameCameraDataInCache(GameCameraData& destination)
	{
		if (!isCameraFound())
		{
			return;
		}
		destination.CacheData(reinterpret_cast<float*>(g_cameraStructAddress), reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET),
							  reinterpret_cast<float*>(g_cameraStructAddress + HFOV_IN_STRUCT_OFFSET), reinterpret_cast<float*>(g_cameraStructAddress + VFOV_IN_STRUCT_OFFSET));
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