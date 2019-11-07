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
	void cameraFOVInterceptor();
	void timescaleInterceptor();
	//void todWriteInterceptor();
	//void fogReadInterceptor();*/
}

// external addresses used in asm.
extern "C" {
	LPBYTE _cameraStructInterceptionContinue = nullptr;
	LPBYTE _fovStructInterceptionContinue = nullptr;
	LPBYTE _timescaleInterceptionContinue = nullptr;
	/*LPBYTE _todWriteInterceptionContinue = nullptr;
	LPBYTE _fogReadInterceptionContinue = nullptr;*/
}


namespace IGCS::GameSpecific::InterceptorHelper
{
	void initializeAOBBlocks(LPBYTE hostImageAddress, DWORD hostImageSize, map<string, AOBBlock*> &aobBlocks)
	{
		aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY] = new AOBBlock(CAMERA_ADDRESS_INTERCEPT_KEY, "66 0F 7F 12 66 0F 7F 4A ?? 66", 1);	
		aobBlocks[HUD_RENDER_INTERCEPT_KEY] = new AOBBlock(HUD_RENDER_INTERCEPT_KEY, "48 85 D2 0F 84 ?? ?? ?? ?? 4C 8B DC 55 41 55 41 56", 1);
		aobBlocks[FOV_INTERCEPT_KEY] = new AOBBlock(FOV_INTERCEPT_KEY, "66 0F 7F 43 ?? 0F 28 48 ?? 66 0F 7F 4B ?? 0F 28 40 ?? 66 0F 7F 43 ?? 0F 28 48 ?? 48 8D 43 ??", 1);
		aobBlocks[TIMESCALE_INTERCEPT_KEY] = new AOBBlock(TIMESCALE_INTERCEPT_KEY, "F3 44 0F10 48 ?? F3 44 0F5C 48 ??", 1);
		//aobBlocks[TIMESTOP_KEY] = new AOBBlock(TIMESTOP_KEY, "8B 90 ?? ?? ?? ?? 0F 14 F6 4C 8D 44 24 ??", 1);


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
		GameImageHooker::setHook(aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY], 0x1C, &_cameraStructInterceptionContinue, &cameraStructInterceptor);
	}


	void setPostCameraStructHooks(map<string, AOBBlock*> &aobBlocks)
	{
		
		GameImageHooker::setHook(aobBlocks[FOV_INTERCEPT_KEY], 0x0E, &_fovStructInterceptionContinue, &cameraFOVInterceptor);
		GameImageHooker::setHook(aobBlocks[TIMESCALE_INTERCEPT_KEY], 0x12, &_timescaleInterceptionContinue, &timescaleInterceptor);
		//GameImageHooker::setHook(aobBlocks[TIMESTOP_KEY], 0x0E, &_timestopReadInterceptionContinue, &timestopReadInterceptor);
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
