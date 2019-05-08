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

using namespace std;

//--------------------------------------------------------------------------------------------------------------------------------
// external asm functions
extern "C" {
	void cameraStructInterceptor();
	void cameraWrite1Interceptor();
	//void cameraWrite2Interceptor();
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
	LPBYTE _cameraWrite1InterceptionContinue = nullptr;
	//LPBYTE _cameraWrite2InterceptionContinue = nullptr;
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
	void initializeAOBBlocks(LPBYTE hostImageAddress, DWORD hostImageSize, map<string, AOBBlock*> &aobBlocks)
	{
		aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY] = new AOBBlock(CAMERA_ADDRESS_INTERCEPT_KEY, "48 8D 4C 24 ?? 48 8B F8 E8 ?? ?? ?? ?? | 0F 28 00 66 0F 7F 43 ?? 0F 28 48 ?? 66 0F 7F 4B ?? 0F 28 40 ?? 66 0F 7F 43 ?? 0F 57 C0 0F 28 4F ?? 66 0F 7F 4B ?? F3 0F 10 57 ?? 0F 2F D0", 1);	
		aobBlocks[CAMERA_WRITE1_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE1_INTERCEPT_KEY, "F3 0F 11 53 ?? 8B 47 ?? 89 43 ?? 0F 28 47 ?? 66 0F 7F 43 ?? 48 8B 5C 24 ?? 48 83 C4 ?? 5F C3", 1);
		//aobBlocks[CAMERA_WRITE2_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE2_INTERCEPT_KEY, "0F 29 5C 24 ?? 0F 56 D0 41 0F 28 C1 0F 29 56 ??", 1);
		//aobBlocks[CAMERA_WRITE3_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE3_INTERCEPT_KEY, "45 0A 88 ?? ?? ?? ?? 66 41 0F 7F 40 ?? 66 41 0F 7F 08", 1);
		//aobBlocks[CAMERA_WRITE4_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE4_INTERCEPT_KEY, "66 41 0F 7F 40 ?? 66 41 0F 7F 80 ?? ?? ?? ?? 66 41 0F 7F 88 ?? ?? ?? ??", 1);
		//aobBlocks[CAMERA_WRITE5_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE5_INTERCEPT_KEY, "66 0F 7F 03 66 0F 7F 4B ?? 48 8B 5C 24 ??", 1);
		//aobBlocks[TIMESTOP_READ_INTERCEPT_KEY] = new AOBBlock(TIMESTOP_READ_INTERCEPT_KEY, "F3 0F 10 87 84 03 00 00 0F 2F C2 F3 0F 10 9F 80 03 00 00", 1);
		//aobBlocks[RESOLUTION_SCALE_INTERCEPT_KEY] = new AOBBlock(RESOLUTION_SCALE_INTERCEPT_KEY, "F3 0F 10 80 6C 12 00 00 48 8D 44 24 3C F3 41 0F 59 46 40", 1);
		//aobBlocks[DISPLAYTYPE_INTERCEPT_KEY] = new AOBBlock(DISPLAYTYPE_INTERCEPT_KEY, "44 0F 29 44 24 40 44 0F 29 4C 24 30 F3 0F 11 45 10 F3 0F 11 4D 14", 1);
		//aobBlocks[DOF_SELECTOR_WRITE_INTERCEPT_KEY] = new AOBBlock(DOF_SELECTOR_WRITE_INTERCEPT_KEY, "89 51 4C 85 D2 74 0E 83 EA 01 74 09 83 FA 01 75 08 88 51 50 C3", 1);
		//aobBlocks[DOF_FAR_BLUR_STRENGTH_WRITE_KEY] = new AOBBlock(DOF_FAR_BLUR_STRENGTH_WRITE_KEY, "8B 00 89 41 74 48 8D 44 24 10 48 8D 4C 24 08 F3 0F 11 4C 24 10", 1);
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
	}


	void setPostCameraStructHooks(map<string, AOBBlock*> &aobBlocks)
	{
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE1_INTERCEPT_KEY], 0x0F, &_cameraWrite1InterceptionContinue, &cameraWrite1Interceptor);
		//GameImageHooker::setHook(aobBlocks[CAMERA_WRITE2_INTERCEPT_KEY], 0x10, &_cameraWrite2InterceptionContinue, &cameraWrite2Interceptor);
		//GameImageHooker::setHook(aobBlocks[CAMERA_WRITE3_INTERCEPT_KEY], 0x12, &_cameraWrite3InterceptionContinue, &cameraWrite3Interceptor);
		//GameImageHooker::setHook(aobBlocks[CAMERA_WRITE4_INTERCEPT_KEY], 0x0F, &_cameraWrite4InterceptionContinue, &cameraWrite4Interceptor);
		//GameImageHooker::setHook(aobBlocks[CAMERA_WRITE5_INTERCEPT_KEY], 0x0E, &_cameraWrite5InterceptionContinue, &cameraWrite5Interceptor);
		//GameImageHooker::setHook(aobBlocks[TIMESTOP_READ_INTERCEPT_KEY], 0x13, &_timestopReadInterceptionContinue, &timestopReadInterceptor);
		//GameImageHooker::setHook(aobBlocks[RESOLUTION_SCALE_INTERCEPT_KEY], 0x13, &_resolutionScaleReadInterceptionContinue, &resolutionScaleReadInterceptor);
		//GameImageHooker::setHook(aobBlocks[DISPLAYTYPE_INTERCEPT_KEY], 0x11, &_displayTypeInterceptionContinue, &displayTypeInterceptor);
		//GameImageHooker::setHook(aobBlocks[DOF_SELECTOR_WRITE_INTERCEPT_KEY], 0x19, &_dofSelectorWriteInterceptionContinue, &dofSelectorWriteInterceptor);
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


	//// Toggles the in-game dof. it selects dof '1' and sets far blur to 0. 
	//void toggleInGameDoFOff(map<string, AOBBlock*> &aobBlocks, bool switchOff)
	//{
	//	if (switchOff)
	//	{
	//		// first select the dof, type 1. 
	//		CameraManipulator::selectDoF(1);

	//		// write xor eax, eax at the far blur strength value read, so it's always 0
	//		//DevilMayCry5.exe+194C767A - 48 8D 4C 24 08        - lea rcx,[rsp+08]
	//		//DevilMayCry5.exe+194C767F - 8B 00                 - mov eax,[rax]
	//		//DevilMayCry5.exe+194C7681 - 89 82 C4010000        - mov [rdx+000001C4],eax
	//		//DevilMayCry5.exe+194C7687 - 48 8D 44 24 10        - lea rax,[rsp+10]
	//		//DevilMayCry5.exe+194C768C - 48 0F46 C1            - cmovbe rax,rcx
	//		//DevilMayCry5.exe+194C7690 - F3 0F11 4C 24 10      - movss [rsp+10],xmm1
	//		//DevilMayCry5.exe+194C7696 - 49 8B 48 38           - mov rcx,[r8+38]
	//		//DevilMayCry5.exe+194C769A - C7 44 24 08 17B7D138  - mov [rsp+08],38D1B717 { (0) }
	//		//DevilMayCry5.exe+194C76A2 - 8B 00                 - mov eax,[rax]						<< REPLACE WITH XOR eax, eax (31 C0 xor eax, eax)
	//		//DevilMayCry5.exe+194C76A4 - 89 41 74              - mov [rcx+74],eax					<< FAR BLUR WRITE. Set to 0 to make sure there's no dof
	//		//DevilMayCry5.exe+194C76A7 - 48 8D 44 24 10        - lea rax,[rsp+10]
	//		//DevilMayCry5.exe+194C76AC - 48 8D 4C 24 08        - lea rcx,[rsp+08]
	//		//DevilMayCry5.exe+194C76B1 - F3 0F11 4C 24 10      - movss [rsp+10],xmm1
	//		//DevilMayCry5.exe+194C76B7 - 48 0F46 C1            - cmovbe rax,rcx
	//		//DevilMayCry5.exe+194C76BB - C7 44 24 08 17B7D138  - mov [rsp+08],38D1B717 { (0) }
	//		//DevilMayCry5.exe+194C76C3 - 49 8B 48 40           - mov rcx,[r8+40]
	//		//DevilMayCry5.exe+194C76C7 - 8B 00                 - mov eax,[rax]
	//		//DevilMayCry5.exe+194C76C9 - 89 81 9C000000        - mov [rcx+0000009C],eax
	//		//DevilMayCry5.exe+194C76CF - C3                    - ret 
	//		BYTE statementBytes[2] = { 0x31, 0xC0 };			//DevilMayCry5.exe+194C76A2 - 8B 00 - mov eax,[rax] -> xor eax, eax
	//		GameImageHooker::writeRange(aobBlocks[DOF_FAR_BLUR_STRENGTH_WRITE_KEY], statementBytes, 2);
	//	}
	//	else
	//	{
	//		// only restore the xor eax, eax with the original statement. The camera disable will enable the right dof as it will enable the write to the dof selector.
	//		BYTE statementBytes[2] = { 0x8B, 0x00 };			//DevilMayCry5.exe+194C76A2 - 8B 00 - mov eax,[rax]
	//		GameImageHooker::writeRange(aobBlocks[DOF_FAR_BLUR_STRENGTH_WRITE_KEY], statementBytes, 2);
	//	}
	//}
}
