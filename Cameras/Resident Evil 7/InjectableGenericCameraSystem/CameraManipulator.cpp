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
#include "GameImageHooker.h"

using namespace DirectX;
using namespace std;

extern "C" {
	uint8_t* g_cameraStructAddress = nullptr;
	uint8_t* g_timescaleAddress = nullptr;
	uint8_t* g_playerStructAddress = nullptr;
	uint8_t* g_vignetteAddress = nullptr;
}

namespace IGCS::GameSpecific::CameraManipulator
{
	static float cachedGamespeedPause = 1.0f;
	static float cachedGamespeedSlowMo = 1.0f;


	void updateCameraDataInGameData()
	{
		if (!g_cameraEnabled)
			return;

		if (isCameraFound())
		{
			static const auto& c = Camera::instance();
			const XMVECTOR newLookQuaternion = c.getToolsQuaternionVector();
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

	void displayResolution(int width, int height)
	{
		MessageHandler::logDebug("Received Resolution: %i x %i", width, height);
	}

	void cachetimespeed()
	{
		if (!g_timescaleAddress)
			return;

		const auto s = reinterpret_cast<float*>(g_timescaleAddress + TIMESCALE_OFFSET);
		cachedGamespeedPause = *s;
	}

	void cacheslowmospeed()
	{
		if (!g_timescaleAddress)
			return;

		const auto s = reinterpret_cast<float*>(g_timescaleAddress + TIMESCALE_OFFSET);
		cachedGamespeedSlowMo = *s;
	}

	float getNearZ()
	{
		if (!g_cameraStructAddress)
			return 0.1f;

		const auto nearZAddress = reinterpret_cast<float*>(g_cameraStructAddress + NEARZ_IN_STRUCT_OFFSET);
		return *nearZAddress;

		//return 0.1f;
	}

	float getFarZ()
	{
		if (!g_cameraStructAddress)
			return 1000.0f;

		const auto farZAddress = reinterpret_cast<float*>(g_cameraStructAddress + FARZ_IN_STRUCT_OFFSET);
		return *farZAddress;
		//return 1000.0f;
	}

	void setTimeStopValue(bool pauseGame)
	{
		if (!g_timescaleAddress)
			return;
		const auto s = reinterpret_cast<float*>(g_timescaleAddress + TIMESCALE_OFFSET);
		*s = pauseGame ? 0.0f : cachedGamespeedPause;
	}

	void setSlowMo(float amount, bool slowMo)
	{
		if (!g_timescaleAddress)
			return;

		const auto s = reinterpret_cast<float*>(g_timescaleAddress + TIMESCALE_OFFSET);
		*s = slowMo ? amount * cachedGamespeedPause : cachedGamespeedPause;
	}

	// Resets the FOV to the one it got when we enabled the camera
	void resetFoV(const GameCameraData& cachedData)
	{
		if (!g_cameraStructAddress)
			return;

		const auto fovAddress = reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET);
		*fovAddress = cachedData._fov;
	}


	// writes the FoV with the specified value
	void changeFoV(const float fovtowrite)
	{
		if (!g_cameraStructAddress || !g_cameraEnabled)
			return;

		const auto fovAddress = reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET);
		*fovAddress = XMConvertToDegrees(fovtowrite);
	}

