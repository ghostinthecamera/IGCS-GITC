////////////////////////////////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System
// Copyright(c) 2017, Frans Bouma
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
#include "Console.h"

using namespace std;

//--------------------------------------------------------------------------------------------------------------------------------
// external asm functions
extern "C" {
	void cameraStructInterceptor();
	void cameraWrite1Interceptor();
	void cameraWrite2Interceptor();
	//void cameraWrite3Interceptor();
	//void timestopInterceptor();
	//void fovReadInterceptor();
}

// external addresses used in asm.
extern "C" {
	LPBYTE _cameraStructInterceptionContinue = nullptr;
	LPBYTE _cameraWrite1InterceptionContinue = nullptr;
	LPBYTE _cameraWrite2InterceptionContinue = nullptr;
	//LPBYTE _cameraWrite3InterceptionContinue = nullptr;
	//LPBYTE _timestopInterceptionContinue = nullptr;
	//LPBYTE _fovReadInterceptionContinue = nullptr;
}


namespace IGCS::GameSpecific::InterceptorHelper
{
	void initializeAOBBlocks(LPBYTE hostImageAddress, DWORD hostImageSize, map<string, AOBBlock*> &aobBlocks)
	{
		aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY] = new AOBBlock(CAMERA_ADDRESS_INTERCEPT_KEY, "0F 57 0C C7 66 0F 70 ?? ??", 1);	
		//aobBlocks[FOV_INTERCEPT_KEY] = new AOBBlock(FOV_INTERCEPT_KEY, "F3 0F 58 48 08 0F 2F C8 76 03 0F 28 C1 48 83 C4 20", 1);
		aobBlocks[CAMERA_WRITE1_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE1_INTERCEPT_KEY, "0F 11 51 ?? 0F 50 C0 85 C0 74 2D", 1);
		aobBlocks[CAMERA_WRITE2_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE2_INTERCEPT_KEY, "03 47 10 0F ?? 00 0F C2 C1 ?? | 0F 11 08 0F 50 C0 A8 ?? 74", 1);
		//aobBlocks[CAMERA_WRITE3_INTERCEPT_KEY] = new AOBBlock(CAMERA_WRITE3_INTERCEPT_KEY, "F3 44 0F 11 77 24 F3 0F 10 8D 88 00 00 00 48 8D 4F 1C", 1);
		//aobBlocks[TIMESTOP_INTERCEPT_KEY] = new AOBBlock(TIMESTOP_INTERCEPT_KEY, "48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 41 54 41 55 41 56 41 57 48 83 EC 40 48 8B B1", 1);
		// the following 2 blocks are obtained from code which isn't intercepted; these AOB scans are used to obtain a RIP relative DWord offset, see interceptor.asm for details.
		//aobBlocks[SUPERSAMPLING_KEY] = new AOBBlock(SUPERSAMPLING_KEY, "41 F7 F8 44 8B C0 8B C1  8B 0D | ?? ?? ?? ?? 99  F7 FF 44 3B C0 41 0F 4C C0", 1);
		//aobBlocks[HUD_TOGGLE_KEY] = new AOBBlock(HUD_TOGGLE_KEY, "83 3D | ?? ?? ?? ?? 00 0F B6 F2 48 89 CF 74 ?? 48 8B 81 C8 00 00 00 48 89 5C 24", 1);
		
		map<string, AOBBlock*>::iterator it;
		bool result = true;
		for (it = aobBlocks.begin(); it != aobBlocks.end(); it++)
		{
			result &= it->second->scan(hostImageAddress, hostImageSize);
		}
		if (result)
		{
			Console::WriteLine("All interception offsets found.");
		}
		else
		{
			Console::WriteError("One or more interception offsets weren't found: tools aren't compatible with this game's version.");
		}
	}


	void setCameraStructInterceptorHook(map<string, AOBBlock*> &aobBlocks)
	{
		GameImageHooker::setHook(aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY], 0x0E, &_cameraStructInterceptionContinue, &cameraStructInterceptor);
	}


	void setPostCameraStructHooks(map<string, AOBBlock*> &aobBlocks)
	{
		//GameImageHooker::setHook(aobBlocks[FOV_INTERCEPT_KEY], 0x11, &_fovReadInterceptionContinue, &fovReadInterceptor);
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE1_INTERCEPT_KEY], 0x07, &_cameraWrite1InterceptionContinue, &cameraWrite1Interceptor);
		GameImageHooker::setHook(aobBlocks[CAMERA_WRITE2_INTERCEPT_KEY], 0x06, &_cameraWrite2InterceptionContinue, &cameraWrite2Interceptor);
		//GameImageHooker::setHook(aobBlocks[CAMERA_WRITE3_INTERCEPT_KEY], 0x12, &_cameraWrite3InterceptionContinue, &cameraWrite3Interceptor);
		//GameImageHooker::setHook(aobBlocks[TIMESTOP_INTERCEPT_KEY], 0xF, &_timestopInterceptionContinue, &timestopInterceptor);
	}
}
