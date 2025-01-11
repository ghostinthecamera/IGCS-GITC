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
	void resolutionInterceptor();
	void aspectratioInterceptor();
	void battleARInterceptor();
	void cutsceneARInterceptor();
}

// external addresses used in asm.
extern "C" {
	LPBYTE _cameraStructInterceptionContinue = nullptr;
	LPBYTE _cameraWrite1InterceptionContinue = nullptr;
	LPBYTE _timestopInterceptionContinue = nullptr;
	LPBYTE _resolutionInterceptionContinue = nullptr;
	LPBYTE _aspectratioInterceptionContinue = nullptr;
	LPBYTE _battleARInterceptionContinue = nullptr;
	LPBYTE _cutsceneARInterceptionContinue = nullptr;
}


namespace IGCS::GameSpecific::InterceptorHelper
{
	void initializeAOBBlocks(LPBYTE hostImageAddress, DWORD hostImageSize, map<string, AOBBlock*> &aobBlocks)
	{
		aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY] = new AOBBlock(CAMERA_ADDRESS_INTERCEPT_KEY, "8B 8E C8 00 00 00 8B 86 C4 00 00 00 3B C1", 1);
		aobBlocks[CAMERA_WRITE1_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE1_INTERCEPT_KEY, "F3 0F 7E 01 66 0F D6 00 F3 0F 7E 41 08 66 0F D6 40 08 F3 0F 7E 41 10 66 0F D6 40 10 F3 0F 7E 41 18 66 0F D6 40 18 F3 0F 7E 41 20 66 0F D6 40 20 F3 0F 7E 41 28 66 0F D6 40 28 F3 0F 7E 41 30 66 0F D6 40 30 F3 0F 7E 41 38 66 0F D6 40 38 F3 0F 7E 41 40 66 0F D6 40 40 F3 0F 7E 41 48 66 0F D6 40 48 F3 0F 7E 41 50 66 0F D6 40 50 F3 0F 7E 41 58 66 0F D6 40 58 F3 0F 7E 41 60 66 0F D6 40 60 F3 0F 7E 41 68 66 0F D6 40 68 F3 0F 7E 41 70 66 0F D6 40 70 F3 0F 7E 41 78 66 0F D6 40 78 F3 0F 7E 81 80 00 00 00 66 0F D6 80 80 00 00 00 F3 0F 7E 81 88 00 00 00 66 0F D6 80 88 00 00 00 F3 0F 7E 81 90 00 00 00 66 0F D6 80 90 00 00 00 F3 0F 7E 81 98 00 00 00 66 0F D6 80 98 00 00 00 F3 0F 7E 81 A0 00 00 00 66 0F D6 80 A0 00 00 00 F3 0F 7E 81 A8 00 00 00 66 0F D6 80 A8 00 00 00 F3 0F 7E 81 B0 00 00 00 66 0F D6 80 B0 00 00 00 F3 0F 7E 81 B8 00 00 00 66 0F D6 80 B8 00 00 00 F3 0F 7E 81 C0 00 00 00 66 0F D6 80 C0 00 00 00 F3 0F 7E 81 C8 00 00 00 66 0F D6 80 C8 00 00 00 D9 81 D0 00 00 00", 1);
		aobBlocks[CAMERA_WRITE2_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE2_INTERCEPT_KEY, "0F 84 9E 00 00 00 66 0F D6 46 78", 1);

