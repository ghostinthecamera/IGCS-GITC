////////////////////////////////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System
// Copyright(c) 2020, Frans Bouma
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
#include "SimpleMath.h"
#include "MessageHandler.h"

using namespace DirectX;
using namespace std;
using namespace DirectX::SimpleMath;

extern "C" {
	LPBYTE g_cameraStructAddress = nullptr;
	LPBYTE g_ARvalueaddress = nullptr;
	LPBYTE g_timescaleaddress = nullptr;
	LPBYTE g_bloomstructaddress = nullptr;
}

namespace IGCS::GameSpecific::CameraManipulator
{
	static float cachedGamespeedPause = 1.0f;
	static float cachedGamespeedSlowMo = 1.0f;

	void updateCameraDataInGameData(Camera& camera)
	{
		if (!g_cameraEnabled)
		{
			return;
		}
		XMVECTOR newLookQuaternion = camera.calculateLookQuaternion();
		XMFLOAT3 newCoords, initCoords;
		static XMFLOAT3 currentCoords;

		if (isCameraFound())
		{
			if (_camInit == 1)
			{
				initCoords = initialiseCamera();
				newCoords = camera.calculateNewCoords(initCoords, newLookQuaternion);
				writeNewCameraValuesToGameData(newCoords, newLookQuaternion);
				currentCoords = newCoords;
				_camInit = (uint8_t)0;
				return;
			}
			//currentCoords = initialiseCamera();
			newCoords = camera.calculateNewCoords(currentCoords, newLookQuaternion);
			writeNewCameraValuesToGameData(newCoords, newLookQuaternion);
			currentCoords = newCoords;
		}
	}

	void applySettingsToGameState()
	{
		//Settings& currentSettings = Globals::instance().settings();
		//if (nullptr != g_resolutionScaleAddress)
		//{
		//	float* resolutionScaleInMemory = reinterpret_cast<float*>(g_resolutionScaleAddress + RESOLUTION_SCALE_IN_STRUCT_OFFSET);
		//	*resolutionScaleInMemory = Utils::clamp(currentSettings.resolutionScale, RESOLUTION_SCALE_MIN, RESOLUTION_SCALE_MAX, 1.0f);
		//}
	}

	void displayResolution(int width, int height)
	{
		//MessageHandler::logDebug("Received Resolution: %i x %i", width, height);
	}

	void setResolution(int width, int height)
	{
		//update below specific to game

		//if (nullptr == g_resolutionAddress || nullptr == g_aspectratiobaseAddress)
		//{
		//	return;
		//}

		//int* widthinmem = reinterpret_cast<int*>(g_resolutionAddress + WIDTH1_ADDRESS);
		//int* heightinmem = reinterpret_cast<int*>(g_resolutionAddress + HEIGHT1_ADDRESS);
		//int* widthinmem2 = reinterpret_cast<int*>(g_resolutionAddress + WIDTH2_ADDRESS);
		//int* heightinmem2 = reinterpret_cast<int*>(g_resolutionAddress + HEIGHT2_ADDRESS);
		//float* ARinmemory = reinterpret_cast<float*>(g_aspectratiobaseAddress + AR_OFFSET);

		//float fwidth = static_cast<float>(width);
		//float fheight = static_cast<float>(height);
		//float newAR = fwidth / fheight;

		//*widthinmem = width;
		//*heightinmem = height;
		//*widthinmem2 = width;
		//*heightinmem2 = height;
		//*ARinmemory = newAR;
	}

	void cachetimespeed()
	{
		if (nullptr == g_timescaleaddress)
		{
			return;
		}
		float* gameSpeedInMemory = reinterpret_cast<float*>(g_timescaleaddress + TIMESCALE_OFFSET);
		cachedGamespeedPause = *gameSpeedInMemory;
	}

	void cacheslowmospeed()
	{
		if (nullptr == g_timescaleaddress)
		{
			return;
		}
		float* gameSpeedInMemory = reinterpret_cast<float*>(g_timescaleaddress + TIMESCALE_OFFSET);
		cachedGamespeedSlowMo = *gameSpeedInMemory;
	}

	// This timestop is based on game speed. So if the game has to be paused, we will write a 0.00001f. 
	// 0.0f causes crashes in certain situations
	// If the game has to be unpaused, we'll write a 1.0f.
	void setTimeStopValue(bool pauseGame, bool slowmoEnabled)
	{
		if (nullptr == g_timescaleaddress)
		{
			return;
		}

		float* gameSpeedInMemory = reinterpret_cast<float*>(g_timescaleaddress + TIMESCALE_OFFSET);
	
		*gameSpeedInMemory = pauseGame ? 0.0f : 1.0f;
	}

