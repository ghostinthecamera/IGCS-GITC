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

//BYTE* byteStorage = nullptr

using namespace std;

//--------------------------------------------------------------------------------------------------------------------------------
// external asm functions
extern "C" {
	void cameraStructInterceptor();
	void cameraWrite1();
	void cameraWrite2();
	void cameraWrite3();
	void cameraWrite4();
	void cameraWrite5();
	void cameraWrite6();
	void cameraWrite7();
	void cameraWrite8();
	void cameraWrite9();
	void cameraWrite10();
	void cameraWrite18();
	void cameraWrite19();
	void cameraWrite20();
	void cameraWrite21();
	void cameraWrite22();
	void cameraWrite23();
	void cameraWrite24();
	void cameraWrite25();
	void fovWrite();
	void fovWrite2();
	void fovWrite3();
	void nearplane();
	void nearplane2();
}

// external addresses used in asm.
extern "C" {
	LPBYTE _cameraStructInterceptionContinue = nullptr;
	LPBYTE _cameraWrite1Continue = nullptr;
	LPBYTE _cameraWrite2Continue = nullptr;
	LPBYTE _cameraWrite3Continue = nullptr;
	LPBYTE _cameraWrite4Continue = nullptr;
	LPBYTE _cameraWrite5Continue = nullptr;
	LPBYTE _cameraWrite6Continue = nullptr;
	LPBYTE _cameraWrite7Continue = nullptr;
	LPBYTE _cameraWrite8Continue = nullptr;
	LPBYTE _cameraWrite9Continue = nullptr;
	LPBYTE _cameraWrite10Continue = nullptr;
	LPBYTE _cameraWrite18Continue = nullptr;
	LPBYTE _cameraWrite19Continue = nullptr;
	LPBYTE _cameraWrite20Continue = nullptr;
	LPBYTE _cameraWrite21Continue = nullptr;
	LPBYTE _cameraWrite22Continue = nullptr;
	LPBYTE _cameraWrite23Continue = nullptr;
	LPBYTE _cameraWrite24Continue = nullptr;
	LPBYTE _cameraWrite25Continue = nullptr;
	LPBYTE _fovContinue = nullptr;
	LPBYTE _fov2Continue = nullptr;
	LPBYTE _fov3Continue = nullptr;
	LPBYTE _fovabsoluteAddress = nullptr;
	LPBYTE _NPabsoluteAddress = nullptr;
	LPBYTE _nearplane1Continue = nullptr;
	LPBYTE _nearplane2Continue = nullptr;
}

