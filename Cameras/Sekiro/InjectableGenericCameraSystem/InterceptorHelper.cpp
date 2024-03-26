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
		aobBlocks[UWKEY] = new AOBBlock(UWKEY, "F3 0F 58 4E 50 F3 0F 11 4E 50", 1);
		aobBlocks[PLAYERPOINTER_KEY] = new AOBBlock(PLAYERPOINTER_KEY, "38 | 48 8B 80 F8 1F 00 00 48 8B 48 28", 1);
		aobBlocks[ENTITY_KEY] = new AOBBlock(ENTITY_KEY, "F3 0F 10 83 ?? ?? ?? ?? F3 0F 59 83 ?? ?? ?? ?? F3 0F 59 D0", 1);
		aobBlocks[FPSUNLOCK] = new AOBBlock(FPSUNLOCK, "89 88 88 3C 4C 89 AB 70 02 00 00 E8 10 C9 7C FF", 1);
		aobBlocks[HUDTOGGLE] = new AOBBlock(HUDTOGGLE, "42 FF 14 C0 48 63 43 4C 89 43 48 44 38 73 60 74 2F 44 38 73 61 74 29 83 F8 11 77 18", 1); //4 byte nop

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
		GameImageHooker::setHook(aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY], 0x23, &_cameraStructInterceptionContinue, &cameraStructInterceptor);
	}


	void setPostCameraStructHooks(map<string, AOBBlock*> &aobBlocks)
	{
		GameImageHooker::setHook(aobBlocks[CAM_BLOCK1], 0x23, &_debugcamInterceptionContinue, &debugcaminterceptor);
		GameImageHooker::setHook(aobBlocks[DOF_KEY], 0x11, &_dofInterceptionContinue, &dofStruct);
		GameImageHooker::setHook(aobBlocks[TIMESTOP_KEY], 0x10, &_timestructinterceptionContinue, &timescaleinterceptor);
		GameImageHooker::setHook(aobBlocks[UWKEY], 0xE, &_fovinterceptContinue, &fovintercept);
		GameImageHooker::setHook(aobBlocks[PLAYERPOINTER_KEY], 0xF, &_playerpointerContinue, &playerpointerinterceptor);
		GameImageHooker::setHook(aobBlocks[ENTITY_KEY], 0x10, &_entitytimeContinue, &entitytimeinterceptor);
	}


	void toggleHud(map<string, AOBBlock*>& aobBlocks, bool hudVisible)
	{
		hudVisible ? Utils::SaveNOPReplace(aobBlocks[HUDTOGGLE], 4, false) : Utils::SaveNOPReplace(aobBlocks[HUDTOGGLE], 4, true);
	}

	//Placeholder code for if absolute addresses are required. Init variable in extern for use in ASM
	void getAbsoluteAddresses(map<string, AOBBlock*>& aobBlocks)
	{
		//var1 = Utils::calculateAbsoluteAddress(aobBlocks[KEYNAME], numberofbytes);
		//OverlayConsole::instance().logDebug("Absolute Address for Var1: %p", (void*)var1);
	}

	void cameraSetup(map<string, AOBBlock*>& aobBlocks, bool enabled, GameAddressData& addressData)
	{
		Utils::SaveNOPReplace(aobBlocks[FOV_INTERCEPT_KEY], 3, enabled);
		Utils::SaveNOPReplace(aobBlocks[FOV_INTERCEPT_DEBUG], 3, enabled);

		if (addressData.dofAddress == nullptr)
		{
			MessageHandler::logError("DOF Struct not found");
			return;
		}
		else
		{
			uint8_t* dof = reinterpret_cast<uint8_t*>(addressData.dofAddress + DOF_OFFSET);
			*dof = enabled ? (uint8_t)0 : (uint8_t)4;
		}

	}

	void toolsInit(map<string, AOBBlock*>& aobBlocks)
	{
		//Utils::SaveNOPReplace(aobBlocks[TIMESTOP_WRITE_KEY], 10, true);
		//Utils::SaveNOPReplace(aobBlocks[TIMESTOP_WRITE_KEY2], 10, true);
	}

	void handleSettings(map<string, AOBBlock*>& aobBlocks, GameAddressData& addressData)
	{
		//Ultrawide FOV Adjustment
		CameraManipulator::ultrawideFOV();
		//FPS Unlock
		BYTE fps120[8] = { 0x89, 0x88, 0x08, 0x3C, 0x4C, 0x89, 0xAB, 0x70 };
		bool enabled = Globals::instance().settings().fpsUnlockToggle;
		if (enabled || !enabled)
		{
			if (aobBlocks[FPSUNLOCK]->nopState2 == enabled) { return; }
			Utils::SaveBytesWrite(aobBlocks[FPSUNLOCK], 8, fps120, enabled);
		}
	}
}
