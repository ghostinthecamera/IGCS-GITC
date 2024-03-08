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
#include "InterceptorHelper.h"
#include "GameConstants.h"
#include "GameImageHooker.h"
#include <map>
#include "MessageHandler.h"
#include "CameraManipulator.h"
#include "Globals.h"

using namespace std;

//--------------------------------------------------------------------------------------------------------------------------------
// external asm functions
extern "C" {
	void cameraStructInterceptor();
	void timestopReadInterceptor();
	void fovReadInterceptor();
	void lodSettingInterceptor();
}

// external addresses used in asm.
extern "C" {
	LPBYTE _cameraStructInterceptionContinue = nullptr;
	LPBYTE _timestopReadInterceptionContinue = nullptr;
	LPBYTE _fovReadInterceptionContinue = nullptr;
	LPBYTE _timestopAbsolute = nullptr;
	LPBYTE _lodSettingInterceptionContinue = nullptr;
}


namespace IGCS::GameSpecific::InterceptorHelper
{
	void initializeAOBBlocks(LPBYTE hostImageAddress, DWORD hostImageSize, map<string, AOBBlock*> &aobBlocks)
	{
		aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY] = new AOBBlock(CAMERA_ADDRESS_INTERCEPT_KEY, "41 0F 11 86 ?? ?? ?? ?? 41 0F 11 8E ?? ?? ?? ?? E8 ?? ?? ?? ?? 0F B6 84 24", 1);
		aobBlocks[TIMESTOP_READ_INTERCEPT_KEY] = new AOBBlock(TIMESTOP_READ_INTERCEPT_KEY, "F3 0F 10 4B ?? 48 8B 4B ??", 1);
		aobBlocks[FOV1_KEY] = new AOBBlock(FOV1_KEY, "8B 02 89 01 8B 42 ?? 89 41 ?? 0F 28 42 ?? 66 0F 7F 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? | 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 0F B6 42 ?? 88 41 ?? C3", 1);
		aobBlocks[FOV2_KEY] = new AOBBlock(FOV2_KEY, "8B 02 89 01 8B 42 ?? 89 41 ?? 0F 28 42 ?? 66 0F 7F 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? | 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 0F B6 42 ?? 88 41 ?? C3", 1);
		aobBlocks[LOD_SETTING_KEY] = new AOBBlock(LOD_SETTING_KEY, "41 88 86 ?? ?? ?? ?? 41 89 96 ?? ?? ?? ?? 84 C0 0F 84 ?? ?? ?? ??", 1);
		aobBlocks[ABS1_TIMESCALE_INTERCEPT_KEY] = new AOBBlock(ABS1_TIMESCALE_INTERCEPT_KEY, "F3 0F 10 4B ?? 48 8B 4B ?? E8 | ?? ?? ?? ??", 1);
		//aobBlocks[FOV_WRITE_NOP_KEY] = new AOBBlock(FOV_WRITE_NOP_KEY, "F3 41 0F 11 46 ?? 8B 46 ?? 41 89 46 ?? 41 C6 86 ?? ?? ?? ?? ??", 1);
		//aobBlocks[CAMERA_MOVE_WRITE_NOP_KEY] = new AOBBlock(CAMERA_MOVE_WRITE_NOP_KEY, "0F 11 47 ?? E8 ?? ?? ?? ?? 0F 57 D2", 1);
		//aobBlocks[CAMERA_MOVE_WRITE_NOP_KEY2] = new AOBBlock(CAMERA_MOVE_WRITE_NOP_KEY2, "0F 11 53 ?? E8 ?? ?? ?? ?? 66 0F 6F 1D ?? ?? ?? ?? 48 8D 4D ?? 0F 28", 1);
		//aobBlocks[CAMERA_MOVE_WRITE_NOP_KEY3] = new AOBBlock(CAMERA_MOVE_WRITE_NOP_KEY3, "F3 0F 7F 07 45 84 E4", 1);
		//aobBlocks[CAMERA_MOVE_WRITE_NOP_KEY4] = new AOBBlock(CAMERA_MOVE_WRITE_NOP_KEY4, "0F 11 07 0f 28 BC 24 ?? ?? ?? ??", 1);

		map<string, AOBBlock*>::iterator it;
		bool result = true;
		for (it = aobBlocks.begin(); it != aobBlocks.end(); it++)
		{
			result &= it->second->scan(hostImageAddress, hostImageSize);
		}
		if (result)
		{
			MessageHandler::logLine("All interception offsets found.");
		}
		else
		{
			MessageHandler::logError("One or more interception offsets weren't found: tools aren't compatible with this game's version.");
		}
	}


	void setCameraStructInterceptorHook(map<string, AOBBlock*> &aobBlocks)
	{
		GameImageHooker::setHook(aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY], 0x10, &_cameraStructInterceptionContinue, &cameraStructInterceptor);
	}


	void setPostCameraStructHooks(map<string, AOBBlock*> &aobBlocks)
	{
		GameImageHooker::setHook(aobBlocks[TIMESTOP_READ_INTERCEPT_KEY], 0x0E, &_timestopReadInterceptionContinue, &timestopReadInterceptor);
		GameImageHooker::setHook(aobBlocks[FOV1_KEY], 0x0F, &_fovReadInterceptionContinue, &fovReadInterceptor);
		GameImageHooker::setHook(aobBlocks[LOD_SETTING_KEY], 0x0E, &_lodSettingInterceptionContinue, &lodSettingInterceptor);
	}

	void getAbsoluteAddresses(map<string, AOBBlock*>& aobBlocks)
	{
		_timestopAbsolute = Utils::calculateAbsoluteAddress(aobBlocks[ABS1_TIMESCALE_INTERCEPT_KEY], 4);

		MessageHandler::logLine("Absolute Address for timescale1: %p", (void*)_timestopAbsolute);
		//OverlayConsole::instance().logDebug("Absolute Address for timescale2: %p", (void*)_timestopTwoAbsolute);
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
				MessageHandler::logError("Already Nopped - this shouldnt be showing. Something isnt working right");
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
				MessageHandler::logError("Already Disabled - this shouldnt be showing. Something isnt working right");
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
				MessageHandler::logError("Already Nopped - this shouldnt be showing. Something isnt working right");
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
				MessageHandler::logError("Already Disabled - this shouldnt be showing. Something isnt working right");
			}
		}
	}
}
