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
	void cameraWrite2Interceptor();
	void cameraWrite3Interceptor();
	void resolutionInterceptor();
	void aspectratioInterceptor();
}

// external addresses used in asm.
extern "C" {
	LPBYTE _cameraStructInterceptionContinue = nullptr;
	LPBYTE _cameraWrite1InterceptionContinue = nullptr;
	LPBYTE _cameraWrite2InterceptionContinue = nullptr;
	LPBYTE _cameraWrite3InterceptionContinue = nullptr;
	LPBYTE _resolutionInterceptionContinue = nullptr;
	LPBYTE _aspectratioInterceptionContinue = nullptr;
	LPBYTE _timestopAddress = nullptr;
}


namespace IGCS::GameSpecific::InterceptorHelper
{
	void initializeAOBBlocks(LPBYTE hostImageAddress, DWORD hostImageSize, map<string, AOBBlock*> &aobBlocks)
	{
		aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY] = new AOBBlock(CAMERA_ADDRESS_INTERCEPT_KEY, "48 8B 04 ?? 48 8B 40 ?? 48 8B 80 48 09 00 00 80 78 ?? ?? 0F 84 ?? ?? ?? ??", 1);
		aobBlocks[CAMERA_WRITE1_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE1_INTERCEPT_KEY, "C5 F8 29 46 ?? C5 F8 29 4E ?? C5 F8 28 86 ?? ?? ?? ??", 1);
		aobBlocks[CAMERA_WRITE2_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE2_INTERCEPT_KEY, "C5 F8 29 4E ?? C5 F8 29 56 ?? C5 F8 29 5E ?? C5 FA 10 0D ?? ?? ?? ??", 1);
		aobBlocks[CAMERA_WRITE3_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE3_INTERCEPT_KEY, "C5 F8 29 4E ?? C5 F8 29 46 ?? C5 F8 29 56 ?? C5 FA 10 86 ?? ?? ?? ??", 1);
		aobBlocks[CAMERA_WRITE4_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE4_INTERCEPT_KEY, "C5 78 29 2D ?? ?? ?? ?? C5 78 29 0D ?? ?? ?? ?? C5 F8 28 0D ?? ?? ?? ??", 1);
		aobBlocks[FOV_WRITE_KEY1] = new AOBBlock(FOV_WRITE_KEY1, "C4 41 7A 11 AE ?? ?? ?? ?? 41 80 BE ?? ?? ?? ?? ?? 0F 84 ?? ?? ?? ??", 1);
		aobBlocks[FOV_WRITE_KEY2] = new AOBBlock(FOV_WRITE_KEY2, "C5 FA 11 96 ?? ?? ?? ?? C5 78 28 05 ?? ?? ?? ?? C5 78 29 84 ?? ?? ?? ?? ??", 1);
		aobBlocks[FOV_WRITE_KEY3] = new AOBBlock(FOV_WRITE_KEY3, "C5 7A 11 8E ?? ?? ?? ?? C5 F8 28 75 ?? C5 F8 28 7D ??", 1);
		aobBlocks[TIMESTOP_ADDRESS] = new AOBBlock(TIMESTOP_ADDRESS, "C5 FB 11 05 | ?? ?? ?? ?? 48 8B ?? ?? ?? ?? ?? 48 8B ?? ?? ?? ?? ??", 1);
 		aobBlocks[TIMESTOP_WRITE_KEY] = new AOBBlock(TIMESTOP_WRITE_KEY, "C5 FB 11 05 ?? ?? ?? ?? 48 8B ?? ?? ?? ?? ?? 48 8B ?? ?? ?? ?? ??", 1);
		aobBlocks[TIMESTOP_WRITE_KEY2] = new AOBBlock(TIMESTOP_WRITE_KEY2, "75 ?? C5 ?? ?? ?? C5 79 ?? ?? 76 ??", 1);
		aobBlocks[HUD_KEY] = new AOBBlock(HUD_KEY, "74 ?? 66 90 48 8B 0F | E8 ?? ?? ?? ?? 48 83 C7 ?? 48 ?? ?? 75 ??", 1);
		aobBlocks[BLOOM_KEY] = new AOBBlock(BLOOM_KEY, "C5 F8 10 40 18 ?? ?? ?? ?? 18 ?? 8B", 1);
		aobBlocks[DOF_KEY] = new AOBBlock(DOF_KEY, "C5 7A 10 0D ?? ?? ?? ?? C5 78 29 C8 48 39 D1 74 ?? C5 F8 28 D7 E8 ?? ?? ?? ?? C5 F0 57 C9 C5 F2 5F C0 C5 B2 5D C0 C4 C1 4A 5C F0 C4 C2 79 A9 F0 C5 FA 11 75 ?? 48 8B 4D ??", 1);
		aobBlocks[DOF_KEY2] = new AOBBlock(DOF_KEY2, "FF ?? C5 FA 11 07 48 8B 4D ??", 1);
		aobBlocks[MENU_DOF_KEY] = new AOBBlock(MENU_DOF_KEY, "0F 84 ?? ?? ?? ?? | C5 C8 ?? ?? C5 FA 10 3D ?? ?? ?? ?? EB ??", 1);
		aobBlocks[AR_KEY] = new AOBBlock(AR_KEY, "C5 C2 59 08 C4 E2 79 B9 48 ?? C4 E2 49 B9 48 ?? ?? ?? ?? ?? 88", 1);
		aobBlocks[RESOLUTION_KEY] = new AOBBlock(RESOLUTION_KEY, "42 8B 54 09 ?? ?? 8B 44 09 ?? ?? 8B 44 09 ?? 42 8B 4C 09 ??", 1);

