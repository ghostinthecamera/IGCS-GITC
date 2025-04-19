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
#include "InterceptorHelper.h"
#include "GameConstants.h"
#include "GameImageHooker.h"
#include <map>
#include "OverlayConsole.h"
#include "CameraManipulator.h"
#include "AOBBlock.h"

using namespace std;

//--------------------------------------------------------------------------------------------------------------------------------
// external asm functions
extern "C" {
	void cameraStructInterceptor();
	void timescaleInterceptor();
	void HUDinterceptor();
	void cameraStruct();
	void cameraWrite();
	void activeCameraInterceptor();
	//void fogReadInterceptor();*/
}

// external addresses used in asm.
extern "C" {
	LPBYTE _cameraStructInterceptionContinue = nullptr;
	LPBYTE _timescaleInterceptionContinue = nullptr;
	LPBYTE _HUDinterceptionContinue = nullptr;
	LPBYTE  _cameraStructContinue = nullptr;
	LPBYTE _cameraWriteContinue = nullptr;
	LPBYTE _activeCameraContinue = nullptr;
}


namespace IGCS::GameSpecific::InterceptorHelper
{
	void initializeAOBBlocks(LPBYTE hostImageAddress, DWORD hostImageSize, map<string, AOBBlock*> &aobBlocks)
	{
		aobBlocks[ACTIVE_ADDRESS_INTERCEPT_KEY] = new AOBBlock(ACTIVE_ADDRESS_INTERCEPT_KEY, "48 8B F9 48 8B 71 08 F3 0F 10 BE 8C 02 00 00", 1);
		aobBlocks[HUD_RENDER_INTERCEPT_KEY] = new AOBBlock(HUD_RENDER_INTERCEPT_KEY, "80 BA ?? ?? ?? ?? ?? 48 89 4C 24 ?? 89 44 24 ?? 74 4E", 1);
		aobBlocks[TIMESCALE_INTERCEPT_KEY] = new AOBBlock(TIMESCALE_INTERCEPT_KEY, "48 8B 05 7B 11 8B 00 | F3 0F 10 88 18 7C 35 00 F3 0F 10 90 1C 7C 35 00 0F B7 80 28 7C 35 00 F3 0F 5C D1", 1);
		aobBlocks[FOV_BYTEWRITE1] = new AOBBlock(FOV_BYTEWRITE1, "C6 83 AC 02 00 00 00 0F 28 83 60 01 00 00 0F 29 87 80 00 00 00 0F 28 8B 70 01 00 00", 1);	// 7 bytes
		aobBlocks[FOV_BYTEWRITE2] = new AOBBlock(FOV_BYTEWRITE2, "45 88 AE AC 02 00 00 41 0F 28 86 60 01 00 00", 1); // 7 bytes
		aobBlocks[FOV_BYTEWRITE3] = new AOBBlock(FOV_BYTEWRITE3, "C6 83 AC 02 00 00 00 48 83 C4 20", 1); // 7 bytes
		aobBlocks[FOV_BYTEWRITE4] = new AOBBlock(FOV_BYTEWRITE4, "C6 83 AC 02 00 00 00 0F 28 83 60 01 00 00 48 8D 54 24 30 0F 28 8B 70 01 00 00", 1); // 7 bytes
		aobBlocks[CAMERA_ADDRESS_KEY2] = new AOBBlock(CAMERA_ADDRESS_KEY2, "8B C0 4C 8D 04 40 49 C1 E0 05", 1); // 4 bytes
		aobBlocks[CAMERA_WRITE] = new AOBBlock(CAMERA_WRITE, "0F 29 00 41 0F 28 48 10 0F 29 48 10 41", 1); // 4 bytes
		
		aobBlocks[FOV_INTERCEPT_KEY] = new AOBBlock(FOV_INTERCEPT_KEY, "F3 0F 11 87 ?? ?? ?? ?? C6 87 ?? ?? ?? ?? 01 48 8B BC 24 ?? ?? ?? ?? 48 8B 4D ?? 48 33 CC E8 ?? ?? ?? ??", 1);
		//aobBlocks[ABS1_TIMESCALE_INTERCEPT_KEY] = new AOBBlock(ABS1_TIMESCALE_INTERCEPT_KEY, "F3 0F 11 35 | ?? ?? ?? ?? F3 0F 11 3D ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 1D", 1);
		//aobBlocks[ABS2_TIMESCALE_INTERCEPT_KEY] = new AOBBlock(ABS2_TIMESCALE_INTERCEPT_KEY, "F3 0F 11 35 ?? ?? ?? ?? F3 0F 11 3D | ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 1D", 1);
		aobBlocks[QUATERNION_WRITE] = new AOBBlock(QUATERNION_WRITE, "66 0F 7F 4B ?? 48 8B 93 ?? ?? ?? ?? 48 85 D2", 1);
		aobBlocks[QUATERNION_COORD_WRITE] = new AOBBlock(QUATERNION_COORD_WRITE, "00 75 ?? 0F 2E 7E ?? 75 ?? 0F 2E 76 ?? 74 ?? | F3 44 0F 11 46 ??", 1);
		aobBlocks[QUATERNION_CUTSCENE_COORD_WRITE] = new AOBBlock(QUATERNION_CUTSCENE_COORD_WRITE, "F3 44 0F 11 57 ?? F3 44 0F 11 47 ?? F3 44 0F 11 4F ?? 48 8B 97", 1);
		//aobBlocks[STRUCTABS_ADDRESS] = new AOBBlock(STRUCTABS_ADDRESS, "F3 0F 10 05 | EC C6 A4 00", 1);


		map<string, AOBBlock*>::iterator it;
		bool result = true;
		for (it = aobBlocks.begin(); it != aobBlocks.end(); it++)
		{
			result &= it->second->scan(hostImageAddress, hostImageSize);
		}
		if (result)
		{
			OverlayConsole::instance().logLine("All interception offsets found.");
		}
		else
		{
			OverlayConsole::instance().logError("One or more interception offsets weren't found: tools aren't compatible with this game's version.");
		}
	}

