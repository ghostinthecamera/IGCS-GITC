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
	void debugcaminterceptor();
	void dofStruct();
	void timescaleinterceptor();
	void fovintercept();
	void playerpointerinterceptor();
	void entitytimeinterceptor();
}

// external addresses used in asm.
extern "C" {
	LPBYTE _cameraStructInterceptionContinue = nullptr;
	LPBYTE _debugcamInterceptionContinue = nullptr;
	LPBYTE _dofInterceptionContinue = nullptr;
	LPBYTE _timestructinterceptionContinue = nullptr;
	LPBYTE _fovinterceptContinue = nullptr;
	LPBYTE _playerpointerContinue = nullptr;
	LPBYTE _entitytimeContinue = nullptr;
}


namespace IGCS::GameSpecific::InterceptorHelper
{
	void initializeAOBBlocks(LPBYTE hostImageAddress, DWORD hostImageSize, map<string, AOBBlock*> &aobBlocks)
	{
		aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY] = new AOBBlock(CAMERA_ADDRESS_INTERCEPT_KEY, "FF FF | 0F 28 00 66 0F 7F 43 10", 1);	
		aobBlocks[CAM_BLOCK1] = new AOBBlock(CAM_BLOCK1, "21 19 C1 FF | 0F 28 00 66 0F 7F 43 10", 1);
		aobBlocks[FOV_INTERCEPT_KEY] = new AOBBlock(FOV_INTERCEPT_KEY, "89 41 50 48 8B D9 8B", 1);
		aobBlocks[FOV_INTERCEPT_DEBUG] = new AOBBlock(FOV_INTERCEPT_DEBUG, "08 8B 41 50 | 89 43 50 8B 41 54", 1);
		aobBlocks[DOF_KEY] = new AOBBlock(DOF_KEY, "41 3B 40 28 41 8B 41 2C", 1);
		aobBlocks[TIMESTOP_KEY] = new AOBBlock(TIMESTOP_KEY, "F3 0F 59 88 68 02 00 00 F3 0F 59 88 64 03 00 00 48 8D 3D", 1);
		aobBlocks[UVKEY] = new AOBBlock(UVKEY, "F3 0F 58 4E 50 F3 0F 11 4E 50", 1);
		aobBlocks[PLAYERPOINTER_KEY] = new AOBBlock(PLAYERPOINTER_KEY, "38 | 48 8B 80 F8 1F 00 00 48 8B 48 28", 1);
		aobBlocks[ENTITY_KEY] = new AOBBlock(ENTITY_KEY, "F3 0F 10 83 ?? ?? ?? ?? F3 0F 59 83 ?? ?? ?? ?? F3 0F 59 D0", 1);
		aobBlocks[FPSUNLOCK] = new AOBBlock(FPSUNLOCK, "89 88 88 3C 4C 89 AB 70 02 00 00 E8 10 C9 7C FF", 1);

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
		GameImageHooker::setHook(aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY], 0x23, &_cameraStructInterceptionContinue, &cameraStructInterceptor);
		GameImageHooker::setHook(aobBlocks[CAM_BLOCK1], 0x23, &_debugcamInterceptionContinue, &debugcaminterceptor);
		GameImageHooker::setHook(aobBlocks[DOF_KEY], 0x11, &_dofInterceptionContinue, &dofStruct);
		GameImageHooker::setHook(aobBlocks[TIMESTOP_KEY], 0x10, &_timestructinterceptionContinue, &timescaleinterceptor);
		GameImageHooker::setHook(aobBlocks[UVKEY], 0xE, &_fovinterceptContinue, &fovintercept);
		GameImageHooker::setHook(aobBlocks[PLAYERPOINTER_KEY], 0xF, &_playerpointerContinue, &playerpointerinterceptor);
		GameImageHooker::setHook(aobBlocks[ENTITY_KEY], 0x10, &_entitytimeContinue, &entitytimeinterceptor);
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
			MessageHandler::logDebug("Initial Range Read %S", static_cast<string>(hookData->_blockName));
			MessageHandler::logDebug("Initial Bytes Read %X", hookData->byteStorage);

		}
		if (enabled)
		{
			if (!hookData->nopState)
			{
				GameImageHooker::nopRange(hookData->locationInImage() + hookData->customOffset(), numberOfBytes);
				MessageHandler::logDebug("Range Nopped %S", static_cast<string>(hookData->_blockName));
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
				MessageHandler::logDebug("Range Restored %S", static_cast<string>(hookData->_blockName));
				MessageHandler::logDebug("Bytes Restored %X", hookData->byteStorage);
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
