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
	void cameraWrite1Interceptor();
	void cameraStructInterceptor2();
	void cameraWrite2Interceptor();
	//void cameraWrite3Interceptor();
	//void cameraWrite4Interceptor();
	//void cameraWrite5Interceptor();
	//void resolutionScaleReadInterceptor();
	//void timestopReadInterceptor();
	//void displayTypeInterceptor();
	//void dofSelectorWriteInterceptor();
}

// external addresses used in asm.
extern "C" {
	LPBYTE _cameraStructInterceptionContinue = nullptr;
	LPBYTE _cameraStructInterception2Continue = nullptr;
	LPBYTE _cameraWrite1InterceptionContinue = nullptr;
	LPBYTE _cameraWrite2InterceptionContinue = nullptr;
	//LPBYTE _cameraWrite3InterceptionContinue = nullptr;
	//LPBYTE _cameraWrite4InterceptionContinue = nullptr;
	//LPBYTE _cameraWrite5InterceptionContinue = nullptr;
	//LPBYTE _resolutionScaleReadInterceptionContinue = nullptr;
	//LPBYTE _timestopReadInterceptionContinue = nullptr;
	//LPBYTE _displayTypeInterceptionContinue = nullptr;
	//LPBYTE _dofSelectorWriteInterceptionContinue = nullptr;
}

