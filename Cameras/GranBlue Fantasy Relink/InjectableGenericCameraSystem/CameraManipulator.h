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
#pragma once
#include "stdafx.h"
#include "Camera.h"
#include "GameCameraData.h"
#include "CameraToolsData.h"


namespace IGCS::GameSpecific::CameraManipulator
{
	void updateCameraDataInGameData(Camera& camera);
	void writeNewCameraValuesToGameData(DirectX::XMFLOAT3 newCoords, DirectX::XMVECTOR newLookQuaternion);
	XMFLOAT3 getCurrentCameraCoords();
	void resetFoV(GameCameraData& cachedData);
	void changeFoV(float amount);
	float getCurrentFoV();
	bool isCameraFound();
	void displayAddresses();
	void applySettingsToGameState();
	void restoreGameCameraData(GameCameraData& source);
	void cacheGameCameraData(GameCameraData& destination);
	void setTimeStopValue(bool pauseGame, bool slowmoEnabled);
	void setSlowMo(double amount, bool slowMo, bool gamepaused);
	void displayResolution(int width, int height);
	void setResolution(int width, int height);
	void cacheGameAddresses(GameAddressData& destination);
	XMFLOAT3 getCurrentLookAtCoords();
	XMFLOAT3 getCurrentUpVector();
	void setTimescaleAddress(LPBYTE address);
	void cachetimespeed();
	void cacheslowmospeed();


	/// <summary>
	/// igcsconnector functions
	/// </summary>
	/// <returns></returns>
	const LPBYTE getCameraStruct();
	void setMatrixRotationVectors(Camera& camera);
	float fovinDegrees(float fov);
	void cacheigcsData(Camera& cam, igcsSessionCacheData& igcscache);
	void restoreigcsData(Camera& cam, igcsSessionCacheData& igcscache);
	void restoreCurrentCameraCoords(DirectX::XMFLOAT3 coordstorestore);
	void restoreFOV(float fov);
	float fovinRadians(float fov);


}