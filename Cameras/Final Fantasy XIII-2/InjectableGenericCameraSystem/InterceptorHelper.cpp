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
	void cameraStructInterceptor2();
	void cameraWrite1Interceptor();
	void cameraWrite1Interceptor2();
	void timestopInterceptor();
}

// external addresses used in asm.
extern "C" {
	LPBYTE _cameraStructInterceptionContinue = nullptr;
	LPBYTE _cameraStruct2InterceptionContinue = nullptr;
	LPBYTE _cameraWrite1InterceptionContinue = nullptr;
	LPBYTE _cameraWrite1InterceptionContinue2 = nullptr;
	LPBYTE  _timestopInterceptionContinue = nullptr;
}


namespace IGCS::GameSpecific::InterceptorHelper
{
	void initializeAOBBlocks(LPBYTE hostImageAddress, DWORD hostImageSize, map<string, AOBBlock*> &aobBlocks)
	{
		aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY] = new AOBBlock(CAMERA_ADDRESS_INTERCEPT_KEY, "8B 86 C4 00 00 00 8B 8E C8 00 00 00 3B C1", 1);
		aobBlocks[CAMERA_WRITE1_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE1_INTERCEPT_KEY, "F3 0F 7E 01 66 0F D6 00 F3 0F 7E 41 08 66 0F D6 40 08 F3 0F 7E 41 10 66 0F D6 40 10 F3 0F 7E 41 18 66 0F D6 40 18 F3 0F 7E 41 20 66 0F D6 40 20 F3 0F 7E 41 28 66 0F D6 40 28 F3 0F 7E 41 30 83 C1 30 66 0F D6 40 30 F3 0F 7E 41 08 66 0F D6 40 38 C2 04 00 CC CC CC CC CC CC 56", 1);

		//aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY2] = new AOBBlock(CAMERA_ADDRESS_INTERCEPT_KEY2, "8B 4F 04 89 81 B0 00 00 00 89 99 B8 00 00 00 8B 06 8B 50 24 6A 01 8B CE FF D2 8B D8 8B 06 8B 50 04 6A 01 8B CE FF D2 8B 4F 04 89 81 B4 00 00 00 89 99 BC 00 00 00 8B 4F 04 0F B6 81 15 01 00 00 83 E8 00 74 3D 83 E8 01 74 48", 1);
		//aobBlocks[CAMERA_WRITE1_INTERCEPT_KEY2] = new AOBBlock(CAMERA_WRITE1_INTERCEPT_KEY2, "F3 0F 7E 44 24 04 66 0F D6 41 04", 1);