	void setCameraStructInterceptorHook(map<string, AOBBlock*> &aobBlocks)
	{
		//GameImageHooker::setHook(aobBlocks[CAMERA_ADDRESS_KEY2], 0x0E, &_cameraStructContinue, &cameraStruct);
		GameImageHooker::setHook(aobBlocks[ACTIVE_ADDRESS_INTERCEPT_KEY], 0x0F, &_activeCameraContinue, &activeCameraInterceptor);
	}


	void setPostCameraStructHooks(map<string, AOBBlock*> &aobBlocks)
	{
		//GameImageHooker::setHook(aobBlocks[CAMERA_WRITE], 0x15, &_cameraWriteContinue, &cameraWrite);
		GameImageHooker::setHook(aobBlocks[TIMESCALE_INTERCEPT_KEY], 0x10, &_timescaleInterceptionContinue, &timescaleInterceptor);
		GameImageHooker::setHook(aobBlocks[HUD_RENDER_INTERCEPT_KEY], 0x10, &_HUDinterceptionContinue, &HUDinterceptor);
	}

	void SaveNOPReplace(AOBBlock* hookData, int numberOfBytes, bool enabled)
	{
		if (hookData->byteStorage == nullptr)  //check if byteStorage is null - if it is this is the first access and so need to read bytes into it
		{
			hookData->byteStorage = new BYTE[numberOfBytes];
			GameImageHooker::readRange(hookData->locationInImage() + hookData->customOffset(), hookData->byteStorage, numberOfBytes);
		}
		if (enabled)
		{
			if (!hookData->nopState)
			{
				GameImageHooker::nopRange(hookData->locationInImage() + hookData->customOffset(), numberOfBytes);
				hookData->nopState = true;
			}
			else
			{
				OverlayConsole::instance().logLine("Already Nopped - this shouldnt be showing. Something isnt working right");
			}
		}
		if (!enabled)
		{
			if (hookData->nopState)
			{
				GameImageHooker::writeRange(hookData->locationInImage() + hookData->customOffset(), hookData->byteStorage, numberOfBytes);
				hookData->nopState = false;
			}
			else
			{
				OverlayConsole::instance().logLine("Already Disabled - this shouldnt be showing. Something isnt working right");
			}
		}
	}
	
	void SaveBytesWrite(AOBBlock* hookData, int numberOfBytes, BYTE* BytestoWrite, bool enabled)
	{
		if (hookData->byteStorage2 == nullptr)  //check if byteStorage is null - if it is this is the first access and so need to read bytes into it
		{
			hookData->byteStorage2 = new BYTE[numberOfBytes];
			GameImageHooker::readRange(hookData->locationInImage() + hookData->customOffset(), hookData->byteStorage2, numberOfBytes);
		}
		if (enabled)
		{
			if (!hookData->nopState2)
			{
				GameImageHooker::writeRange(hookData->locationInImage() + hookData->customOffset(), BytestoWrite, numberOfBytes);
				hookData->nopState2 = true;
			}
			else
			{
				OverlayConsole::instance().logLine("Already Nopped - this shouldnt be showing. Something isnt working right");
			}
		}
		if (!enabled)
		{
			if (hookData->nopState2)
			{
				GameImageHooker::writeRange(hookData->locationInImage() + hookData->customOffset(), hookData->byteStorage2, numberOfBytes);
				hookData->nopState2 = false;
			}
			else
			{
				OverlayConsole::instance().logLine("Already Disabled - this shouldnt be showing. Something isnt working right");
			}
		}
	}
}
