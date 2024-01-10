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
	LPBYTE g_HUDaddress = nullptr;
}



namespace IGCS::GameSpecific::CameraManipulator
{
	static float _originalCoords[3];
	static float _originalMatrix[12];
	static float _horiginalFoV;
	static float _voriginalFoV;
	static float _bullshitNumber;
	static float k;
	static float fovRatio;
	//static float _basespeed1 = 30.0f;
	//static float _basespeed2 = 30.0f;
	//static LPBYTE g_resolutionScaleMenuValueAddress = nullptr;

	void toggleHUD()
	{
		if (nullptr == g_HUDaddress)
		{
			return;
		}

		BYTE* HUDinMemory = reinterpret_cast<BYTE*>(g_HUDaddress + HUD_OFFSET);
		*HUDinMemory = *HUDinMemory == (BYTE)0 ? (BYTE)1 : (BYTE)0;
	}

	void timeStop(bool enabled) 
	{
		if (nullptr == g_timescaleAddress)
		{
			return;
		}

		float* timestopinmemory = reinterpret_cast<float*>(g_timescaleAddress + TIMESTOP_OFFSET);
		*timestopinmemory = (enabled) ? 0.0f :1.0f;
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
		if (g_cameraStructAddress == nullptr)
		{
			return;
		}
		float* hfovAddress = reinterpret_cast<float*>(g_cameraStructAddress + HFOV_FROM_CAM_STRUCT);
		float* vfovAddress = reinterpret_cast<float*>(g_cameraStructAddress + VFOV_FROM_CAM_STRUCT);
		float* bullshitnumberAddress = reinterpret_cast<float*>(g_cameraStructAddress + BULLSHIT_FACTOR_OFFSET);
		*hfovAddress = _horiginalFoV;
		*vfovAddress = _voriginalFoV;
		*bullshitnumberAddress = _bullshitNumber;
	}

	float establishbullshitfactor()
	{
		float* bullshitnumberAddress = reinterpret_cast<float*>(g_cameraStructAddress + BULLSHIT_FACTOR_OFFSET);
		float* hfovAdress = reinterpret_cast<float*>(g_cameraStructAddress + HFOV_FROM_CAM_STRUCT);

		float magicnumber = *bullshitnumberAddress;
		float hfov = *hfovAdress;

		k = magicnumber * hfov;

		return k;	
	}

	void establishfovRatio()
	{
		float* hfovAddress = reinterpret_cast<float*>(g_cameraStructAddress + HFOV_FROM_CAM_STRUCT);
		float* vfovAddress = reinterpret_cast<float*>(g_cameraStructAddress + VFOV_FROM_CAM_STRUCT);

		float hfov = *hfovAddress;
		float vfov = *vfovAddress;

		fovRatio = vfov / hfov;
	}


	// changes the FoV with the specified amount
	void changeFoV(float amount)
	{
		if (g_cameraStructAddress == nullptr)
		{
			return;
		}
		float* hfovAddress = reinterpret_cast<float*>(g_cameraStructAddress + HFOV_FROM_CAM_STRUCT);
		float* vfovAddress = reinterpret_cast<float*>(g_cameraStructAddress + VFOV_FROM_CAM_STRUCT);
		float* bullshitnumberAddress = reinterpret_cast<float*>(g_cameraStructAddress + BULLSHIT_FACTOR_OFFSET);
		float hnewValue = *hfovAddress;

		float multiplier = Utils::clamp(abs(_horiginalFoV / hnewValue ), 0.5f, 1.0f, 1.0f);

	    hnewValue = Utils::clamp((hnewValue - (amount*multiplier)),0.25f,6.0f,6.00f);

		//if (hnewValue < 0.9f)
		//{
		//	hnewValue = *hfovAddress - (amount / 10);
		//}
		//else if (hnewValue > 3.0f)
		//{
		//	hnewValue = *hfovAddress - (amount * 2);
		//}
		//else hnewValue = *hfovAddress - amount;

		float vnewValue = hnewValue * fovRatio;
		float newbullshitnumber = k / hnewValue;

		//if (hnewValue < 0.2f)
		//{
		//	// clamp. Game will crash with negative fov
		//	hnewValue = 0.2f;
		//}
		*hfovAddress = hnewValue;
		*vfovAddress = vnewValue;
		*bullshitnumberAddress = newbullshitnumber;
	}

