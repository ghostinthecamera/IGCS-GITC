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
#include "stdafx.h"
#include "CameraManipulator.h"
#include "GameConstants.h"
#include "InterceptorHelper.h"
#include "Globals.h"
#include "OverlayConsole.h"
#include "OverlayControl.h"
#include "GameImageHooker.h"
#include "Camera.h"

using namespace DirectX;
using namespace std;

extern "C" {
	LPBYTE g_cameraStructAddress = nullptr;
	LPBYTE g_fovStructAddress = nullptr;
	LPBYTE g_timescaleAddress = nullptr;
}



namespace IGCS::GameSpecific::CameraManipulator
{
	static float _originalCoords[3];
	static float _originalMatrix[12];
	static float _horiginalFoV;
	static float _voriginalFoV;
	const float _basespeed1 = 30.0f;
	const float _basespeed2 = 30.0f;
	const float fovRatio = 1.778;

	void speedUp(int multiplier, bool toggle)
	{
		if (nullptr == g_timescaleAddress)
		{
			return;
		}
		if (toggle == false)
		{
			float* basespeed = reinterpret_cast<float*>(g_timescaleAddress);
			*basespeed = _basespeed1;
			float* basespeed2 = reinterpret_cast<float*>(g_timescaleAddress + 0x04);
			*basespeed2 = _basespeed2;
		} 
		if (toggle == true)
		{
			float* basespeed = reinterpret_cast<float*>(g_timescaleAddress);
			*basespeed = _basespeed1 * multiplier;
			float* basespeed2 = reinterpret_cast<float*>(g_timescaleAddress + 0x04);
			*basespeed2 = _basespeed2 * multiplier;
		}

	}


    void sloMoFunc(float amount, bool timestop)
	{
		if (nullptr == g_timescaleAddress)
		{
			return;
		}
		if (timestop == false)
		{
			float* timescaleInMemory = reinterpret_cast<float*>(g_timescaleAddress + TIMESTOP_OFFSET);
			*timescaleInMemory = *timescaleInMemory > 0.96f ? amount : 1.0f;
		}
	}


	void timeStop()
	{
		if (nullptr == g_timescaleAddress)
		{
			return;
		}

		float* timescaleInMemory = reinterpret_cast<float*>(g_timescaleAddress + TIMESTOP_OFFSET);
		*timescaleInMemory = *timescaleInMemory > 0.04f ? 0.0f : 1.0f;
	}

	void getSettingsFromGameState()
	{
		Settings& currentSettings = Globals::instance().settings();
	}


	void applySettingsToGameState()
	{
		Settings& currentSettings = Globals::instance().settings();
	}


	// Resets the FOV to the one it got when we enabled the camera
	void resetFoV()
	{
		if (g_fovStructAddress == nullptr)
		{
			return;
		}
		float* fovAddress = reinterpret_cast<float*>(g_fovStructAddress + HFOV_IN_STRUCT_OFFSET);
		float* vfovAddress = reinterpret_cast<float*>(g_fovStructAddress + VFOV_IN_STRUCT_OFFSET);
		*fovAddress = _horiginalFoV;
		*vfovAddress = _voriginalFoV;
	}


	// changes the FoV with the specified amount
	void changeFoV(float amount)
	{
		if (g_fovStructAddress == nullptr)
		{
			return;
		}
		float* hfovAddress = reinterpret_cast<float*>(g_fovStructAddress + HFOV_IN_STRUCT_OFFSET);
		float* vfovAddress = reinterpret_cast<float*>(g_fovStructAddress + VFOV_IN_STRUCT_OFFSET);
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

		float vnewValue = hnewValue*fovRatio;

		if (hnewValue < 0.2f)
		{
			// clamp. Game will crash with negative fov
			hnewValue = 0.2f;
		}
		*hfovAddress = hnewValue;
		*vfovAddress = vnewValue;
	}
	
	XMFLOAT3 initialiseCamera()
	{
		float* matrixInMemory = reinterpret_cast<float*>(g_cameraStructAddress);
		XMMATRIX viewMatrix = XMMATRIX(matrixInMemory);
		XMFLOAT4X4 _viewMatrix;
		XMStoreFloat4x4(&_viewMatrix, viewMatrix);

		float realX = -1 * ((_viewMatrix._41 * _viewMatrix._11) + (_viewMatrix._42 * _viewMatrix._21) + (_viewMatrix._43 * _viewMatrix._31));
		float realY = -1 * ((_viewMatrix._41 * _viewMatrix._12) + (_viewMatrix._42 * _viewMatrix._22) + (_viewMatrix._43 * _viewMatrix._32));
		float realZ = -1 * ((_viewMatrix._41 * _viewMatrix._13) + (_viewMatrix._42 * _viewMatrix._23) + (_viewMatrix._43 * _viewMatrix._33));

		XMFLOAT3 realPos(realX, realY, realZ);
		//IGCS::OverlayControl::addNotification(realX);
		return realPos;
	}

