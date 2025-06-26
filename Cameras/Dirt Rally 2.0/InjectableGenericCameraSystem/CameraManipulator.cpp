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
#include "MessageHandler.h"
#include "Console.h"

using namespace DirectX;
using namespace std;

extern "C" {
	uint8_t* g_cameraStructAddress = nullptr;
	uint8_t* g_cameraQuaternionAddress = nullptr;
	uint8_t* g_cameraPositionAddress = nullptr;
	uint8_t* g_carPositionAddress = nullptr;
	uint8_t* g_timescaleAddress = nullptr;
	uint8_t* g_dofStrengthAddress = nullptr;
}

namespace IGCS::GameSpecific::CameraManipulator
{
	static float cachedGamespeedPause = 1.0f;
	static float cachedGamespeedSlowMo = 1.0f;

	bool validatePLayerPositionMemory() {
		if (!g_cameraPositionAddress) {
			return false;
		}
		__try {
			// Try to read critical camera data to validate accessibility
			volatile float testRead;
			// Test reading position data
			const auto pos = reinterpret_cast<float*>(g_carPositionAddress + PLAYER_ROTATION_IN_STRUCT_OFFSET);
			testRead = *pos;
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			// Memory became inaccessible
			g_cameraPositionAddress = nullptr;
			return false;
		}
	}


	bool validateCameraMemory() {
		if (!g_cameraStructAddress) {
			return false;
		}

		__try {
			// Try to read critical camera data to validate accessibility
			volatile float testRead;

			// Test reading quaternion data
			const auto q = reinterpret_cast<float*>(g_cameraStructAddress + QUATERNION_IN_STRUCT_OFFSET);
			testRead = *q;

			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			// Memory became inaccessible
			g_cameraStructAddress = nullptr;
			return false;
		}
	}

	void updateCameraDataInGameData()
	{
		if (!g_cameraEnabled)
			return;

		if (!System::instance().isCameraStructValid)
			return;

		static const auto& c = Camera::instance();

		const XMVECTOR newLookQuaternion = c.getToolsQuaternionVector();
		if (isCameraFound())
		{
			const XMFLOAT3 currentCoords = c.getInternalPosition();

			// Use height-locked calculation for horizontal movements when enabled
			XMFLOAT3 newCoords;
			if (Camera::instance().getHeightLockedMovement())
			{
				newCoords = Camera::instance().calculateNewCoordsHeightLocked(currentCoords, newLookQuaternion, true);
			}
			else
			{
				newCoords = Camera::instance().calculateNewCoords(currentCoords, newLookQuaternion);
			}

			writeNewCameraValuesToGameData(newCoords, newLookQuaternion);
		}
	}

	uint8_t* getCameraStructAddress() {
		return g_cameraStructAddress;
	}

	void applySettingsToGameState()
	{
		const auto& s = Globals::instance().settings();

		
	}

	void setDOF(bool enabled)
	{
		if (!g_dofStrengthAddress)
			return;

		const auto dofStrengthInMemory = reinterpret_cast<float*>(g_dofStrengthAddress + DOF_STRENGTH_OFFSET);
		*dofStrengthInMemory = enabled ? 0.0f : 1.0f;
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

	void cachetimespeed()
	{
		//could do this check in system.cpp but this game is unique so we will do it here anyway
		if (!g_timescaleAddress)
			return;



		const auto gameSpeedInMemory = reinterpret_cast<float*>(g_timescaleAddress + TIMESCALE_OFFSET);
		cachedGamespeedPause = *gameSpeedInMemory;
	}

	void cacheslowmospeed()
	{
		if (!g_timescaleAddress)
			return;

		const auto gameSpeedInMemory = reinterpret_cast<float*>(g_timescaleAddress + TIMESCALE_OFFSET);
		cachedGamespeedSlowMo = *gameSpeedInMemory;
	}

	float getNearZ()
	{
		if (!g_cameraQuaternionAddress)
			return 0.1f;

		if (!System::instance().isCameraStructValid)
			return 0.1f;

		const auto nearZAddress = reinterpret_cast<float*>(g_cameraQuaternionAddress + NEAR_Z_FROM_QUATERNION_OFFSET);
		return *nearZAddress;

		//return 0.1f;
	}

	float getMotionBlur()
	{
		if (!g_cameraStructAddress)
			return 0.0f;
		if (!System::instance().isCameraStructValid)
			return 0.0f;
		const auto motionBlurAddress = reinterpret_cast<float*>(g_cameraQuaternionAddress + MOTION_BLUR_STRENGTH_FROM_CAMQUATERNION_OFFSET);
		return *motionBlurAddress;
	}

	void setMotionBlur(float amount)
	{
		if (!g_cameraStructAddress)
			return;
		if (!System::instance().isCameraStructValid)
			return;

		const auto motionBlurAddress = reinterpret_cast<float*>(g_cameraQuaternionAddress + MOTION_BLUR_STRENGTH_FROM_CAMQUATERNION_OFFSET);
		*motionBlurAddress = amount;
	}

	float getFarZ()
	{
		//if (!g_activecamAddress)
		//	return 0.0f;

		//const auto farZAddress = reinterpret_cast<float*>(g_activecamAddress + ACTIVECAM_FARZ);
		//return *farZAddress;
		return 1000.0f;
	}

	// This timestop is based on game speed. So if the game has to be paused, we will write a 0.0f (or 0.00001f). 
	// If the game has to be unpaused, we'll write a 1.0f.
	void setTimeStopValue(bool pauseGame)
	{
		if (!g_timescaleAddress)
			return;

		const auto gameSpeedInMemory = reinterpret_cast<float*>(g_timescaleAddress + TIMESCALE_OFFSET);
		*gameSpeedInMemory = pauseGame ? 0.0f : cachedGamespeedPause;
	}

	void setSlowMo(float amount, bool slowMo)
	{
		if (!g_timescaleAddress)
			return;

		const auto gameSpeedInMemory = reinterpret_cast<float*>(g_timescaleAddress + TIMESCALE_OFFSET);
		*gameSpeedInMemory = slowMo ? amount * cachedGamespeedPause : cachedGamespeedPause;
	}

	// Resets the FOV to the one it got when we enabled the camera
	void resetFoV(const GameCameraData& cachedData)
	{
		if (!g_cameraStructAddress)
			return;

		const auto fovAddress = reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET);
		*fovAddress = cachedData._fov;
	}