	void setSlowMo(float amount, bool slowMo, bool gamepaused)
	{
		if (nullptr == g_timescaleaddress)
		{
			return;
		}

		float* gameSpeedInMemory = reinterpret_cast<float*>(g_timescaleaddress + TIMESCALE_OFFSET);

		*gameSpeedInMemory = slowMo ? amount: 1.0f;
	}

	// Resets the FOV to the one it got when we enabled the camera
	void resetFoV(GameCameraData& cachedData)
	{
		if (nullptr == g_cameraStructAddress)
		{
			return;
		}
		float* hfovAddress = reinterpret_cast<float*>(g_cameraStructAddress + HFOV_IN_STRUCT_OFFSET);
		float* vfovAddress = reinterpret_cast<float*>(g_cameraStructAddress + VFOV_IN_STRUCT_OFFSET);
		*hfovAddress = cachedData._hfov;
		*vfovAddress = cachedData._vfov;
	}

	void toggleBloom(bool enabled, GameCameraData cachedData)
	{
		if (g_bloomstructaddress == nullptr)
		{
			return;
		}
		float* bloom = reinterpret_cast<float*>(g_bloomstructaddress + BLOOM_OFFSET);
		*bloom = enabled ? 0.0f : cachedData._bloom;
	}

	// changes the FoV with the specified amount
	void changeFoV(float amount, GameCameraData& cachedData)
	{
		if (nullptr == g_cameraStructAddress)
		{
			return;
		}
		if (g_ARvalueaddress == nullptr)
		{
			MessageHandler::logLine("AR not found - using default AR value of 1.78");
		}
		float* hfovAddress = reinterpret_cast<float*>(g_cameraStructAddress + HFOV_IN_STRUCT_OFFSET);
		float* vfovAddress = reinterpret_cast<float*>(g_cameraStructAddress + VFOV_IN_STRUCT_OFFSET);
		float newValue = *hfovAddress - amount;

		newValue = Utils::rangeClamp(newValue, 0.1f, 40.0f);

		float newVvalue = newValue * cachedData._AspectRatio;

		*hfovAddress = newValue;
		*vfovAddress = newVvalue;
	}

	float getCurrentFoV()
	{
		if (nullptr == g_cameraStructAddress)
		{
			return DEFAULT_FOV;
		}
		float* fovAddress = reinterpret_cast<float*>(g_cameraStructAddress + HFOV_IN_STRUCT_OFFSET);
		return *fovAddress;
	}
	

	XMFLOAT3 getCurrentCameraCoords()
	{

		float* coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		return XMFLOAT3(coordsInMemory[0], coordsInMemory[1], coordsInMemory[2]);
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
		XMFLOAT3 Coords;

		XMMATRIX rotationMatrixPacked = DirectX::XMMatrixRotationQuaternion(newLookQuaternion);
		XMVECTOR newViewCoords = XMLoadFloat3(&newCoords);
		XMStoreFloat4x4(&rotationMatrix, rotationMatrixPacked);

		XMStoreFloat(&Coords.x, -XMVector3Dot(rotationMatrixPacked.r[0], newViewCoords));
		XMStoreFloat(&Coords.y, -XMVector3Dot(rotationMatrixPacked.r[1], newViewCoords));
		XMStoreFloat(&Coords.z, -XMVector3Dot(rotationMatrixPacked.r[2], newViewCoords));

		float* matrixInMemory = reinterpret_cast<float*>(g_cameraStructAddress + MATRIX_IN_STRUCT_OFFSET);
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

		float* coordsInMemory = coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		coordsInMemory[0] = Coords.x;
		coordsInMemory[1] = Coords.y;
		coordsInMemory[2] = Coords.z;
		coordsInMemory[3] = 1.0f;
	}


	bool isCameraFound()
	{
		return nullptr != g_cameraStructAddress;
	}


	void displayAddresses()
	{
		MessageHandler::logDebug("Camera struct address: %p", (void*)g_cameraStructAddress);
		MessageHandler::logDebug("Time struct address: %p", (void*)g_timescaleaddress);
		MessageHandler::logDebug("AR struct address: %p", (void*)g_ARvalueaddress);
		MessageHandler::logDebug("Bloom struct address: %p", (void*)g_bloomstructaddress);
	}


