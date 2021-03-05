////////////////////////////////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System
// Copyright(c) 2017, Frans Bouma
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

using namespace std;

//--------------------------------------------------------------------------------------------------------------------------------
// external asm functions
extern "C" {
	void cameraStructInterceptor();
	void DOFinterceptor();
	void ARread();
	void BLOOMinterceptor();
}

// external addresses used in asm.
extern "C" {
	LPBYTE _cameraStructInterceptionContinue = nullptr;
	LPBYTE _dofstructInterceptionContinue = nullptr;
	LPBYTE _ARInterceptionContinue = nullptr;
	LPBYTE _bloomstructinterceptionContinue = nullptr;
}


namespace IGCS::GameSpecific::InterceptorHelper
{
	void initializeAOBBlocks(LPBYTE hostImageAddress, DWORD hostImageSize, map<string, AOBBlock*> &aobBlocks)
	{
		aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY] = new AOBBlock(CAMERA_ADDRESS_INTERCEPT_KEY, "8B 01 89 02 8B 41 04 89 42 04 8B 41 08 89 42 08 8B 49 0C 89 4A 0C 8B 55 F0 83 C2 10 89 55 FC 8B 45 08 83 C0 10 8B 4D FC 8B 10 89 11 8B 50 04 89 51 04 8B 50 08 89 51 08 8B 40 0C 89 41 0C 8B 4D F0 83 C1 20 89 4D F8 8B 55 08 83 C2 20 8B 45 F8 8B 0A 89 08 8B 4A 04 89 48 04 8B 4A 08 89 48 08 8B 52 0C 89 50 0C 8B 45 F0 83 C0 30 89 45 F4 8B 4D 08 83 C1 30 8B 55 F4 8B 01 89 02 8B 41 04 89 42 04 8B 41 08 89 42 08 8B 49 0C 89 4A 0C 8B 55 EC C6 82 AD 00 00 00 01 8B E5 5D C2 04 00 CC CC CC CC CC CC CC CC CC CC 55 8B EC 83 EC 14 89 4D EC 8B 45 EC 83 C0 48 89 45 F0 8B 4D 08 8B 55 F0 8B 01 89 02 8B 41 04 89 42 04 8B 41 08 89 42 08", 1);	
		aobBlocks[HFOV_INTERCEPT_KEY] = new AOBBlock(HFOV_INTERCEPT_KEY, "48 89 45 F0 8B 4D 08 8B 55 F0 8B 01 | 89 02 8B 41 04", 1);
		aobBlocks[VFOV_INTERCEPT_KEY] = new AOBBlock(VFOV_INTERCEPT_KEY, "89 51 04 8B 50 08 89 51 08 8B 40 0C 89 41 0C 8B 4D F0 83 C1 20 89 4D F8 8B 55 08 83 C2 20 8B 45 F8 8B 0A 89 08 8B 4A 04 89 48 04 8B 4A 08 89 48 08 8B 52 0C 89 50 0C 8B 45 F0 83 C0 30 89 45 F4 8B 4D 08 83 C1 30 8B 55 F4 8B 01 89 02 8B 41 04 89 42 04 8B 41 08 89 42 08 8B 49 0C 89 4A 0C 8B 55 EC C6 82 AD 00 00 00 01 8B E5 5D C2 04 00 CC CC CC CC CC CC CC CC CC CC 55 8B EC 51 89 4D FC 8B 45 FC D9 80 B0 00 00 00 8B E5 5D C3 CC CC", 1);
		aobBlocks[HUDTOGGLE] = new AOBBlock(HUDTOGGLE, "FF D0 8B 4D FC E8 74", 1);
		aobBlocks[TIMESTOP_KEY] = new AOBBlock(TIMESTOP_KEY, "E8 B9 12 00 00 83", 1);
		aobBlocks[DOF_KEY] = new AOBBlock(DOF_KEY, "F3 0F 11 42 40 8B 45 C0", 1);
		aobBlocks[AR_KEY] = new AOBBlock(AR_KEY, "D9 81 FC 03 00 00 D9 1C 24 F3 0F 10 84 24 8C 01 00 00", 1);
		aobBlocks[BLOOM_KEY] = new AOBBlock(BLOOM_KEY, "89 8D E4 FE FF FF 8B 85 E4 FE FF FF F3 0F 10 40 10", 1);

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


	void setCameraStructInterceptorHook(map<string, AOBBlock*>& aobBlocks)
	{
		GameImageHooker::setHook(aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY], 0x8E, &_cameraStructInterceptionContinue, &cameraStructInterceptor);
		GameImageHooker::setHook(aobBlocks[DOF_KEY], 0x10, &_dofstructInterceptionContinue, &DOFinterceptor);
		GameImageHooker::setHook(aobBlocks[AR_KEY], 0x12, &_ARInterceptionContinue, &ARread);
		GameImageHooker::setHook(aobBlocks[BLOOM_KEY], 0x11, &_bloomstructinterceptionContinue, &BLOOMinterceptor);
	}

	void setPostCameraStructHooks(map<string, AOBBlock*> & aobBlocks)
	{
	}

	void SaveNOPReplace(AOBBlock * hookData, int numberOfBytes, bool enabled)
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

	void SaveBytesWrite(AOBBlock * hookData, int numberOfBytes, BYTE * BytestoWrite, bool enabled)
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