namespace IGCS::GameSpecific::InterceptorHelper
{
//std::vector<byteStorageStruct> storageVector;
	void initializeAOBBlocks(LPBYTE hostImageAddress, DWORD hostImageSize, map<string, AOBBlock*> &aobBlocks)
	{
		aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY] = new AOBBlock(CAMERA_ADDRESS_INTERCEPT_KEY, "48 8D 4C 24 ?? 48 8B F8 E8 ?? ?? ?? ?? | 0F 28 00 66 0F 7F 43 ?? 0F 28 48 ?? 66 0F 7F 4B ?? 0F 28 40 ?? 66 0F 7F 43 ?? 0F 57 C0 0F 28 4F ?? 66 0F 7F 4B ?? F3 0F 10 57 ?? 0F 2F D0", 1);	
		aobBlocks[CAMERA_WRITE1_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE1_INTERCEPT_KEY, "F3 0F 11 53 ?? 8B 47 ?? 89 43 ?? 0F 28 47 ?? 66 0F 7F 43 ?? 48 8B 5C 24 ?? 48 83 C4 ?? 5F C3", 1);
		aobBlocks[CAMERA_ADDRESS2_INTERCEPT_KEY] = new AOBBlock(CAMERA_ADDRESS2_INTERCEPT_KEY, "FF 90 ?? ?? ?? ?? 48 8B D0 48 8D 4C 24 ?? 48 8B F8", 1);
		aobBlocks[CAMERA_WRITE2_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE2_INTERCEPT_KEY, "0F 29 03 F3 0F 5C 4B ?? F3 0F 59 CF F3 0F58 4B ?? F3 0F 11 4B ??", 1);
		//aobBlocks[CAMERA_WRITE3_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE3_INTERCEPT_KEY, "45 0A 88 ?? ?? ?? ?? 66 41 0F 7F 40 ?? 66 41 0F 7F 08", 1);
		//aobBlocks[CAMERA_WRITE4_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE4_INTERCEPT_KEY, "66 41 0F 7F 40 ?? 66 41 0F 7F 80 ?? ?? ?? ?? 66 41 0F 7F 88 ?? ?? ?? ??", 1);
		//aobBlocks[CAMERA_WRITE5_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE5_INTERCEPT_KEY, "66 0F 7F 03 66 0F 7F 4B ?? 48 8B 5C 24 ??", 1);
		//aobBlocks[TIMESTOP_READ_INTERCEPT_KEY] = new AOBBlock(TIMESTOP_READ_INTERCEPT_KEY, "F3 0F 10 87 84 03 00 00 0F 2F C2 F3 0F 10 9F 80 03 00 00", 1);
		//aobBlocks[RESOLUTION_SCALE_INTERCEPT_KEY] = new AOBBlock(RESOLUTION_SCALE_INTERCEPT_KEY, "F3 0F 10 80 6C 12 00 00 48 8D 44 24 3C F3 41 0F 59 46 40", 1);
		//aobBlocks[DISPLAYTYPE_INTERCEPT_KEY] = new AOBBlock(DISPLAYTYPE_INTERCEPT_KEY, "44 0F 29 44 24 40 44 0F 29 4C 24 30 F3 0F 11 45 10 F3 0F 11 4D 14", 1);
		//aobBlocks[DOF_SELECTOR_WRITE_INTERCEPT_KEY] = new AOBBlock(DOF_SELECTOR_WRITE_INTERCEPT_KEY, "89 51 4C 85 D2 74 0E 83 EA 01 74 09 83 FA 01 75 08 88 51 50 C3", 1);
		aobBlocks[QUATERNION_WRITE2_KEY] = new AOBBlock(QUATERNION_WRITE2_KEY, "FF 50 ?? 48 8B D3 48 8D 4C 24 ?? | E8 ?? ?? ?? ?? 48 83 C4 ??", 1);
		//aobBlocks[HUD_TOGGLE_ADDRESS_KEY] = new AOBBlock(HUD_TOGGLE_ADDRESS_KEY, "48 8B 3D | ?? ?? ?? ?? 80 7F 08 00 66 C7 47 0A 00 00", 1);

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
		GameImageHooker::setHook(aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY], 0x2B, &_cameraStructInterceptionContinue, &cameraStructInterceptor);
		GameImageHooker::setHook(aobBlocks[CAMERA_ADDRESS2_INTERCEPT_KEY], 0x0E, &_cameraStructInterception2Continue, &cameraStructInterceptor2);

	}

	void setPostCameraStructHooks(map<string, AOBBlock*> &aobBlocks)
	{
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE1_INTERCEPT_KEY], 0x0F, &_cameraWrite1InterceptionContinue, &cameraWrite1Interceptor);
		//GameImageHooker::setHook(aobBlocks[CAMERA_WRITE2_INTERCEPT_KEY], 0x11, &_cameraWrite2InterceptionContinue, &cameraWrite2Interceptor);
		//GameImageHooker::setHook(aobBlocks[CAMERA_WRITE3_INTERCEPT_KEY], 0x12, &_cameraWrite3InterceptionContinue, &cameraWrite3Interceptor);
		//GameImageHooker::setHook(aobBlocks[CAMERA_WRITE4_INTERCEPT_KEY], 0x0F, &_cameraWrite4InterceptionContinue, &cameraWrite4Interceptor);
		//GameImageHooker::setHook(aobBlocks[CAMERA_WRITE5_INTERCEPT_KEY], 0x0E, &_cameraWrite5InterceptionContinue, &cameraWrite5Interceptor);
		//GameImageHooker::setHook(aobBlocks[TIMESTOP_READ_INTERCEPT_KEY], 0x13, &_timestopReadInterceptionContinue, &timestopReadInterceptor);
		//GameImageHooker::setHook(aobBlocks[RESOLUTION_SCALE_INTERCEPT_KEY], 0x13, &_resolutionScaleReadInterceptionContinue, &resolutionScaleReadInterceptor);
		//GameImageHooker::setHook(aobBlocks[DISPLAYTYPE_INTERCEPT_KEY], 0x11, &_displayTypeInterceptionContinue, &displayTypeInterceptor);
		//GameImageHooker::setHook(aobBlocks[DOF_SELECTOR_WRITE_INTERCEPT_KEY], 0x19, &_dofSelectorWriteInterceptionContinue, &dofSelectorWriteInterceptor);
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

	//// if hideHud is false, we'll place a 1 at the flag position which signals that the rendering of the hud was successful, which shows the hud.
	//// otherwise we'll place a 0 there which makes the hud disappear.
	//void toggleHud(map<string, AOBBlock*> &aobBlocks, bool hideHud)
	//{
	//	//DevilMayCry5.exe+18679B15 - 80 78 08 00           - cmp byte ptr [rax+08],00 { 0 }
	//	//DevilMayCry5.exe+18679B19 - 0F84 E3010000         - je DevilMayCry5.exe+18679D02
	//	//DevilMayCry5.exe+18679B1F - 48 89 7C 24 30        - mov [rsp+30],rdi
	//	//DevilMayCry5.exe+18679B24 - 48 8B 3D 5D214CEF     - mov rdi,[DevilMayCry5.exe+7B3BC88] <<< READ address at this address, add 8. Write 0 to toggle hud OFF, 1 for normal operation.
	//	//DevilMayCry5.exe+18679B2B - 80 7F 08 00           - cmp byte ptr [rdi+08],00 { 0 }
	//	//DevilMayCry5.exe+18679B2F - 66 C7 47 0A 0000      - mov word ptr [rdi+0A],0000 { 0 }
	//	//DevilMayCry5.exe+18679B35 - 0F84 9E010000         - je DevilMayCry5.exe+18679CD9
	//	//DevilMayCry5.exe+18679B3B - 48 8B 05 C6C93DEF     - mov rax,[DevilMayCry5.exe+7A56508] { (176) }
	//	//DevilMayCry5.exe+18679B42 - 48 8B 48 48           - mov rcx,[rax+48]
	//	//DevilMayCry5.exe+18679B46 - 48 85 C9              - test rcx,rcx
	//	// Obtained through:
	//	//DevilMayCry5.exe+185ADDE8 - 0F1F 84 00 00000000   - nop [rax+rax+00000000]
	//	//DevilMayCry5.exe+185ADDF0 - 48 8B 05 91DE58EF     - mov rax,[DevilMayCry5.exe+7B3BC88] <<< READ address at this address, add 8. Write 0 to toggle hud OFF, 1 for normal operation.
	//	//DevilMayCry5.exe+185ADDF7 - 0FB6 40 08            - movzx eax,byte ptr [rax+08]
	//	//DevilMayCry5.exe+185ADDFB - C3                    - ret 
	//	//DevilMayCry5.exe+185ADDFC - 00 00                 - add [rax],al

	//	LPBYTE drawComponentStructAddress = (LPBYTE)*(__int64*)Utils::calculateAbsoluteAddress(aobBlocks[HUD_TOGGLE_ADDRESS_KEY], 4); //DevilMayCry5.exe+18679B24 - 48 8B 3D 5D214CEF     - mov rdi,[DevilMayCry5.exe+7B3BC88]
	//	BYTE* drawComponentResultAddress = reinterpret_cast<BYTE*>(drawComponentStructAddress + HUD_TOGGLE_OFFSET_IN_STRUCT);
	//	*drawComponentResultAddress = hideHud ? (BYTE)0 : (BYTE)1;
	//}
}