	void restoreGameCameraData(GameCameraData& source)
	{
		if (!isCameraFound())
		{
			return;
		}
		source.RestoreData(reinterpret_cast<float*>(g_cameraStructAddress + MATRIX_IN_STRUCT_OFFSET), reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET), 
						   reinterpret_cast<float*>(g_cameraStructAddress + HFOV_IN_STRUCT_OFFSET), reinterpret_cast<float*>(g_cameraStructAddress + VFOV_IN_STRUCT_OFFSET), 
						   reinterpret_cast<float*>(g_ARvalueaddress + AR_OFFSET), reinterpret_cast<float*>(g_bloomstructaddress + BLOOM_OFFSET));
	}


	void cacheGameCameraData(GameCameraData& destination)
	{
		if (!isCameraFound())
		{
			return;
		}
		destination.CacheData(reinterpret_cast<float*>(g_cameraStructAddress + MATRIX_IN_STRUCT_OFFSET), reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET),
							  reinterpret_cast<float*>(g_cameraStructAddress + HFOV_IN_STRUCT_OFFSET), reinterpret_cast<float*>(g_cameraStructAddress + VFOV_IN_STRUCT_OFFSET),
							  reinterpret_cast<float*>(g_ARvalueaddress + AR_OFFSET), reinterpret_cast<float*>(g_bloomstructaddress + BLOOM_OFFSET));
	}

	void cacheGameAddresses(GameAddressData& destination)
	{
		destination.cameraAddress = g_cameraStructAddress;
		destination.timescaleAddress = g_timescaleaddress;
		destination.ARAddress = g_ARvalueaddress;
		destination.bloomAddress = g_bloomstructaddress;

	}

	//gamespecific
	XMFLOAT3 initialiseCamera()
	{
		XMFLOAT4X4 _viewMatrix;

		float* matrixInMemory = reinterpret_cast<float*>(g_cameraStructAddress);
		XMMATRIX viewMatrix = XMMATRIX(matrixInMemory);

		XMMATRIX transposeMatrix = XMMatrixTranspose(viewMatrix); //transpose so it is in the format expected by DirectX for inversion (row major/row vectors) as the matrix is currently column major, column vectors
		XMMATRIX invertMatrix = XMMatrixInverse(nullptr, transposeMatrix); //inverse matrix to retrieve real camera position to feed towards the construction of our own quaternion
		XMStoreFloat4x4(&_viewMatrix, invertMatrix); //convert to FLOAT 4x4 for easy access
		XMFLOAT3 realPos(_viewMatrix._14, _viewMatrix._24, _viewMatrix._34); //extract coordinates into an XMFLOAT3 to be used to calculate our own matrix

		return realPos;
	}

	/// <summary>
	/// IGCS Connector Code to return the values we need
	/// </summary>
	/// <returns></returns>
	
	void cacheigcsData(Camera& cam, igcsSessionCacheData& igcscache)
	{
		igcscache.eulers.x = cam.getPitch();
		igcscache.eulers.y = cam.getYaw();
		igcscache.eulers.z = cam.getRoll();
		igcscache.Coordinates = getCurrentCameraCoords();
		igcscache.quaternion = cam.getToolsQuaternion();
		igcscache.fov = fovinDegrees(getCurrentFoV());
	}

	void restoreigcsData(Camera& cam, igcsSessionCacheData& igcscache)
	{
		cam.setPitch(igcscache.eulers.x);
		cam.setYaw(igcscache.eulers.y);
		cam.setRoll(igcscache.eulers.z);
		restoreCurrentCameraCoords(igcscache.Coordinates);
		restoreFOV(igcscache.fov);
	}

	void restoreCurrentCameraCoords(XMFLOAT3 coordstorestore)
	{
		float* coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		memcpy(coordsInMemory, &coordstorestore, 3 * sizeof(float));
	}

	void restoreFOV(float fov)
	{
		float* fovAddress = reinterpret_cast<float*>(g_cameraStructAddress + HFOV_IN_STRUCT_OFFSET);
		*fovAddress = fov;
	}

	float fovinDegrees(float fov)
	{
		float k = 180.0f / XM_PI;
		float dfov = fov * k;
		return dfov;
	}

	float fovinRadians(float fov)
	{
		float k = XM_PI / 180.0f;
		float dfov = fov * k;
		return dfov;
	}

	const LPBYTE getCameraStruct()
	{
		return g_cameraStructAddress;
	}

	void setMatrixRotationVectors(Camera& camera)
	{
		Vector3 rV, uV, fV, eulers, coords;
		Vector4 quat;

		XMMATRIX viewMatrix = XMMATRIX(reinterpret_cast<float*>(g_cameraStructAddress));
		//XMMATRIX transposeMatrix = XMMatrixTranspose(viewMatrix);
		XMMATRIX m = XMMatrixInverse(nullptr, viewMatrix);
		DirectX::XMVECTOR q = DirectX::XMQuaternionRotationMatrix(m);
		Matrix sM = (Matrix)m;

		eulers = sM.ToEuler();
		XMStoreFloat3(&rV, m.r[0]);
		XMStoreFloat3(&uV, m.r[1]);
		XMStoreFloat3(&fV, m.r[2]);
		XMStoreFloat3(&coords, m.r[3]);
		XMStoreFloat4(&quat, q);

		camera.setRightVector(rV);
		camera.setUpVector(uV);
		camera.setForwardVector(fV);
		camera.setGameQuaternion(quat);
		camera.setGameEulers(eulers);
		camera.setGameCoords(coords);
	}
}