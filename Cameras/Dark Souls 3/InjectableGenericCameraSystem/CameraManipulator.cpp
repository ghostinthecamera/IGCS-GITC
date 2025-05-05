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
	LPBYTE g_cameraStructAddress = nullptr;
	LPBYTE g_dofstructaddress = nullptr;
	LPBYTE g_timescaleaddress = nullptr;
	LPBYTE g_playerpointer = nullptr;
	LPBYTE g_opacitypointer = nullptr;
}

namespace IGCS::GameSpecific::CameraManipulator
{
	static float cachedGamespeedPause = 0.0167f;
	static float cachedGamespeedSlowMo = 0.0167f;

	void updateCameraDataInGameData()
	{
		if (!g_cameraEnabled)
			return;

		//const XMVECTOR newLookQuaternion = Camera::instance().calculateLookQuaternion();
		const XMVECTOR newLookQuaternion = Camera::instance().getToolsQuaternionVector();
		if (isCameraFound())
		{
			const XMFLOAT3 currentCoords = getCurrentCameraCoords();
			XMFLOAT3 newCoords = Camera::instance().calculateNewCoords(currentCoords, newLookQuaternion);
			writeNewCameraValuesToGameData(newCoords,newLookQuaternion);
		}
	}


	void applySettingsToGameState()
	{
		const auto& s = Globals::instance().settings();

		Utils::setIfChanged(g_hideplayer, s.playerhideToggle);
		Utils::setIfChanged(g_hideNPC, s.npchideToggle);

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
		if (!g_timescaleaddress)
			return;



		const auto gameSpeedInMemory = reinterpret_cast<float*>(g_timescaleaddress + TIMESCALE_OFFSET);
		cachedGamespeedPause = *gameSpeedInMemory;
	}

	void cacheslowmospeed()
	{
		if (!g_timescaleaddress)
			return;

		const auto gameSpeedInMemory = reinterpret_cast<float*>(g_timescaleaddress + TIMESCALE_OFFSET);
		cachedGamespeedSlowMo = *gameSpeedInMemory;
	}

	// This timestop is based on game speed. So if the game has to be paused, we will write a 0.0f (or 0.00001f). 
	// If the game has to be unpaused, we'll write a 1.0f.
	void setTimeStopValue(bool pauseGame)
	{
		if (!g_timescaleaddress)
			return;

		const auto value = Globals::instance().settings().timestopType ? 0.000001f : 0.0f;

		const auto gameSpeedInMemory = reinterpret_cast<float*>(g_timescaleaddress + TIMESCALE_OFFSET);
		*gameSpeedInMemory = pauseGame ? value : cachedGamespeedPause;
	}

	void setSlowMo(float amount, bool slowMo)
	{
		if (!g_timescaleaddress)
			return;

		const auto gameSpeedInMemory = reinterpret_cast<float*>(g_timescaleaddress + TIMESCALE_OFFSET);
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

		const auto coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		return {coordsInMemory[0], coordsInMemory[1], coordsInMemory[2]};
	}

	XMVECTOR getCurrentCameraCoordsVector()
	{

		const auto coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		return XMVECTOR(XMVectorSet(coordsInMemory[0], coordsInMemory[1], coordsInMemory[2], 0.0f));
	}

	XMVECTOR getCurrentPlayerPosition()
	{

		const auto coordsInMemory = reinterpret_cast<float*>(g_playerpointer + PLAYER_POSITION_OFFSET);
		return XMVECTOR(XMVectorSet(coordsInMemory[0], coordsInMemory[1], coordsInMemory[2], 0.0f));
	}

	void setCurrentCameraCoords(XMFLOAT3 coords)
	{
		const auto coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		memcpy(coordsInMemory, &coords, sizeof(XMFLOAT3));
	}

