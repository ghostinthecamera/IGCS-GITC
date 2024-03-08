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
	LPBYTE g_timestopAddress = nullptr;

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
		XMFLOAT3 currentCoords, newCoords;
		if (isCameraFound())
		{
			currentCoords = getCurrentCameraCoords();
			newCoords = camera.calculateNewCoords(currentCoords, newLookQuaternion);
			writeNewCameraValuesToGameData(newCoords, newLookQuaternion);
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
		MessageHandler::logDebug("Received Resolution: %i x %i", width, height);
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

	// This timestop is based on game speed. So if the game has to be paused, we will write a 0.0f. If the game has to be unpaused, we'll write a 1.0f.
	void setTimeStopValue(bool pauseGame, bool slowmoEnabled)
	{
		if (nullptr == g_timestopAddress)
		{
			return;
		}
		float* gameSpeedInMemory = reinterpret_cast<float*>(g_timestopAddress + TIMESTOP_FLOAT_OFFSET);
		if (pauseGame) 
		{ 
			cachedGamespeedPause = *gameSpeedInMemory; 
		}
		if (slowmoEnabled)  //if disabling pause but slowmo enabled, then set slowmo flag to false and set gamepause flag to true
		{
			Globals::instance().sloMo(false);
		}
		*gameSpeedInMemory = pauseGame ? 0.0f : 1.0f;
	}

	void setSlowMo(float amount, bool slowMo, bool gamepaused)
	{
		if (nullptr == g_timestopAddress)
		{
			return;
		}
		float* gameSpeedInMemory = reinterpret_cast<float*>(g_timestopAddress + TIMESTOP_FLOAT_OFFSET);
		if (slowMo) 
		{ 
			cachedGamespeedSlowMo = *gameSpeedInMemory; 
		}
		if (gamepaused && cachedGamespeedSlowMo > 0.95f)
		{
			Globals::instance().gamePaused(false);
		}
		*gameSpeedInMemory = slowMo? amount * 1.0f : cachedGamespeedSlowMo;
	}

	// Resets the FOV to the one it got when we enabled the camera
	void resetFoV(GameCameraData& cachedData)
	{
		if (nullptr == g_cameraStructAddress)
		{
			return;
		}
		float* fovAddress = reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET);
		*fovAddress = cachedData._fov;
	}


	// changes the FoV with the specified amount
	void changeFoV(float amount)
	{
		if (nullptr == g_cameraStructAddress)
		{
			return;
		}
		float* fovAddress = reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET);
		float newValue = *fovAddress + amount;

		newValue = Utils::clamp(newValue, 0.01f, 0.01f);
		*fovAddress = newValue;
	}

	float getCurrentFoV()
	{
		if (nullptr == g_cameraStructAddress)
		{
			return DEFAULT_FOV;
		}
		float* fovAddress = reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET);
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

		XMMATRIX rotationMatrixPacked = DirectX::XMMatrixRotationQuaternion(newLookQuaternion);
		XMStoreFloat4x4(&rotationMatrix, rotationMatrixPacked);

		rotationMatrix._41 = newCoords.x;
		rotationMatrix._42 = newCoords.y;
		rotationMatrix._43 = newCoords.z;

		float* matrixInMemory = reinterpret_cast<float*>(g_cameraStructAddress + MATRIX_IN_STRUCT_OFFSET);
		memcpy(matrixInMemory, &rotationMatrix, 16 * sizeof(float));
	}


	bool isCameraFound()
	{
		return nullptr != g_cameraStructAddress;
	}


	void displayAddresses()
	{
		MessageHandler::logDebug("Camera struct address: %p", (void*)g_cameraStructAddress);
		MessageHandler::logDebug("Time struct address: %p", (void*)g_timestopAddress);
	}


	void restoreGameCameraData(GameCameraData& source)
	{
		if (!isCameraFound())
		{
			return;
		}
		source.RestoreData(reinterpret_cast<float*>(g_cameraStructAddress + MATRIX_IN_STRUCT_OFFSET), reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET), 
						   reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET));
	}


	void cacheGameCameraData(GameCameraData& destination)
	{
		if (!isCameraFound())
		{
			return;
		}
		destination.CacheData(reinterpret_cast<float*>(g_cameraStructAddress + MATRIX_IN_STRUCT_OFFSET), reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET),
							  reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET));
	}

	void cacheGameAddresses(GameAddressData& destination)
	{
		destination.cameraAddress = g_cameraStructAddress;
		destination.timescaleAddress = g_timestopAddress;
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
		igcscache.fov = getCurrentFoV();
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
		float* fovAddress = reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET);
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
		Vector3 rV, uV, fV, eulers;
		Vector4 quat;

		XMMATRIX m = XMMATRIX(reinterpret_cast<float*>(g_cameraStructAddress + MATRIX_IN_STRUCT_OFFSET));
		DirectX::XMVECTOR q = DirectX::XMQuaternionRotationMatrix(m);
		Matrix sM = (Matrix)m;

		eulers = sM.ToEuler();
		XMStoreFloat3(&rV, m.r[0]);
		XMStoreFloat3(&uV, m.r[1]);
		XMStoreFloat3(&fV, m.r[2]);
		XMStoreFloat4(&quat, q);

		camera.setRightVector(rV);
		camera.setUpVector(uV);
		camera.setForwardVector(fV);
		camera.setGameQuaternion(quat);
		camera.setGameEulers(eulers);
	}
}