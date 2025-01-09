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
	void ARread();
	void BLOOMinterceptor();
	void timestopInterceptor();
}

// external addresses used in asm.
extern "C" {
	LPBYTE _cameraStructInterceptionContinue = nullptr;
	LPBYTE _ARInterceptionContinue = nullptr;
	LPBYTE _bloomstructinterceptionContinue = nullptr;
	LPBYTE  _timestopInterceptionContinue = nullptr;
}


namespace IGCS::GameSpecific::InterceptorHelper
{
	void initializeAOBBlocks(LPBYTE hostImageAddress, DWORD hostImageSize, map<string, AOBBlock*> &aobBlocks)
	{
		aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY] = new AOBBlock(CAMERA_ADDRESS_INTERCEPT_KEY, "8B 01 89 02 8B 41 04 89 42 04 8B 41 08 89 42 08 8B 49 0C 89 4A 0C 8B 55 F0 83 C2 10 89 55 FC 8B 45 08 83 C0 10 8B 4D FC 8B 10 89 11 8B 50 04 89 51 04 8B 50 08 89 51 08 8B 40 0C 89 41 0C 8B 4D F0 83 C1 20 89 4D F8 8B 55 08 83 C2 20 8B 45 F8 8B 0A 89 08 8B 4A 04 89 48 04 8B 4A 08 89 48 08 8B 52 0C 89 50 0C 8B 45 F0 83 C0 30 89 45 F4 8B 4D 08 83 C1 30 8B 55 F4 8B 01 89 02 8B 41 04 89 42 04 8B 41 08 89 42 08 8B 49 0C 89 4A 0C 8B 55 EC C6 82 AD 00 00 00 01 8B E5 5D C2 04 00 CC CC CC CC CC CC CC CC CC CC 55 8B EC 83 EC 14 89 4D EC 8B 45 EC 83 C0 48 89 45 F0 8B 4D 08 8B 55 F0 8B 01 89 02 8B 41 04 89 42 04 8B 41 08 89 42 08", 1);
		aobBlocks[HFOV_INTERCEPT_KEY] = new AOBBlock(HFOV_INTERCEPT_KEY, "48 89 45 F0 8B 4D 08 8B 55 F0 8B 01 | 89 02 8B 41 04", 1);
		aobBlocks[VFOV_INTERCEPT_KEY] = new AOBBlock(VFOV_INTERCEPT_KEY, "89 51 04 8B 50 08 89 51 08 8B 40 0C 89 41 0C 8B 4D F0 83 C1 20 89 4D F8 8B 55 08 83 C2 20 8B 45 F8 8B 0A 89 08 8B 4A 04 89 48 04 8B 4A 08 89 48 08 8B 52 0C 89 50 0C 8B 45 F0 83 C0 30 89 45 F4 8B 4D 08 83 C1 30 8B 55 F4 8B 01 89 02 8B 41 04 89 42 04 8B 41 08 89 42 08 8B 49 0C 89 4A 0C 8B 55 EC C6 82 AD 00 00 00 01 8B E5 5D C2 04 00 CC CC CC CC CC CC CC CC CC CC 55 8B EC 51 89 4D FC 8B 45 FC D9 80 B0 00 00 00 8B E5 5D C3 CC CC", 1);
		aobBlocks[HUDTOGGLE] = new AOBBlock(HUDTOGGLE, "FF D0 8B 4D FC E8 74", 1);
		//aobBlocks[TIMESTOP_KEY] = new AOBBlock(TIMESTOP_KEY, "E8 B9 12 00 00 83", 1); //old timestop
		aobBlocks[TIMESTOP_KEY] = new AOBBlock(TIMESTOP_KEY, "D9 9D ?? ?? ?? ?? F3 0F 2A 45 ??", 1);
		aobBlocks[DOF_KEY] = new AOBBlock(DOF_KEY, "74 66 8B 94 24 D4 06 00 00 89 94 24 B8 00 00 00", 1);
		aobBlocks[AR_KEY] = new AOBBlock(AR_KEY, "D9 81 FC 03 00 00 D9 1C 24 F3 0F 10 84 24 8C 01 00 00", 1);
		aobBlocks[PP_KEY] = new AOBBlock(PP_KEY, "74 66 8B 94 24 D4 06 00 00 89 94 24 98 02 00 00", 1);
		aobBlocks[BLOOM_KEY] = new AOBBlock(BLOOM_KEY, "89 8D E4 FE FF FF 8B 85 E4 FE FF FF F3 0F 10 40 10", 1);
		aobBlocks[BLOOM_WRITE] = new AOBBlock(BLOOM_WRITE, "F3 0F 11 41 10 8B E5 5D C2 04 00 CC CC CC 55", 1);

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
		GameImageHooker::setHook(aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY], 0x8E, &_cameraStructInterceptionContinue, &cameraStructInterceptor);
	}


	void setPostCameraStructHooks(map<string, AOBBlock*> &aobBlocks)
	{
		GameImageHooker::setHook(aobBlocks[AR_KEY], 0x12, &_ARInterceptionContinue, &ARread);
		GameImageHooker::setHook(aobBlocks[TIMESTOP_KEY], 0x13, &_timestopInterceptionContinue, &timestopInterceptor);
		GameImageHooker::setHook(aobBlocks[BLOOM_KEY], 0x11, &_bloomstructinterceptionContinue, &BLOOMinterceptor);
	}


	void toggleHud(map<string, AOBBlock*>& aobBlocks, bool hudVisible)
	{
		hudVisible ? Utils::SaveNOPReplace(aobBlocks[HUDTOGGLE], 2, false) : Utils::SaveNOPReplace(aobBlocks[HUDTOGGLE], 2, true);
	}

	//Placeholder code for if absolute addresses are required. Init variable in extern for use in ASM
	void getAbsoluteAddresses(map<string, AOBBlock*>& aobBlocks)
	{
		//var1 = Utils::calculateAbsoluteAddress(aobBlocks[KEYNAME], numberofbytes);
		//OverlayConsole::instance().logDebug("Absolute Address for Var1: %p", (void*)var1);
	}

	void cameraSetup(map<string, AOBBlock*>& aobBlocks, bool enabled, GameAddressData& addressData, GameCameraData cacheddata)
	{
		BYTE jumpbyte = { 0xEB };
		Utils::SaveNOPReplace(aobBlocks[BLOOM_WRITE], 5, enabled);
		Utils::SaveNOPReplace(aobBlocks[HFOV_INTERCEPT_KEY], 2, enabled);
		Utils::SaveNOPReplace(aobBlocks[VFOV_INTERCEPT_KEY], 3, enabled);
		Utils::SaveBytesWrite(aobBlocks[DOF_KEY], 1, &jumpbyte, enabled);
		CameraManipulator::toggleBloom(enabled, cacheddata);
	}

	void toolsInit(map<string, AOBBlock*>& aobBlocks)
	{
		//Utils::SaveNOPReplace(aobBlocks[TIMESTOP_WRITE_KEY], 10, true);
		//Utils::SaveNOPReplace(aobBlocks[TIMESTOP_WRITE_KEY2], 10, true);
	}

	void handleSettings(map<string, AOBBlock*>& aobBlocks, GameAddressData& addressData)
	{
		BYTE jumpbyte = { 0xEB };
		bool enabled = Globals::instance().settings().tonemapToggle;
		if (enabled || !enabled)
		{
			if (aobBlocks[PP_KEY]->nopState2 == enabled) { return; }
			Utils::SaveBytesWrite(aobBlocks[PP_KEY], 1, &jumpbyte, enabled);
		}
	}
}
