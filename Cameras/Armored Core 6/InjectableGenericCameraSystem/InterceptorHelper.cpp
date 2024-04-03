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
	void cameraWrite1Interceptor();
	void timestopInterceptor();
}

// external addresses used in asm.
extern "C" {
	LPBYTE _cameraStructInterceptionContinue = nullptr;
	LPBYTE _cameraWrite1InterceptionContinue = nullptr;
	LPBYTE  _timestopInterceptionContinue = nullptr;
}


namespace IGCS::GameSpecific::InterceptorHelper
{
	void initializeAOBBlocks(LPBYTE hostImageAddress, DWORD hostImageSize, map<string, AOBBlock*> &aobBlocks)
	{
		aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY] = new AOBBlock(CAMERA_ADDRESS_INTERCEPT_KEY, "48 8B 46 ?? 48 8D 55 ?? 0F 28 40 ?? 0F 29 45 ??", 1);
		aobBlocks[CAMERA_WRITE1_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE1_INTERCEPT_KEY, "0F 28 43 ?? 0F 29 47 ?? 0F 28 4B ?? 0F 29 4F ?? 0F 28 43 ?? 0F 29 47 ?? 0F 28 4B ?? 48 8B 5C 24 ?? 0F 29 4F ??", 1);
		aobBlocks[FOV_WRITE_KEY] = new AOBBlock(FOV_WRITE_KEY, "F3 0F 11 49 ?? F3 0F 11 49 ?? F3 0F 11 49 ?? F3 0F 11 49 ?? C3 33 C0", 1);
		aobBlocks[TIMESTOP_INTERCEPT_KEY] = new AOBBlock(TIMESTOP_INTERCEPT_KEY, "F3 0F 10 88 ?? ?? ?? ?? F3 0F 59 88 ?? ?? ?? ?? F3 0F 59 88 ?? ?? ?? ?? 48 8D 35 ?? ?? ?? ??", 1);
		aobBlocks[TIMESTOP_WRITE_KEY] = new AOBBlock(TIMESTOP_WRITE_KEY, "C7 83 ?? ?? ?? ?? 00 00 80 3F 48 83 3D ?? ?? ?? ?? ??", 1);
		aobBlocks[TIMESTOP_WRITE_KEY2] = new AOBBlock(TIMESTOP_WRITE_KEY2, "C7 81 ?? ?? ?? ?? 00 00 80 3F C3 CC 48 89 ??", 1);
		aobBlocks[HUD_KEY] = new AOBBlock(HUD_KEY, "42 FF 14 C0 48 63 43 ?? 89 43 ?? 4C 39 73 ?? 74 ?? 44 38 73 69 74 29 83 F8 12", 1);
		aobBlocks[VIGNETTE_KEY] = new AOBBlock(VIGNETTE_KEY, "45 ?? ?? ?? ?? | 0F ?? ?? 49 8B ?? 5B C3 CC", 1);
		aobBlocks[PAUSE_BYTE_KEY] = new AOBBlock(PAUSE_BYTE_KEY, "66 ?? ?? ?? ?? ?? ?? | 01 00 80 BF ?? ?? ?? ?? ?? 75 ?? E8 ?? ?? ?? ??", 1);

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
		GameImageHooker::setHook(aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY], (0xA616 - 0xA606), &_cameraStructInterceptionContinue, &cameraStructInterceptor);
	}


	void setPostCameraStructHooks(map<string, AOBBlock*> &aobBlocks)
	{
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE1_INTERCEPT_KEY], 0x24, &_cameraWrite1InterceptionContinue, &cameraWrite1Interceptor);
		GameImageHooker::setHook(aobBlocks[TIMESTOP_INTERCEPT_KEY], 0x18, &_timestopInterceptionContinue, &timestopInterceptor);
	}


	void toggleHud(map<string, AOBBlock*>& aobBlocks, bool hudVisible)
	{
		hudVisible ? Utils::SaveNOPReplace(aobBlocks[HUD_KEY], 4, false) : Utils::SaveNOPReplace(aobBlocks[HUD_KEY], 4, true);
	}

	void togglePause(map<string, AOBBlock*>& aobBlocks, bool enabled)
	{
		BYTE pause[] = { 0x00, 0x00 };
		enabled ? Utils::SaveBytesWrite(aobBlocks[PAUSE_BYTE_KEY], 2, pause, true) : Utils::SaveBytesWrite(aobBlocks[PAUSE_BYTE_KEY], 2, pause, false);
	}

	//Placeholder code for if absolute addresses are required. Init variable in extern for use in ASM
	void getAbsoluteAddresses(map<string, AOBBlock*>& aobBlocks)
	{
		//var1 = Utils::calculateAbsoluteAddress(aobBlocks[KEYNAME], numberofbytes);
		//OverlayConsole::instance().logDebug("Absolute Address for Var1: %p", (void*)var1);
	}

	void cameraSetup(map<string, AOBBlock*>& aobBlocks, bool enabled)
	{
		Utils::SaveNOPReplace(aobBlocks[FOV_WRITE_KEY], 5, enabled);
	}

	void toolsInit(map<string, AOBBlock*>& aobBlocks)
	{
		Utils::SaveNOPReplace(aobBlocks[TIMESTOP_WRITE_KEY], 10, true);
		//Utils::SaveNOPReplace(aobBlocks[TIMESTOP_WRITE_KEY2], 10, true);
	}

	void handleSettings(map<string, AOBBlock*>& aobBlocks)
	{
		//Vignette Toggle
		bool enabled = Globals::instance().settings().vignetteToggle;
		if (enabled || !enabled)
		{
			if (aobBlocks[VIGNETTE_KEY]->nopState == enabled){return;}
			Utils::SaveNOPReplace(aobBlocks[VIGNETTE_KEY], 3, enabled);
		}
	}
}
