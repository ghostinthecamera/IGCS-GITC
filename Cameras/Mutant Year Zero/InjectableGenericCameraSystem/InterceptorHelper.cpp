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
	void resolutionScaleReadInterceptor();
	void timestopReadInterceptor();
}

// external addresses used in asm.
extern "C" {
	LPBYTE _cameraStructInterceptionContinue = nullptr;
	LPBYTE _resolutionHookABSADD = nullptr;
	LPBYTE _resolutionScaleReadInterceptionContinue = nullptr;
	LPBYTE _timestopReadInterceptionContinue = nullptr;
	LPBYTE _HUDAddress = nullptr;
}



namespace IGCS::GameSpecific::InterceptorHelper
{
	void initializeAOBBlocks(LPBYTE hostImageAddress, DWORD hostImageSize, map<string, AOBBlock*> &aobBlocks)
	{
		aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY] = new AOBBlock(CAMERA_ADDRESS_INTERCEPT_KEY, "F2 0F 11 87 ?? ?? ?? ?? 8B 44 24 ?? 89 87 ?? ?? ?? ?? F2 0F 10 44 24 ??", 1);	
		aobBlocks[TIMESTOP_READ_INTERCEPT_KEY] = new AOBBlock(TIMESTOP_READ_INTERCEPT_KEY, "F3 0F 59 83 ?? ?? ?? ?? F3 0F 59 F0 48 8B 03 0F 28 D7", 1);
		aobBlocks[RESOLUTION_SCALE_INTERCEPT_KEY] = new AOBBlock(RESOLUTION_SCALE_INTERCEPT_KEY, "F3 0F 10 00 0F 57 F6 F3 44 0F 10 0D ?? ?? ?? ??", 1);
		aobBlocks[RESOLUTION_ABSADD_INTERCEPT_KEY] = new AOBBlock(RESOLUTION_ABSADD_INTERCEPT_KEY, "F3 44 0F 10 0D | ?? ?? ?? ?? 0F2F C6", 1);
		aobBlocks[HUD_RENDER_INTERCEPT_KEY] = new AOBBlock(HUD_RENDER_INTERCEPT_KEY, "0F 10 05 | ?? ?? ?? ?? 33 C0 48 89 41 ?? 48 89 41 ?? 48 89 41 ?? 0F 11 01", 1);

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
		GameImageHooker::setHook(aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY], 0x36, &_cameraStructInterceptionContinue, &cameraStructInterceptor);
	}

	void getAbsoluteAddresses(map<string, AOBBlock*> &aobBlocks)
	{
		_resolutionHookABSADD = Utils::calculateAbsoluteAddress(aobBlocks[RESOLUTION_ABSADD_INTERCEPT_KEY], 4);
		_HUDAddress = Utils::calculateAbsoluteAddress(aobBlocks[HUD_RENDER_INTERCEPT_KEY], 4);

		OverlayConsole::instance().logDebug("Absolute Address for Res Scale Injection: %p", (void*)_resolutionHookABSADD);
		OverlayConsole::instance().logDebug("Absolute Address for HUD: %p", (void*)_HUDAddress);
	}


	void setPostCameraStructHooks(map<string, AOBBlock*> &aobBlocks)
	{
		GameImageHooker::setHook(aobBlocks[TIMESTOP_READ_INTERCEPT_KEY], 0x0F, &_timestopReadInterceptionContinue, &timestopReadInterceptor);
		GameImageHooker::setHook(aobBlocks[RESOLUTION_SCALE_INTERCEPT_KEY], 0x10, &_resolutionScaleReadInterceptionContinue, &resolutionScaleReadInterceptor);

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

	void hudToggle()
	{
		float* hudInMemory = reinterpret_cast<float*>(_HUDAddress + 0x0C);
		OverlayConsole::instance().logDebug("HUD Toggle 1 Address: %p", (void*)hudInMemory);
		//OverlayConsole::instance().logDebug("HUD Toggle 1 value: %f", (float)*hudInMemory);
		*hudInMemory = *hudInMemory > 0.1f ? 0.0f : 1.0f;
		//OverlayConsole::instance().logDebug("HUD Toggle 1 value: %f", (float)*hudInMemory);
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