	// newLookQuaternion: newly calculated quaternion of camera view space. Can be used to construct a 4x4 matrix if the game uses a matrix instead of a quaternion
	// newCoords are the new coordinates for the camera in worldspace.
	//void writeNewCameraValuesToGameData(XMFLOAT3 newCoords, XMVECTOR newLookQuaternion)
	//{
	//	if (!isCameraFound())
	//	{
	//		return;
	//	}

	//	float* matrixInMemory = reinterpret_cast<float*>(g_cameraStructAddress + MATRIX_IN_STRUCT_OFFSET);
	//	XMFLOAT4X4 rotationMatrix;

	//	XMMATRIX rotationMatrixPacked = XMMatrixRotationQuaternion(newLookQuaternion);

	//	XMStoreFloat4x4(&rotationMatrix, rotationMatrixPacked);

	//	rotationMatrix._41 = newCoords.x;
	//	rotationMatrix._42 = newCoords.y;
	//	rotationMatrix._43 = newCoords.z;

	//	memcpy(matrixInMemory, &rotationMatrix, 16 * sizeof(float));
	//}

	void writeNewCameraValuesToGameData(XMFLOAT3& newCoords,XMVECTOR newLookQuaternion)
	{
		if (!isCameraFound())
			return;

		// 1) load position, set w=1
		XMVECTOR pos = XMVectorSet(newCoords.x,newCoords.y,newCoords.z,1.0f);

		// 2) build rotation matrix and overwrite its translation row
		XMMATRIX m = XMMatrixRotationQuaternion(newLookQuaternion);
		m.r[3] = pos;

		// 3) write out in a guaranteed row-major layout
		auto pGameMat = reinterpret_cast<XMFLOAT4X4*>(g_cameraStructAddress + MATRIX_IN_STRUCT_OFFSET);
		XMStoreFloat4x4(pGameMat, m);
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
		MessageHandler::logDebug("Time struct address: %p", static_cast<void*>(g_timescaleaddress));
		MessageHandler::logDebug("DOF struct address: %p", static_cast<void*>(g_dofstructaddress));
		MessageHandler::logDebug("Player Pointer: %p", static_cast<void*>(g_playerpointer));
		MessageHandler::logDebug("Player Coordinates: %p", static_cast<void*>(g_playerpointer + PLAYER_POSITION_OFFSET));
		MessageHandler::logDebug("Opacity Pointer: %p", static_cast<void*>(g_opacitypointer));
	}


	void restoreGameCameraData(const GameCameraData& source)
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
		destination.timescaleAddress = g_timescaleaddress;
		destination.dofAddress = g_dofstructaddress;
		destination.playerPointerAddress = g_playerpointer;
		destination.entityOpacityPointer = g_opacitypointer;
		destination.playerPosition = (g_playerpointer + PLAYER_POSITION_OFFSET);
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

		const auto coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		memcpy(coordsInMemory, &coordstorestore, 3 * sizeof(float));
	}

	void restoreFOV(const float fov)
	{
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
		const auto m = static_cast<FXMMATRIX>(reinterpret_cast<float*>(g_cameraStructAddress + MATRIX_IN_STRUCT_OFFSET));
		XMFLOAT3 r, u, f;
		XMFLOAT4 q;
		XMStoreFloat3(&r, m.r[0]);
		XMStoreFloat3(&u, m.r[1]);
		XMStoreFloat3(&f, m.r[2]);
		XMStoreFloat4(&q, XMQuaternionRotationMatrix(m));
		Camera::instance().setRightVector(r);
		Camera::instance().setUpVector(u);
		Camera::instance().setForwardVector(f);
		Camera::instance().setGameQuaternion(q);
		Camera::instance().setGameEulers(RotationMatrixToEulerAngles(m, MULTIPLICATION_ORDER, false, false));
	}

	XMFLOAT3 getEulers()
	{
		const auto m = static_cast<XMMATRIX>(reinterpret_cast<float*>(g_cameraStructAddress + MATRIX_IN_STRUCT_OFFSET));
		return RotationMatrixToEulerAngles(m, MULTIPLICATION_ORDER, false, false);
	}

}