	float calcvecdot(XMVECTOR vec1, XMVECTOR vec2)
	{
		float dot = ((XMVectorGetX(vec1)*XMVectorGetX(vec2)) +((XMVectorGetY(vec1)*XMVectorGetY(vec2)) + ((XMVectorGetZ(vec1)*XMVectorGetZ(vec2)))));
		return dot;
	}

	void writeNewCameraValuesToGameData(XMFLOAT3 newCoords, XMVECTOR newLookQuaternion)
	{
		if (!isCameraFound())
		{
			return;
		}
		XMFLOAT4X4 rotationMatrix;

		XMMATRIX rotationMatrixPacked = XMMatrixRotationQuaternion(newLookQuaternion);
		XMVECTOR newViewCoords = XMLoadFloat3(&newCoords);
		XMStoreFloat4x4(&rotationMatrix,rotationMatrixPacked);


		XMVECTOR xAxis = XMLoadFloat3(&XMFLOAT3(rotationMatrix._11, rotationMatrix._12, rotationMatrix._13));
		XMVECTOR yAxis = XMLoadFloat3(&XMFLOAT3(rotationMatrix._21, rotationMatrix._22, rotationMatrix._23));
		XMVECTOR zAxis = XMLoadFloat3(&XMFLOAT3(rotationMatrix._31, rotationMatrix._32, rotationMatrix._33));
		XMFLOAT3 Coords(-calcvecdot(xAxis, newViewCoords), -calcvecdot(yAxis, newViewCoords), -calcvecdot(zAxis, newViewCoords));


		float* coordsInMemory = nullptr;
		float* matrixInMemory = nullptr;

		coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		coordsInMemory[0] = Coords.x;
		coordsInMemory[1] = Coords.y;
		coordsInMemory[2] = Coords.z;
		coordsInMemory[3] = 1.0f;

		matrixInMemory = reinterpret_cast<float*>(g_cameraStructAddress);
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
	}


	bool isCameraFound()
	{
		return nullptr != g_cameraStructAddress;
	}


	void displayCameraStructAddress()
	{
		OverlayConsole::instance().logDebug("Camera struct address: %p", (void*)g_cameraStructAddress);
	}
	

	// should restore the camera values in the camera structures to the cached values. This assures the free camera is always enabled at the original camera location.
	void restoreOriginalValuesAfterCameraDisable()
	{
		float* matrixInMemory = nullptr;
		float* coordsInMemory = nullptr;
		float *hfovInMemory = nullptr;
		float* vfovInMemory = nullptr;

		if (!isCameraFound())
		{
			return;
		}
		matrixInMemory = reinterpret_cast<float*>(g_cameraStructAddress);
		coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		memcpy(matrixInMemory, _originalMatrix, 12 * sizeof(float));
		memcpy(coordsInMemory, _originalCoords, 3 * sizeof(float));

		hfovInMemory = reinterpret_cast<float*>(g_fovStructAddress + HFOV_IN_STRUCT_OFFSET);
		*hfovInMemory = _horiginalFoV;
		vfovInMemory = reinterpret_cast<float*>(g_fovStructAddress + VFOV_IN_STRUCT_OFFSET);
		*vfovInMemory = _voriginalFoV;
	}


	void cacheOriginalValuesBeforeCameraEnable()
	{
		float* matrixInMemory = nullptr;
		float* coordsInMemory = nullptr;
		float* hfovInMemory = nullptr;
		float* vfovInMemory = nullptr;


		if (!isCameraFound())
		{
			return;
		}
		matrixInMemory = reinterpret_cast<float*>(g_cameraStructAddress);
		coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		memcpy(_originalMatrix, matrixInMemory, 12 * sizeof(float));
		memcpy(_originalCoords, coordsInMemory, 3 * sizeof(float));

		hfovInMemory = reinterpret_cast<float*>(g_fovStructAddress + HFOV_IN_STRUCT_OFFSET);
		_horiginalFoV = *hfovInMemory;
		vfovInMemory = reinterpret_cast<float*>(g_fovStructAddress + VFOV_IN_STRUCT_OFFSET);
		_voriginalFoV = *vfovInMemory;
	}

}