	XMFLOAT3 currentQuatCoords()
	{
		float* coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + QUATERNION_COORD_OFFSET);
		XMFLOAT3 currentCoords = XMFLOAT3(coordsInMemory[0], coordsInMemory[1], coordsInMemory[2]);
		return currentCoords;
	}

	XMFLOAT3 currentQuatCoordsInverse()
	{
		float* coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + QUATERNION_COORD_OFFSET);
		XMFLOAT3 currentCoords = XMFLOAT3(-coordsInMemory[0], -coordsInMemory[1], -coordsInMemory[2]);
		return currentCoords;
	}

	XMFLOAT3 initialiseCamera()
	{
		//float* matrixInMemory = reinterpret_cast<float*>(g_cameraStructAddress);
		//XMMATRIX viewMatrix = XMMATRIX(matrixInMemory);
		//XMFLOAT4X4 _viewMatrix;
		//XMStoreFloat4x4(&_viewMatrix, viewMatrix);

		//float realX = -1 * ((_viewMatrix._14 * _viewMatrix._11) + (_viewMatrix._24 * _viewMatrix._21) + (_viewMatrix._34 * _viewMatrix._31));
		//float realY = -1 * ((_viewMatrix._14 * _viewMatrix._12) + (_viewMatrix._24 * _viewMatrix._22) + (_viewMatrix._34 * _viewMatrix._32));
		//float realZ = -1 * ((_viewMatrix._14 * _viewMatrix._13) + (_viewMatrix._24 * _viewMatrix._23) + (_viewMatrix._34 * _viewMatrix._33));

		//XMFLOAT3 realPos(realX, realY, realZ);
		////IGCS::OverlayControl::addNotification(realX);
		//return realPos;

		XMFLOAT4X4 _viewMatrix;

		float* matrixInMemory = reinterpret_cast<float*>(g_cameraStructAddress);
		XMMATRIX viewMatrix = XMMATRIX(matrixInMemory);


		//don't need to transpose //XMMATRIX transposeMatrix = XMMatrixTranspose(viewMatrix); //transpose so it is in the format expected by DirectX for inversion (row major/row vectors) as the matrix is currently column major, column vectors
		XMMATRIX invertMatrix = XMMatrixInverse(nullptr, viewMatrix); //inverse matrix to retrieve real camera position to feed towards the construction of our own quaternion
		XMStoreFloat4x4(&_viewMatrix, invertMatrix); //convert to FLOAT 4x4 for easy access
		XMFLOAT3 realPos(_viewMatrix._14, _viewMatrix._24, _viewMatrix._34); //extract coordinates into an XMFLOAT3 to be used to calculate our own matrix

		return realPos;
	}

	float calcvecdot(XMVECTOR vec1, XMVECTOR vec2)
	{
		float dot = ((XMVectorGetX(vec1) * XMVectorGetX(vec2)) + ((XMVectorGetY(vec1) * XMVectorGetY(vec2)) + ((XMVectorGetZ(vec1) * XMVectorGetZ(vec2)))));
		return dot;
	}

	void writeNewCameraValuesToGameData(XMFLOAT3 newCoords, XMVECTOR newLookQuaternion)
	{
		if (!isCameraFound())
		{
			return;
		}
		XMFLOAT4X4 rotationMatrix;
		float* matrixInMemory = nullptr;
		XMFLOAT3 Coords;
		XMVECTOR newViewCoords = XMLoadFloat3(&newCoords);//old code to keep
		
		//XMVECTOR UpPos = XMLoadFloat3(&upposition);
		//XMMATRIX viewMatrix = XMMatrixLookToRH(newViewCoords, newLookQuaternion, XMVECTOR({0.0f,0.0f,0.0f,0.0f}));//new code to make viewmatrix
		//XMStoreFloat4x4(&rotationMatrix, viewMatrix);

		//old code to keep
		XMMATRIX rotationMatrixPacked = XMMatrixRotationQuaternion(newLookQuaternion);
		XMStoreFloat4x4(&rotationMatrix,rotationMatrixPacked);
		
		XMFLOAT3 xAxisXF(rotationMatrix._11, rotationMatrix._12, rotationMatrix._13);
		XMFLOAT3 yAxisXF(rotationMatrix._21, rotationMatrix._22, rotationMatrix._23);
		XMFLOAT3 zAxisXF(rotationMatrix._31, rotationMatrix._32, rotationMatrix._33);

		XMVECTOR xAxis = XMLoadFloat3(&xAxisXF);
		XMVECTOR yAxis = XMLoadFloat3(&yAxisXF);
		XMVECTOR zAxis = XMLoadFloat3(&zAxisXF);

		XMStoreFloat(&Coords.x, -XMVector3Dot(xAxis, newViewCoords));
		XMStoreFloat(&Coords.y, -XMVector3Dot(yAxis, newViewCoords));
		XMStoreFloat(&Coords.z, -XMVector3Dot(zAxis, newViewCoords));

		matrixInMemory = reinterpret_cast<float*>(g_cameraStructAddress);

		matrixInMemory[0] = rotationMatrix._11;
		matrixInMemory[1] = rotationMatrix._12;
		matrixInMemory[2] = rotationMatrix._13;
		matrixInMemory[3] = Coords.x;
		matrixInMemory[4] = rotationMatrix._21;
		matrixInMemory[5] = rotationMatrix._22;
		matrixInMemory[6] = rotationMatrix._23;
		matrixInMemory[7] = Coords.y;
		matrixInMemory[8] = rotationMatrix._31;
		matrixInMemory[9] = rotationMatrix._32;
		matrixInMemory[10] = rotationMatrix._33;
		matrixInMemory[11] = Coords.z;
		
	}

	void writeNewCameraValuesToGameDataQuaternion(XMFLOAT3 newCoords, XMVECTOR newLookQuaternion)
	{
		if (!isCameraFound())
		{
			return;
		}
		float* quaternionInMemory = nullptr;
		float* quaternionCoordsInMemory = nullptr;
		//float* coords1 = nullptr;
		//float* coords2 = nullptr;

		//quaternionInMemory = reinterpret_cast<float*>(g_cameraStructAddress + QUATERNION_OFFSET);
		quaternionCoordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + QUATERNION_COORD_OFFSET);
		//coords1 = reinterpret_cast<float*>(g_cameraStructAddress + COORDS1);
		//coords2 = reinterpret_cast<float*>(g_cameraStructAddress + COORDS2);

		quaternionInMemory[0] = XMVectorGetX(newLookQuaternion);
		quaternionInMemory[1] = XMVectorGetY(newLookQuaternion);
		quaternionInMemory[2] = XMVectorGetZ(newLookQuaternion);
		quaternionInMemory[3] = XMVectorGetW(newLookQuaternion);

		//quaternionCoordsInMemory[0] = newCoords.x;
		//quaternionCoordsInMemory[1] = newCoords.y;
		//quaternionCoordsInMemory[2] = newCoords.z;

		//coords1[0] = newCoords.x;
		//coords1[1] = newCoords.y;
		//coords1[2] = newCoords.z;

		//coords2[0] = newCoords.x;
		//coords2[1] = newCoords.y;
		//coords2[2] = newCoords.z;
	}


	bool isCameraFound()
	{
		return nullptr != g_cameraStructAddress;
	}


	void displayCameraStructAddress()
	{
		OverlayConsole::instance().logDebug("Camera struct address: %p", (void*)g_cameraStructAddress);
		OverlayConsole::instance().logDebug("Timescale struct address: %p", (void*)g_timescaleAddress);
	}
	

	// should restore the camera values in the camera structures to the cached values. This assures the free camera is always enabled at the original camera location.
	void restoreOriginalValuesAfterCameraDisable()
	{
		float* matrixInMemory = nullptr;
		//float* coordsInMemory = nullptr;
		float *hfovInMemory = nullptr;
		float* vfovInMemory = nullptr;
		float* bullshitnumberinMemory = nullptr;


		if (!isCameraFound())
		{
			return;
		}
		// gameplay / cutscene cam
		matrixInMemory = reinterpret_cast<float*>(g_cameraStructAddress);
		//coordsInMemory = reinterpret_cast<float*>(g_cameraStructAddress + COORDS_IN_STRUCT_OFFSET);
		memcpy(matrixInMemory, _originalMatrix, 12 * sizeof(float));
		//memcpy(coordsInMemory, _originalCoords, 3 * sizeof(float));

		hfovInMemory = reinterpret_cast<float*>(g_cameraStructAddress + HFOV_FROM_CAM_STRUCT);
		*hfovInMemory = _horiginalFoV;
		vfovInMemory = reinterpret_cast<float*>(g_cameraStructAddress + VFOV_FROM_CAM_STRUCT);
		*vfovInMemory = _voriginalFoV;
		bullshitnumberinMemory = reinterpret_cast<float*>(g_cameraStructAddress + BULLSHIT_FACTOR_OFFSET);
		*bullshitnumberinMemory = _bullshitNumber;
	}


	void cacheOriginalValuesBeforeCameraEnable()
	{
		float* matrixInMemory = nullptr;
		//float* coordsInMemory = nullptr;
		float* hfovInMemory = nullptr;
		float* vfovInMemory = nullptr;
		float* bullshitnumberinMemory = nullptr;


		if (!isCameraFound())
		{
			return;
		}
		// gameplay/cutscene cam
		matrixInMemory = reinterpret_cast<float*>(g_cameraStructAddress);
		memcpy(_originalMatrix, matrixInMemory, 12 * sizeof(float));

		hfovInMemory = reinterpret_cast<float*>(g_cameraStructAddress + HFOV_FROM_CAM_STRUCT);
		_horiginalFoV = *hfovInMemory;
		vfovInMemory = reinterpret_cast<float*>(g_cameraStructAddress + VFOV_FROM_CAM_STRUCT);
		_voriginalFoV = *vfovInMemory;
		bullshitnumberinMemory = reinterpret_cast<float*>(g_cameraStructAddress + BULLSHIT_FACTOR_OFFSET);
		_bullshitNumber = *bullshitnumberinMemory;
	}

}