		map<string, AOBBlock*>::iterator it;
		bool result = true;
		for (it = aobBlocks.begin(); it != aobBlocks.end(); it++)
		{
			result &= it->second->scan(hostImageAddress, hostImageSize);
			//Sleep(500);
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
		GameImageHooker::setHook(aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY], 0xF, &_cameraStructInterceptionContinue, &cameraStructInterceptor);
	}


	void setPostCameraStructHooks(map<string, AOBBlock*> &aobBlocks)
	{
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE1_INTERCEPT_KEY], (0x1DE - 0x1C7), &_cameraWrite1InterceptionContinue, &cameraWrite1Interceptor);
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE2_INTERCEPT_KEY], (0x53C - 0x52D), &_cameraWrite2InterceptionContinue, &cameraWrite2Interceptor);
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE3_INTERCEPT_KEY], (0x5A0 - 0x591), &_cameraWrite3InterceptionContinue, &cameraWrite3Interceptor);
		GameImageHooker::setHook(aobBlocks[RESOLUTION_KEY], (0x25E - 0x24F), &_resolutionInterceptionContinue, &resolutionInterceptor);
		GameImageHooker::setHook(aobBlocks[AR_KEY], (0x1022 - 0x1012), &_aspectratioInterceptionContinue, &aspectratioInterceptor);
	}


	void toggleHud(map<string, AOBBlock*>& aobBlocks, bool hudVisible)
	{
		hudVisible ? Utils::SaveNOPReplace(aobBlocks[HUD_KEY], 5, false) : Utils::SaveNOPReplace(aobBlocks[HUD_KEY], 5, true);
	}

	//Placeholder code for if absolute addresses are required. Init variable in extern for use in ASM
	void getAbsoluteAddresses(map<string, AOBBlock*>& aobBlocks)
	{
		_timestopAddress = Utils::calculateAbsoluteAddress(aobBlocks[TIMESTOP_ADDRESS], 4);
		GameSpecific::CameraManipulator::setTimescaleAddress(_timestopAddress);
		MessageHandler::logDebug("Absolute Address for timescale1: %p", (void*)_timestopAddress);
	}

	void cameraSetup(map<string, AOBBlock*>& aobBlocks, bool enabled)
	{
		Utils::SaveNOPReplace(aobBlocks[FOV_WRITE_KEY1], 9, enabled);
		Utils::SaveNOPReplace(aobBlocks[FOV_WRITE_KEY2], 8, enabled);
		Utils::SaveNOPReplace(aobBlocks[FOV_WRITE_KEY3], 8, enabled);
		Utils::SaveNOPReplace(aobBlocks[BLOOM_KEY], 5, enabled);
		Utils::SaveNOPReplace(aobBlocks[DOF_KEY], 8, enabled);
		Utils::SaveNOPReplace(aobBlocks[DOF_KEY2], 2, enabled);
		Utils::SaveNOPReplace(aobBlocks[MENU_DOF_KEY], 8, enabled);
		Utils::SaveNOPReplace(aobBlocks[CAMERA_WRITE4_INTERCEPT_KEY], 48, enabled);
	}

	void toolsInit(map<string, AOBBlock*>& aobBlocks)
	{
		BYTE jmpbyte = JMP_BYTE;
		Utils::SaveBytesWrite(aobBlocks[TIMESTOP_WRITE_KEY2], 1, &jmpbyte, true);
	}

	void handleSettings(map<string, AOBBlock*>& aobBlocks)
	{
	}
}