	// changes the FoV with the specified amount
	void changeFoV(const float fovtowrite)
	{
		if (!g_cameraStructAddress || !g_cameraEnabled)
			return;

		const auto fovAddress = reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET);
		*fovAddress = fovtowrite;
	}

	float getCurrentFoV()
	{
		if (!g_cameraStructAddress)
			return DEFAULT_FOV;

		const auto fovAddress = reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET);
		return *fovAddress;
	}
	

	XMFLOAT3 getCurrentCameraCoords()
	{
		if (!g_cameraStructAddress){
			return { 0.0f, 0.0f, 0.0f }; // Return a default value if the address is not valid
		}

		if (!System::instance().isCameraStructValid)
			return { 0.0f, 0.0f, 0.0f };

		const auto coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		return { coordsInMemory[0], coordsInMemory[1], coordsInMemory[2] };

	}

	XMVECTOR getCurrentCameraCoordsVector()
	{
		if (!g_cameraStructAddress)
			return XMVECTOR(XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f)); // Return a default vector if the address is not valid

		if (!System::instance().isPlayerStructValid)
			return { 0.0f, 0.0f, 0.0f };

		const auto coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		return XMVECTOR(XMVectorSet(coordsInMemory[0], coordsInMemory[1], coordsInMemory[2], 0.0f));
	}

	XMVECTOR getCurrentPlayerPosition()
	{
		if (!g_carPositionAddress)
			return XMVECTOR(XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f)); // Return a default vector if the address is not valid

		if (!System::instance().isPlayerStructValid)
			return XMVECTOR(XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f));

		const auto coordsInMemory = reinterpret_cast<float*>(g_carPositionAddress + PLAYER_POSITION_IN_STRUCT_OFFSET);
		return XMVECTOR(XMVectorSet(coordsInMemory[0], coordsInMemory[1], coordsInMemory[2], 0.0f));
	}

	XMVECTOR getCurrentPlayerRotation()
	{
		if (!g_carPositionAddress)
			return XMVECTOR(XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f)); // Return a default vector if the address is not valid

		if (!System::instance().isPlayerStructValid)
			return XMVECTOR(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));

		const auto r = reinterpret_cast<float*>(g_carPositionAddress + PLAYER_ROTATION_IN_STRUCT_OFFSET);
		return XMVECTOR(XMVectorSet(r[0], r[1], r[2], r[3]));
	}

	void setCurrentCameraCoords(XMFLOAT3 coords)
	{
		if (!g_cameraStructAddress)
			return;

		if (!System::instance().isCameraStructValid)
			return;

		const auto coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		memcpy(coordsInMemory, &coords, sizeof(XMFLOAT3));
	}


	void writeNewCameraValuesToGameData(XMFLOAT3& newCoords,XMVECTOR newLookQuaternion)
	{
		if (!isCameraFound())
			return;

		if (!System::instance().isCameraStructValid)
			return;

		auto c = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		auto q = reinterpret_cast<XMFLOAT4*>(g_cameraStructAddress + QUATERNION_IN_STRUCT_OFFSET);

		// 1) load position, set w=1
		XMVECTOR pos = XMVectorSet(newCoords.x,newCoords.y,newCoords.z,1.0f);

		memcpy(c, &pos, 3 * sizeof(float));
		XMStoreFloat4(q, newLookQuaternion);
		//memcpy(q, &newLookQuaternion, 4 * sizeof(float));


	}

	// Version that takes XMVECTOR
	void writeNewCameraValuesToGameData(const XMVECTOR newCoords, const XMVECTOR newLookQuaternion)
	{
		// Convert XMVECTOR to XMFLOAT3 and call the other overload
		XMFLOAT3 coordsFloat3;
		XMStoreFloat3(&coordsFloat3, newCoords);
		writeNewCameraValuesToGameData(coordsFloat3, newLookQuaternion);
	}

	bool isCameraFound()
	{
		return nullptr != g_cameraStructAddress;
	}


	void displayAddresses()
	{
		MessageHandler::logDebug("Camera struct address: %p", static_cast<void*>(g_cameraStructAddress));
		MessageHandler::logDebug("Camera quaternion address: %p", static_cast<void*>(g_cameraQuaternionAddress));
		MessageHandler::logDebug("Camera position address: %p", static_cast<void*>(g_cameraPositionAddress));
		MessageHandler::logDebug("Car position address: %p", static_cast<void*>(g_carPositionAddress));
		MessageHandler::logDebug("DOF strength address: %p", static_cast<void*>(g_dofStrengthAddress));
		MessageHandler::logDebug("Timescale address: %p", static_cast<void*>(g_timescaleAddress));
	}


	void restoreGameCameraData(const GameCameraData& source)
	{
		if (!isCameraFound())
		{
			return;
		}
		source.RestoreData(reinterpret_cast<float*>(g_cameraStructAddress + QUATERNION_IN_STRUCT_OFFSET), reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET),
						   reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET));
	}


	void cacheGameCameraData(GameCameraData& destination)
	{
		if (!isCameraFound())
		{
			return;
		}
		destination.CacheData(reinterpret_cast<float*>(g_cameraStructAddress + QUATERNION_IN_STRUCT_OFFSET), reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET),
							  reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET));
	}

	void cacheGameAddresses(GameAddressData& destination)
	{
		destination.cameraAddress = g_cameraStructAddress;
		destination.camQuaternionAddress = g_cameraQuaternionAddress;
		destination.camPositionAddress = g_cameraPositionAddress;
		destination.carPositionAddress = g_carPositionAddress;
		destination.dofStrengthAddress = g_dofStrengthAddress;
		destination.timescaleAddress = g_timescaleAddress;
	}

	/// <summary>
	/// IGCS Connector Code to return the values we need
	/// </summary>
	/// <returns></returns>
	
	void cacheigcsData(IGCSSessionCacheData& igcscache)
	{
		igcscache.eulers = Camera::instance().getRotation();
		igcscache.Coordinates = getCurrentCameraCoords();
		igcscache.quaternion = Camera::instance().getToolsQuaternion();
		igcscache.fov = getCurrentFoV();
	}

	void restoreigcsData(const IGCSSessionCacheData& igcscache)
	{
		Camera::instance().setRotation(igcscache.eulers);

		restoreCurrentCameraCoords(igcscache.Coordinates);
		restoreFOV(igcscache.fov);
	}

	void restoreCurrentCameraCoords(const XMFLOAT3 coordstorestore)
	{
		if (!g_cameraStructAddress)
		{
			return;
		}

		if (!System::instance().isCameraStructValid)
			return;

		const auto coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		memcpy(coordsInMemory, &coordstorestore, 3 * sizeof(float));
	}

	void restoreFOV(const float fov)
	{
		if (g_cameraStructAddress)
		{
			return;
		}

		const auto fovAddress = reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET);
		*fovAddress = fov;
	}

	float fovinDegrees(const float fov)
	{
		const float dfov = fov * (180.0f / XM_PI);
		return dfov;
	}

	float fovinRadians(const float fov)
	{
		const float dfov = fov * DEG_TO_RAD_FACTOR;
		return dfov;
	}

	LPBYTE getCameraStruct()
	{
		return g_cameraStructAddress;
	}

	void setMatrixRotationVectors()
	{
		if (!g_cameraStructAddress) return;
		if (!System::instance().isCameraStructValid) return;

		const auto q = reinterpret_cast<XMFLOAT4*>(g_cameraStructAddress + QUATERNION_IN_STRUCT_OFFSET);
		const XMVECTOR qv = XMLoadFloat4(q);
		const auto m = XMMatrixRotationQuaternion(qv);

		XMFLOAT3 r, u, f;
		XMStoreFloat3(&r, m.r[0]);
		XMStoreFloat3(&u, m.r[1]);
		XMStoreFloat3(&f, m.r[2]);

		Camera::instance().setRightVector(r);
		Camera::instance().setUpVector(u);
		Camera::instance().setForwardVector(f);
		Camera::instance().setGameQuaternion(*q);
		Camera::instance().setGameEulers(Utils::QuaternionToEulerAngles(qv, MULTIPLICATION_ORDER));
	}

	XMFLOAT3 getEulers()
	{
		if (!System::instance().isCameraStructValid)
			return {0.0f,0.0f,0.0f};

		const auto q = static_cast<XMFLOAT4>(reinterpret_cast<float*>(g_cameraStructAddress + QUATERNION_IN_STRUCT_OFFSET));
		const XMVECTOR qv = XMLoadFloat4(&q);
		return Utils::QuaternionToEulerAngles(qv, MULTIPLICATION_ORDER);
	}

}