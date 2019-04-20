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
#include "GameImageHooker.h"
#include "Defaults.h"
#include "OverlayConsole.h"

namespace IGCS::GameImageHooker
{
	// Sets a jmp qword ptr [address] statement at hostImageAddress + startOffset for x64 and a jmp <relative address> for x86
	void setHook(LPBYTE hostImageAddress, DWORD startOffset, DWORD continueOffset, LPBYTE* interceptionContinue, void* asmFunction)
	{
		if (hostImageAddress == nullptr)
		{
			return;
		}
		LPBYTE startOfHookAddress = hostImageAddress + startOffset;
		*interceptionContinue = startOfHookAddress + continueOffset;
#ifdef _WIN64
		// x64
		BYTE instruction[14];	// 6 bytes of the jmp qword ptr [0] and 8 bytes for the real address which is stored right after the 6 bytes of jmp qword ptr [0] bytes 
								// write bytes of jmp qword ptr [address], which is jmp qword ptr 0 offset.
		memcpy(instruction, jmpFarInstructionBytes, sizeof(jmpFarInstructionBytes));
		// now write the address. Do this with a recast of the pointer to an __int64 pointer to avoid endianmess.
		__int64* targetAddressLocationInInstruction = (__int64*)(&instruction[6]);
		__int64 targetAddress = (__int64)asmFunction;
#else	
		// x86
		// we will write a jmp <relative address> as x86 doesn't have a jmp <absolute address>. 
		// calculate this relative address by using Destination - Current, which is: &asmFunction - (<base> + startOffset + 5), as jmp <relative> is 5 bytes.
		BYTE instruction[5];
		instruction[0] = 0xE9;	// JMP relative
		DWORD targetAddress = (DWORD)asmFunction - (((DWORD)startOfHookAddress) + 5);
		DWORD* targetAddressLocationInInstruction = (DWORD*)&instruction[1];
#endif
		targetAddressLocationInInstruction[0] = targetAddress;	// write bytes this way to avoid endianess
		SIZE_T noBytesWritten;
		BOOL result = WriteProcessMemory(OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, GetCurrentProcessId()), startOfHookAddress, instruction, sizeof(instruction), &noBytesWritten);
		if (result)
		{
			OverlayConsole::instance().logDebug("Hook set to address: %p", (void*)startOfHookAddress);
		}
		else
		{
			OverlayConsole::instance().logError("Couldn't write to process memory, so couldn't set hook. Error code: %010x", GetLastError());
		}
	}
	

	// Sets a jmp qword ptr [address] statement at baseAddress + startOffset for x64 and a jmp <relative address> for x86
	void setHook(AOBBlock* hookData, DWORD continueOffset, LPBYTE* interceptionContinue, void* asmFunction)
	{
		setHook(hookData->locationInImage(), hookData->customOffset(), continueOffset, interceptionContinue, asmFunction);
	}


	// Writes the bytes pointed at by bufferToWrite starting at address startAddress, for the length in 'length'.
	void writeRange(LPBYTE startAddress, BYTE* bufferToWrite, int length)
	{
		SIZE_T noBytesWritten;
		WriteProcessMemory(OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, GetCurrentProcessId()), startAddress, bufferToWrite, length, &noBytesWritten);
	}


	// Writes the bytes pointed at by bufferToWrite starting at address startAddress, for the length in 'length'.
	void writeRange(AOBBlock* hookData, BYTE* bufferToWrite, int length)
	{
		writeRange(hookData->locationInImage() + hookData->customOffset(), bufferToWrite, length);
	}


	// Writes NOP opcodes to a range of memory.
	void nopRange(LPBYTE startAddress, int length)
	{
		BYTE* nopBuffer;
		if (length < 0 || length>1024)
		{
			// no can/wont do 
			return;
		}
		nopBuffer = (BYTE*)malloc(length * sizeof(BYTE));
		for (int i = 0; i < length; i++)
		{
			nopBuffer[i] = 0x90;
		}
		SIZE_T noBytesWritten;
		WriteProcessMemory(OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, GetCurrentProcessId()), startAddress, nopBuffer, length, &noBytesWritten);
		free(nopBuffer);
	}


	// Writes NOP opcodes to a range of memory.
	void nopRange(AOBBlock* hookData, int length)
	{
		nopRange(hookData->locationInImage() + hookData->customOffset(), length);
	}
}