namespace IGCS::GameSpecific::InterceptorHelper
{
//std::vector<byteStorageStruct> storageVector;
	void initializeAOBBlocks(LPBYTE hostImageAddress, DWORD hostImageSize, map<string, AOBBlock*> &aobBlocks)
	{
		aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY] = new AOBBlock(CAMERA_ADDRESS_INTERCEPT_KEY, "4C 8B A1 F0 38 04 00", 1);	
		aobBlocks[CAMERA_WRITE1_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE1_INTERCEPT_KEY, "0F C6 D2 27 F3 0F 10 D1 0F C6 D2 27 0F 29 12 48", 1);
		aobBlocks[CAMERA_WRITE2_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE2_INTERCEPT_KEY, "0F 29 03 F3 0F 5C 4B 70", 1);
		aobBlocks[CAMERA_WRITE3_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE3_INTERCEPT_KEY, "0F C6 D2 27 0F 29 56 50 0F", 1);
		aobBlocks[CAMERA_WRITE4_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE4_INTERCEPT_KEY, "F3 0F 10 5C 24 58 0F 14 D8 0F", 1);
		aobBlocks[CAMERA_WRITE5_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE5_INTERCEPT_KEY, "0F 29 43 10 0F 5C 73 50", 1);
		aobBlocks[CAMERA_WRITE6_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE6_INTERCEPT_KEY, "66 41 0F 7F 00 66 41 0F 7F 48 10", 1);
		aobBlocks[CAMERA_WRITE7_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE7_INTERCEPT_KEY, "0F 29 03 41 0F 58 CD 0F 29 5B 10 66 41 0F 6F F4", 1);
		aobBlocks[CAMERA_WRITE8_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE8_INTERCEPT_KEY, "0F 28 00 0F 28 CA 0F 58 47 10 0F 55 C8 0F 29 4F 10", 1);
		aobBlocks[CAMERA_WRITE9_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE9_INTERCEPT_KEY, "0F 56 C1 41 0F 28 D1 0F 29 46 10 41 0F 28 C9", 1);
		aobBlocks[CAMERA_WRITE10_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE10_INTERCEPT_KEY, "45 0F 28 53 D0 45 0F 28 5B C0 66 0F 7F 07", 1);
		aobBlocks[CAMERA_WRITE11_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE11_INTERCEPT_KEY, "66 0F 7F 06 44 0F 28 B4 24 60 06 00 00", 1);
		aobBlocks[CAMERA_WRITE12_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE12_INTERCEPT_KEY, "66 0F 7F 07 44 0F 28 74 24 70", 1);
		aobBlocks[CAMERA_WRITE13_SERVICEAREA_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE13_SERVICEAREA_INTERCEPT_KEY, "66 0F 7F 06 80 BF 5C 0A 00 00 00", 1);
		aobBlocks[CAMERA_WRITE14_SERVICEAREA_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE14_SERVICEAREA_INTERCEPT_KEY, "66 0F 7F 06 49 83 7E 10 00", 1);
		aobBlocks[CAMERA_WRITE15_SERVICEAREA_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE15_SERVICEAREA_INTERCEPT_KEY, "66 0F 7F 06 80 BF 50 08 00 00 00", 1);
		aobBlocks[CAMERA_WRITE16_SERVICEAREA_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE16_SERVICEAREA_INTERCEPT_KEY, "66 0F 7F 07 48 8B D7", 1);
		aobBlocks[CAMERA_WRITE17_SERVICEAREA_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE17_SERVICEAREA_INTERCEPT_KEY, "0F 29 76 10 8B 47 18", 1);
		aobBlocks[CAMERA_WRITE26_SERVICEAREA_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE26_SERVICEAREA_INTERCEPT_KEY, "0F 29 47 10 48 8B 9C 24 D0 00 00 00", 1);
		aobBlocks[CAMERA_WRITE18_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE18_INTERCEPT_KEY, "0F 29 07 F3 0F 10 44 24 54", 1);
		aobBlocks[CAMERA_WRITE19_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE19_INTERCEPT_KEY, "0F 28 00 66 0F 7F 07 0F 2F", 1);
		aobBlocks[CAMERA_WRITE20_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE20_INTERCEPT_KEY, "0F 55 C8 0F 28 C3 0F 58 4B", 1);
		aobBlocks[CAMERA_WRITE21_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE21_INTERCEPT_KEY, "0F 28 D3 0F 55 D0 0F 29 57", 1);
		aobBlocks[CAMERA_WRITE22_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE22_INTERCEPT_KEY, "F3 0F 10 C4 F3 0F 10 E8 0F C6 ED E1", 1);
		aobBlocks[CAMERA_WRITE23_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE23_INTERCEPT_KEY, "0F 55 CA 0F 58 4F 10 0F 55 C1 0F 29 47 10 E8", 1);
		aobBlocks[CAMERA_WRITE24_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE24_INTERCEPT_KEY, "0F 28 00 48 8B BC 24 38 01 00 00", 1);
		aobBlocks[CAMERA_WRITE25_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE25_INTERCEPT_KEY, "0F 28 BC 24 A0 00 00 00 0F 28 00", 1);
		aobBlocks[FOV1_KEY] = new AOBBlock(FOV1_KEY, "F3 0F 11 4F 70 48 8B 5C 24 30", 1);
		aobBlocks[FOV2_KEY] = new AOBBlock(FOV2_KEY, "F3 44 0F 11 75 70 44 0F 28 B4 24 90 00 00 00", 1);
		aobBlocks[FOV3_KEY] = new AOBBlock(FOV3_KEY, "48 8B CB F3 0F 11 47 70 8B 43 18 89 47 78", 1);
		aobBlocks[FOV_ABS_KEY] = new AOBBlock(FOV_ABS_KEY, "F3 0F 59 0D | 00 9F 97 00", 1);
		aobBlocks[FOV4_KEY] = new AOBBlock(FOV4_KEY, "0F B6 44 24 68 0F 5B C9", 1);
		aobBlocks[FOV5_KEY] = new AOBBlock(FOV5_KEY, "0F 29 03 F3 0F 5C 4B 70", 1);
		aobBlocks[NEARPLANE1_KEY] = new AOBBlock(NEARPLANE1_KEY, "66 0F 6E C0 0F 5B C0 F3 0F 11 4B 78", 1);
		aobBlocks[NEARPLANE2_KEY] = new AOBBlock(NEARPLANE2_KEY, "F3 0F 11 4B 78 F3 0F 10 83 AC 00 00 00", 1);
		aobBlocks[NP1_ABS_KEY] = new AOBBlock(NP1_ABS_KEY, "F3 0F 59 05 | CC 13 80 00", 1);

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
		GameImageHooker::setHook(aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY], 0x12, &_cameraStructInterceptionContinue, &cameraStructInterceptor);

	}

	void setPostCameraStructHooks(map<string, AOBBlock*> &aobBlocks)
	{
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE1_INTERCEPT_KEY], 0x0F, &_cameraWrite1Continue, &cameraWrite1);
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE2_INTERCEPT_KEY], 0x11, &_cameraWrite2Continue, &cameraWrite2);
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE3_INTERCEPT_KEY], 0x13, &_cameraWrite3Continue, &cameraWrite3);
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE4_INTERCEPT_KEY], 0x12, &_cameraWrite4Continue, &cameraWrite4);
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE5_INTERCEPT_KEY], 0x0F, &_cameraWrite5Continue, &cameraWrite5);
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE6_INTERCEPT_KEY], 0x0E, &_cameraWrite6Continue, &cameraWrite6);
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE7_INTERCEPT_KEY], 0x10, &_cameraWrite7Continue, &cameraWrite7);
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE8_INTERCEPT_KEY], 0x10, &_cameraWrite8Continue, &cameraWrite8);
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE9_INTERCEPT_KEY], 0x10, &_cameraWrite9Continue, &cameraWrite9);
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE10_INTERCEPT_KEY], 0x0E, &_cameraWrite10Continue, &cameraWrite10);
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE18_INTERCEPT_KEY], 0x10, &_cameraWrite18Continue, &cameraWrite18);
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE19_INTERCEPT_KEY], 0x0E, &_cameraWrite19Continue, &cameraWrite19);
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE20_INTERCEPT_KEY], 0x11, &_cameraWrite20Continue, &cameraWrite20);
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE21_INTERCEPT_KEY], 0x11, &_cameraWrite21Continue, &cameraWrite21);
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE22_INTERCEPT_KEY], 0x10, &_cameraWrite22Continue, &cameraWrite22);
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE23_INTERCEPT_KEY], 0x0E, &_cameraWrite23Continue, &cameraWrite23);
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE24_INTERCEPT_KEY], 0x10, &_cameraWrite24Continue, &cameraWrite24);
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE25_INTERCEPT_KEY], 0x0F, &_cameraWrite25Continue, &cameraWrite25);
		GameImageHooker::setHook(aobBlocks[FOV3_KEY], 0x0E, &_fovContinue, &fovWrite);
		GameImageHooker::setHook(aobBlocks[FOV4_KEY], 0x15, &_fov2Continue, &fovWrite2);
		GameImageHooker::setHook(aobBlocks[FOV5_KEY], 0x16, &_fov3Continue, &fovWrite3);
		GameImageHooker::setHook(aobBlocks[NEARPLANE1_KEY], 0x14, &_nearplane1Continue, &nearplane);
		GameImageHooker::setHook(aobBlocks[NEARPLANE2_KEY], 0x12, &_nearplane2Continue, &nearplane2);
	}

	void getAbsoluteAddresses(map<string, AOBBlock*>& aobBlocks)
	{
		_fovabsoluteAddress = Utils::calculateAbsoluteAddress(aobBlocks[FOV_ABS_KEY], 4);
		_NPabsoluteAddress = Utils::calculateAbsoluteAddress(aobBlocks[NP1_ABS_KEY], 4);

		OverlayConsole::instance().logDebug("Absolute Address of FOV Absolute Address: %p", (void*)_fovabsoluteAddress);
		OverlayConsole::instance().logDebug("Absolute Address of Near Plane Absolute Address: %p", (void*)_NPabsoluteAddress);
	}
	// Reads and stores x number of bytes, and replaces them with NOPS when activated. Returns bytes when deactivated.
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
}