	float getCurrentFoV()
	{
		if (!g_cameraStructAddress)
			return DEFAULT_FOV;

		const auto fovAddress = reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET);
		return XMConvertToRadians(*fovAddress);
	}

	float getCurrentVFoV()
	{
		if (!g_cameraStructAddress)
			return DEFAULT_FOV;

		float aspectRatio;

		if (!Globals::instance().settings().d3ddisabled)
		{
			aspectRatio = (D3DMODE == D3DMODE::DX11) ? D3DHook::instance().getAspectRatio() : D3D12Hook::instance().getAspectRatio();
		}
		else
		{
			aspectRatio = ASPECT_RATIO;
		}

		return 2.0f * atan(tan(getCurrentFoV() / 2.0f) / aspectRatio);
	}
	

	XMFLOAT3 getCurrentCameraCoords()
	{
		if (!g_cameraStructAddress){
			return { 0.0f, 0.0f, 0.0f }; // Return a default value if the address is not valid
		}

		const auto coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		return { coordsInMemory[0], coordsInMemory[1], coordsInMemory[2] };

	}

	XMVECTOR getCurrentCameraCoordsVector()
	{
		if (!g_cameraStructAddress)
			return XMVECTOR(DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f)); // Return a default vector if the address is not valid

		const auto coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		return XMVECTOR(XMVectorSet(coordsInMemory[0], coordsInMemory[1], coordsInMemory[2], 0.0f));
	}

	XMVECTOR getCurrentPlayerPosition()
	{
		if (!g_playerStructAddress)
			return XMVECTOR(XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f)); // Return a default vector if the address is not valid

		const auto coordsInMemory = reinterpret_cast<float*>(g_playerStructAddress + PLAYER_POSITION_OFFSET);
		return XMVECTOR(XMVectorSet(coordsInMemory[0], coordsInMemory[1], coordsInMemory[2], 0.0f));

		//return XMVECTOR(XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f)); // Default position if not found
	}

	XMVECTOR getCurrentPlayerRotation()
	{
		if (!g_playerStructAddress)
			return XMVECTOR(XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f)); // Return a default vector if the address is not valid

		const auto r = reinterpret_cast<float*>(g_playerStructAddress + PLAYER_ROTATION_OFFSET);
		return XMVECTOR(XMVectorSet(r[0], r[1], r[2], r[3]));

		//return XMVECTOR(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f)); // Default rotation if not found
	}

	void setCurrentCameraCoords(XMFLOAT3 coords)
	{
		if (!g_cameraStructAddress)
			return;

		const auto coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		memcpy(coordsInMemory, &coords, sizeof(XMFLOAT3));
	}


	void writeNewCameraValuesToGameData(XMFLOAT3& newCoords,XMVECTOR newLookQuaternion)
	{
		if (!isCameraFound())
			return;

		static bool handednessChecked = false;
		if (!handednessChecked)
		{
			Utils::determineHandedness();
			handednessChecked = true;
		}

		const auto c = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		const auto q = reinterpret_cast<XMFLOAT4*>(g_cameraStructAddress + QUATERNION_IN_STRUCT_OFFSET);
		const auto m = reinterpret_cast<XMMATRIX*>(g_cameraStructAddress + MATRIX_IN_STRUCT_OFFSET);

		// Write coordinates directly as floats
		c[0] = newCoords.x;
		c[1] = newCoords.y;
		c[2] = newCoords.z;

		XMStoreFloat4(q, newLookQuaternion);

		auto mInt = XMMatrixRotationQuaternion(newLookQuaternion);
		mInt.r[3].m128_f32[0] = newCoords.x;
		mInt.r[3].m128_f32[1] = newCoords.y;
		mInt.r[3].m128_f32[2] = newCoords.z;
		mInt.r[3].m128_f32[3] = 1.0f;

		*m = mInt;
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
		MessageHandler::logDebug("Timescale Address: %p", static_cast<void*>(g_timescaleAddress));
		MessageHandler::logDebug("Vignette address: %p", static_cast<void*>(g_vignetteAddress));
		MessageHandler::logDebug("player address: %p", static_cast<void*>(g_playerStructAddress));
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
		destination.timescaleAddress = g_timescaleAddress;
		destination.vignetteAddress = g_vignetteAddress;
		destination.playerAddress = g_playerStructAddress;
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

	// check units
	void restoreFOV(const float fov)
	{
		if (g_cameraStructAddress)
		{
			return;
		}

		const auto fovAddress = reinterpret_cast<float*>(g_cameraStructAddress + FOV_IN_STRUCT_OFFSET);
		*fovAddress = fov;
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


	/// <summary>
	/// Game specific code
	/// </summary>

	void toggleVignette(bool enable)
	{
		if (!g_vignetteAddress)
		{
			return;
		}

		const auto v = g_vignetteAddress + VIGNETTE_OFFSET;
		enable ? *v = static_cast<uint8_t>(2) : *v = static_cast<uint8_t>(0);
		const auto state = *v;
		MessageHandler::logDebug("Vignette state set to %i", state);
	}

	void toggleFlashlight(bool enable)
	{
		if (!g_playerStructAddress)
		{
			return;
		}

		g_disableFlashlight = enable ? static_cast<uint8_t>(1) : static_cast<uint8_t>(0);
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

	bool validatePlayerPositionMemory() {
		//if (!g_playerStructAddress) {
		//	return false;
		//}
		//__try {
		//	// Try to read critical camera data to validate accessibility
		//	// Test reading position data
		//	const auto pos = reinterpret_cast<float*>(g_playerStructAddress + CAR_POSITION_IN_VEHICLE_OFFSET);
		//	volatile float testRead = *pos;
		//	return true;
		//}
		//__except (EXCEPTION_EXECUTE_HANDLER) {
		//	// Memory became inaccessible
		//	g_playerStructAddress = nullptr;
		//	return false;
		//}
		return true; // Default return value if not implemented
	}


	bool validateCameraMemory() {
		//if (!g_cameraStructAddress) {
		//	return false;
		//}

		//__try {
		//	// Try to read critical camera data to validate accessibility

		//	// Test reading quaternion data
		//	const auto q = reinterpret_cast<float*>(g_cameraStructAddress + QUATERNION_IN_STRUCT_OFFSET);
		//	volatile float testRead = *q;

		//	return true;
		//}
		//__except (EXCEPTION_EXECUTE_HANDLER) {
		//	// Memory became inaccessible
		//	g_cameraStructAddress = nullptr;
		//	return false;
		//}
		return false; // Default return value if not implemented
	}

	bool validateReplayTimescaleMemory() {
		//if (!g_timescaleAddress) {
		//	return false;
		//}
		//__try {
		//	// Try to read critical timescale data to validate accessibility
		//	const auto replayTimescale = reinterpret_cast<float*>(g_timescaleAddress + REPLAY_TIMESCALE_OFFSET);
		//	volatile float testRead = *replayTimescale;
		//	return true;
		//}
		//__except (EXCEPTION_EXECUTE_HANDLER) {
		//	// Memory became inaccessible
		//	g_timescaleAddress = nullptr;
		//	return false;
		//}
		return false; // Default return value if not implemented
	}

}