		aobBlocks[FOV_WRITE_KEY] = new AOBBlock(FOV_WRITE_KEY, "F3 0F 11 86 A0 02 00 00 F3 0F 11 8E A8 02 00 00", 1);
		aobBlocks[FOV_WRITE_KEY_CUTSCENE] = new AOBBlock(FOV_WRITE_KEY_CUTSCENE, "D9 58 20 56 D9 41 24", 1);
		aobBlocks[NEAR_PLANE_KEY_CUTSCENE] = new AOBBlock(NEAR_PLANE_KEY_CUTSCENE, "D9 58 28 D9 41 2C D9 58 2C D9 41 30 D9 58 30", 1);
		aobBlocks[TIMESTOP_INTERCEPT_KEY] = new AOBBlock(TIMESTOP_INTERCEPT_KEY, "D9 5D E8 66 0F 6E 45 F0 F3 0F 10 4D E8", 1);
		aobBlocks[DOF_KEY] = new AOBBlock(DOF_KEY, "0F 84 ?? ?? ?? ?? 8D 55 ?? 52 8D 45 ?? 50 E8 ?? ?? ?? ?? F3 0F 7E 45 ??", 1);
		aobBlocks[BLOOM_KEY] = new AOBBlock(BLOOM_KEY, "F3 0F 7E 07 66 0F D6 00 F3 0F 7E 47 ?? 66 0F D6 40 ?? F3 0F 7E 47 ?? 66 0F D6 40 ?? 8B 4F ?? 5F", 1);
		aobBlocks[HUD_KEY] = new AOBBlock(HUD_KEY, "0F 82 ?? ?? ?? ?? 5F 8B 4D ?? 89 86 ?? ?? ?? ??", 1);
		aobBlocks[RESOLUTION_INTERCEPT_KEY] = new AOBBlock(RESOLUTION_INTERCEPT_KEY, "8B 4D ?? 8B 55 ?? 39 88 ?? ?? ?? ??", 1);
		aobBlocks[AR_INTERCEPT_KEY] = new AOBBlock(AR_INTERCEPT_KEY, "66 0F D6 86 ?? ?? ?? ?? F3 0F 10 87 ?? ?? ?? ?? 6A ??", 1);
		aobBlocks[BATTLE_AR_INTERCEPT_KEY] = new AOBBlock(BATTLE_AR_INTERCEPT_KEY, "F3 0F 11 45 F4 F3 0F 7E 43 18", 1);
		aobBlocks[BATTLE_AR_NOP_KEY] = new AOBBlock(BATTLE_AR_NOP_KEY, "D9 58 30 33 51 34 83 E2 01 31 50 34", 1);
		aobBlocks[CUTSCENE_AR_INTERCEPT_KEY] = new AOBBlock(CUTSCENE_AR_INTERCEPT_KEY, "66 0F D6 45 F0 F3 0F 7E 43 18", 1);


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
	}


	void setPostCameraStructHooks(map<string, AOBBlock*> &aobBlocks)
	{
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE1_INTERCEPT_KEY], 0x4E, &_cameraWrite1InterceptionContinue, &cameraWrite1Interceptor);
		GameImageHooker::setHook(aobBlocks[TIMESTOP_INTERCEPT_KEY], 0x0F, &_timestopInterceptionContinue, &timestopInterceptor);
		GameImageHooker::setHook(aobBlocks[RESOLUTION_INTERCEPT_KEY], 0x2A, &_resolutionInterceptionContinue, &resolutionInterceptor);
		GameImageHooker::setHook(aobBlocks[AR_INTERCEPT_KEY], 0x10, &_aspectratioInterceptionContinue, &aspectratioInterceptor);
		GameImageHooker::setHook(aobBlocks[BATTLE_AR_INTERCEPT_KEY], 0x12, &_battleARInterceptionContinue, &battleARInterceptor);
		GameImageHooker::setHook(aobBlocks[CUTSCENE_AR_INTERCEPT_KEY], 0x12, &_cutsceneARInterceptionContinue, &cutsceneARInterceptor);
	}


	void toggleHud(map<string, AOBBlock*>& aobBlocks, bool hudVisible)
	{
		hudVisible ? Utils::SaveNOPReplace(aobBlocks[HUD_KEY], 6, false) : Utils::SaveNOPReplace(aobBlocks[HUD_KEY], 6, true);
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
		BYTE _camNop[6] = { 0xE9, 0x9F, 0x00, 0x00, 0x00, 0x90 };
		BYTE _dofNop[6] = { 0xE9, 0xD1, 0x00, 0x00, 0x00, 0x90 };


		Utils::SaveBytesWrite(aobBlocks[CAMERA_WRITE2_INTERCEPT_KEY], 6, _camNop, enabled);
		Utils::SaveNOPReplace(aobBlocks[FOV_WRITE_KEY], 16, enabled);
		Utils::SaveNOPReplace(aobBlocks[FOV_WRITE_KEY_CUTSCENE], 3, enabled);
		Utils::SaveNOPReplace(aobBlocks[NEAR_PLANE_KEY_CUTSCENE], 3, enabled);
		Utils::SaveBytesWrite(aobBlocks[DOF_KEY], 6, _dofNop, enabled);
		Utils::SaveNOPReplace(aobBlocks[BLOOM_KEY], 4, enabled);
	}

	void setBattleAR(map<string, AOBBlock*>& aobBlocks)
	{
		Utils::SaveNOPReplace(aobBlocks[BATTLE_AR_NOP_KEY], 3, true);
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