		aobBlocks[FOV_WRITE_KEY] = new AOBBlock(FOV_WRITE_KEY, "F3 0F 11 8F A8 02 00 00", 1);
		aobBlocks[FOV_WRITE_KEY_CUTSCENE] = new AOBBlock(FOV_WRITE_KEY_CUTSCENE, "D9 58 20 56 D9 41 24 D9 58 24 8B 50 34", 1);
		aobBlocks[NEAR_PLANE_KEY_CUTSCENE] = new AOBBlock(NEAR_PLANE_KEY_CUTSCENE, "D9 41 28 D9 58 28 D9 41 2C D9 58 2C D9 41 30 D9 58 30 33 51 34", 1);
		aobBlocks[TIMESTOP_INTERCEPT_KEY] = new AOBBlock(TIMESTOP_INTERCEPT_KEY, "D9 5C 24 14 F3 0F 2A 44 24 0C", 1);
		aobBlocks[GAMEPLAY_POST_FX] = new AOBBlock(GAMEPLAY_POST_FX, "74 32 8B 43 08 8B 53 6C", 1);
		aobBlocks[CUTSCENE_POST_FX] = new AOBBlock(CUTSCENE_POST_FX, "74 3D 8B 53 6C 8B 44 24 0C", 1);
		aobBlocks[HUDTOGGLE] = new AOBBlock(HUDTOGGLE, "0F 84 ?? ?? ?? ?? 80 79 ?? ?? 0F 84 ?? ?? ?? ?? 8B 4F ??", 1);
		aobBlocks[HUDTOGGLE2] = new AOBBlock(HUDTOGGLE2, "8B 45 08 | F3 0F 10 05 ?? ?? ?? ?? 0F C6 C0 00", 1);

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
		GameImageHooker::setHook(aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY], 0xE, &_cameraStructInterceptionContinue, &cameraStructInterceptor);
		//GameImageHooker::setHook(aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY2], 0xF, &_cameraStruct2InterceptionContinue, &cameraStructInterceptor2);
	}


	void setPostCameraStructHooks(map<string, AOBBlock*> &aobBlocks)
	{
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE1_INTERCEPT_KEY], 0x51, &_cameraWrite1InterceptionContinue, &cameraWrite1Interceptor);
		//GameImageHooker::setHook(aobBlocks[CAMERA_WRITE1_INTERCEPT_KEY2], 0x5D, &_cameraWrite1InterceptionContinue2, &cameraWrite1Interceptor2);
		GameImageHooker::setHook(aobBlocks[TIMESTOP_INTERCEPT_KEY], 0x10, &_timestopInterceptionContinue, &timestopInterceptor);
	}


	void toggleHud(map<string, AOBBlock*>& aobBlocks, bool hudVisible)
	{
		BYTE jumpbyte[] = {0xE9, 0xCA, 0x00, 0x00, 0x00, 0x90};
		hudVisible ? Utils::SaveBytesWrite(aobBlocks[HUDTOGGLE], 6, jumpbyte, false) : Utils::SaveBytesWrite(aobBlocks[HUDTOGGLE], 6, jumpbyte, true);
		hudVisible ? Utils::SaveNOPReplace(aobBlocks[HUDTOGGLE2], 8, false) : Utils::SaveNOPReplace(aobBlocks[HUDTOGGLE2], 8, true);
	}

	//Placeholder code for if absolute addresses are required. Init variable in extern for use in ASM
	void getAbsoluteAddresses(map<string, AOBBlock*>& aobBlocks)
	{
		//var1 = Utils::calculateAbsoluteAddress(aobBlocks[KEYNAME], numberofbytes);
		//OverlayConsole::instance().logDebug("Absolute Address for Var1: %p", (void*)var1);
	}

	void cameraSetup(map<string, AOBBlock*>& aobBlocks, bool enabled, GameAddressData& addressData)
	{
		BYTE jumpbyte = { 0xEB };
		Utils::SaveNOPReplace(aobBlocks[FOV_WRITE_KEY], 16, enabled);
		Utils::SaveNOPReplace(aobBlocks[FOV_WRITE_KEY_CUTSCENE], 3, enabled);
		Utils::SaveNOPReplace(aobBlocks[NEAR_PLANE_KEY_CUTSCENE], 3, enabled);
		Utils::SaveBytesWrite(aobBlocks[GAMEPLAY_POST_FX], 1, &jumpbyte, enabled);
		Utils::SaveBytesWrite(aobBlocks[CUTSCENE_POST_FX], 1, &jumpbyte, enabled);
	}

	void toolsInit(map<string, AOBBlock*>& aobBlocks)
	{
		//Utils::SaveNOPReplace(aobBlocks[TIMESTOP_WRITE_KEY], 10, true);
		//Utils::SaveNOPReplace(aobBlocks[TIMESTOP_WRITE_KEY2], 10, true);
	}

	void handleSettings(map<string, AOBBlock*>& aobBlocks, GameAddressData& addressData)
	{
		//BYTE jumpbyte = { 0xEB };
		//bool enabled = Globals::instance().settings().tonemapToggle;
		//if (enabled || !enabled)
		//{
		//	if (aobBlocks[PP_KEY]->nopState2 == enabled) { return; }
		//	Utils::SaveBytesWrite(aobBlocks[PP_KEY], 1, &jumpbyte, enabled);
		//}
	}
}
