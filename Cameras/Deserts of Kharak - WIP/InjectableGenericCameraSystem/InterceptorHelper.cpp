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
	/*void cameraWrite1Interceptor();
	void cameraWrite2Interceptor();
	void fovReadInterceptor();
	void resolutionScaleReadInterceptor();
	void todWriteInterceptor();
	void timestopReadInterceptor();
	void fogReadInterceptor();*/
}

// external addresses used in asm.
extern "C" {
	LPBYTE _cameraStructInterceptionContinue = nullptr;
	/*LPBYTE _cameraWrite1InterceptionContinue = nullptr;
	LPBYTE _cameraWrite2InterceptionContinue = nullptr;
	LPBYTE _fovReadInterceptionContinue = nullptr;
	LPBYTE _resolutionScaleReadInterceptionContinue = nullptr;
	LPBYTE _todWriteInterceptionContinue = nullptr;
	LPBYTE _timestopReadInterceptionContinue = nullptr;
	LPBYTE _fogReadInterceptionContinue = nullptr;*/
}


namespace IGCS::GameSpecific::InterceptorHelper
{
	void initializeAOBBlocks(LPBYTE hostImageAddress, DWORD hostImageSize, map<string, AOBBlock*> &aobBlocks)
	{
		aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY] = new AOBBlock(CAMERA_ADDRESS_INTERCEPT_KEY, "F3 0F 10 83 84 02 00 00", 1);	
		aobBlocks[CAMERA_WRITE1_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE1_INTERCEPT_KEY, "E8 66 6A E6 FF 48 83 C4 28 C3", 1);
		aobBlocks[CAMERA_WRITE2_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE2_INTERCEPT_KEY, "E8 16 68 E6 FF 48 83 C4 28 C3", 1);
		/*aobBlocks[TIMESTOP_READ_INTERCEPT_KEY] = new AOBBlock(TIMESTOP_READ_INTERCEPT_KEY, "48 89 54 24 28 48 8B 57 18 83 E1 10 4C 31 C1 41 0F 29 7B A8", 1);
		aobBlocks[TOD_WRITE_INTERCEPT_KEY] = new AOBBlock(TOD_WRITE_INTERCEPT_KEY, "0F 2F D1 F3 0F 11 12 72 ?? F3 0F 5C D1", 1);
		aobBlocks[RESOLUTION_SCALE_INTERCEPT_KEY] = new AOBBlock(RESOLUTION_SCALE_INTERCEPT_KEY, "48 8B 70 60 48 8B 82 48 02 00 00 48 89 B4 24 B8 00 00 00", 1);
		aobBlocks[PAUSE_FUNCTION_LOCATION_KEY] = new AOBBlock(PAUSE_FUNCTION_LOCATION_KEY, "53 48 83 EC 20 48 89 CB 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? FF 83 6C 18 00 00 83 BB 6C 18 00 00 01", 1);
		aobBlocks[UNPAUSE_FUNCTION_LOCATION_KEY] = new AOBBlock(UNPAUSE_FUNCTION_LOCATION_KEY, "53 48 83 EC 20 48 89 CB 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 8B 83 ?? ?? ?? ?? 85 C0", 1);
		aobBlocks[HUD_RENDER_INTERCEPT_KEY] = new AOBBlock(HUD_RENDER_INTERCEPT_KEY, "48 8B C4 48 89 58 08 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 B0 48 81 EC ?? ??  00 00 0F 29 70 B8 0F 29 78 A8 44 0F 29 40 98", 1);
		aobBlocks[PHOTOMODE_RANGE_DISABLE_KEY] = new AOBBlock(PHOTOMODE_RANGE_DISABLE_KEY, "F3 41 0F 5D D0 0F 28 C1 0F C6 D2 00 41 0F 59 D2", 1);
		aobBlocks[DOF_ENABLE_WRITE_LOCATION_KEY] = new AOBBlock(DOF_ENABLE_WRITE_LOCATION_KEY, "88 83 11 01 00 00 E8 ?? ?? ?? ?? 88 83 13 01 00 00 84 C0", 1);
		aobBlocks[AR_LIMIT_LOCATION_KEY] = new AOBBlock(AR_LIMIT_LOCATION_KEY, "F3 44 0F 59 CF 41 0F 28 D0 F3 0F 5C C2 0F 28 FA 44 0F 28 F2", 1);
		aobBlocks[FOG_READ_INTERCEPT_KEY] = new AOBBlock(FOG_READ_INTERCEPT_KEY, "F3 41 0F 10 7E 58 F3 44 0F 59 51 20 F3 45 0F 10 46 50 0F 29 44 24 70", 1);*/

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
		GameImageHooker::setHook(aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY], 0x13, &_cameraStructInterceptionContinue, &cameraStructInterceptor);
	}


	void setPostCameraStructHooks(map<string, AOBBlock*> &aobBlocks)
	{
		/*GameImageHooker::setHook(aobBlocks[CAMERA_WRITE1_INTERCEPT_KEY], 0x13, &_cameraWrite1InterceptionContinue, &cameraWrite1Interceptor);
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE2_INTERCEPT_KEY], 0x17, &_cameraWrite2InterceptionContinue, &cameraWrite2Interceptor);
		GameImageHooker::setHook(aobBlocks[TOD_WRITE_INTERCEPT_KEY], 0x1C, &_todWriteInterceptionContinue, &todWriteInterceptor);
		GameImageHooker::setHook(aobBlocks[TIMESTOP_READ_INTERCEPT_KEY], 0x14, &_timestopReadInterceptionContinue, &timestopReadInterceptor);
		GameImageHooker::setHook(aobBlocks[RESOLUTION_SCALE_INTERCEPT_KEY], 0x17, &_resolutionScaleReadInterceptionContinue, &resolutionScaleReadInterceptor);
		GameImageHooker::setHook(aobBlocks[FOG_READ_INTERCEPT_KEY], 0x12, &_fogReadInterceptionContinue, &fogReadInterceptor);*/

		//CameraManipulator::setPauseUnpauseGameFunctionPointers(aobBlocks[PAUSE_FUNCTION_LOCATION_KEY]->absoluteAddress(), aobBlocks[UNPAUSE_FUNCTION_LOCATION_KEY]->absoluteAddress());
		//disablePhotomodeRangeLimit(aobBlocks);
		//disableARLimits(aobBlocks);
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
	// if enabled is true, we'll place a 'ret' at the start of the code block, making the game skip rendering any hud element. If false, we'll reset
	// the original first statement so code will proceed as normal. 
	//void toggleHudRenderState(map<string, AOBBlock*> &aobBlocks, bool enabled)
	//{
	//	// start of HUD widget render code
	//	//ACOdyssey.exe+C0DFCC0 - 48 8B C4              - mov rax,rsp					<< Place RET here to hide hud completely. Restore-> hud is there again.
	//	//ACOdyssey.exe+C0DFCC3 - 48 89 58 08           - mov [rax+08],rbx
	//	//ACOdyssey.exe+C0DFCC7 - 55                    - push rbp
	//	//ACOdyssey.exe+C0DFCC8 - 56                    - push rsi
	//	//ACOdyssey.exe+C0DFCC9 - 57                    - push rdi
	//	//ACOdyssey.exe+C0DFCCA - 41 54                 - push r12
	//	//ACOdyssey.exe+C0DFCCC - 41 55                 - push r13
	//	//ACOdyssey.exe+C0DFCCE - 41 56                 - push r14
	//	//ACOdyssey.exe+C0DFCD0 - 41 57                 - push r15
	//	//ACOdyssey.exe+C0DFCD2 - 48 8D 6C 24 B0        - lea rbp,[rsp-50]
	//	//ACOdyssey.exe+C0DFCD7 - 48 81 EC 50010000     - sub rsp,00000150 { 336 }
	//	//ACOdyssey.exe+C0DFCDE - 0F29 70 B8            - movaps [rax-48],xmm6
	//	//ACOdyssey.exe+C0DFCE2 - 0F29 78 A8            - movaps [rax-58],xmm7
	//	//ACOdyssey.exe+C0DFCE6 - 44 0F29 40 98         - movaps [rax-68],xmm8
	//	//ACOdyssey.exe+C0DFCEB - 48 8B 05 2E3A1FF9     - mov rax,[ACOdyssey.exe+52D3720] { [53E4231C] }
	//	//ACOdyssey.exe+C0DFCF2 - 48 31 E0              - xor rax,rsp
	//	//ACOdyssey.exe+C0DFCF5 - 48 89 45 10           - mov [rbp+10],rax
	//	//ACOdyssey.exe+C0DFCF9 - 4C 8B 62 18           - mov r12,[rdx+18]
	//	//ACOdyssey.exe+C0DFCFD - 48 89 D3              - mov rbx,rdx
	//	//ACOdyssey.exe+C0DFD00 - 48 8B 32              - mov rsi,[rdx]
	//	//ACOdyssey.exe+C0DFD03 - 45 89 C6              - mov r14d,r8d
	//	if (enabled)
	//	{
	//		// set ret
	//		BYTE statementBytes[1] = { 0xC3 };						// ACOdyssey.exe+C0DFCC0 - 48 8B C4              - mov rax,rsp
	//		GameImageHooker::writeRange(aobBlocks[HUD_RENDER_INTERCEPT_KEY], statementBytes, 1);
	//	}
	//	else
	//	{
	//		// set original statement
	//		BYTE statementBytes[3] = { 0x48, 0x8B, 0xC4 };			// ACOdyssey.exe+C0DFCC0 - 48 8B C4              - mov rax,rsp
	//		GameImageHooker::writeRange(aobBlocks[HUD_RENDER_INTERCEPT_KEY], statementBytes, 3);
	//	}
	//}
	
}
