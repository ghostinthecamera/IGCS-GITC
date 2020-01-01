;////////////////////////////////////////////////////////////////////////////////////////////////////////
;// Part of Injectable Generic Camera System
;// Copyright(c) 2019, Frans Bouma
;// All rights reserved.
;// https://github.com/FransBouma/InjectableGenericCameraSystem
;//
;// Redistribution and use in source and binary forms, with or without
;// modification, are permitted provided that the following conditions are met :
;//
;//  * Redistributions of source code must retain the above copyright notice, this
;//	  list of conditions and the following disclaimer.
;//
;//  * Redistributions in binary form must reproduce the above copyright notice,
;//    this list of conditions and the following disclaimer in the documentation
;//    and / or other materials provided with the distribution.
;//
;// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
;// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
;// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
;// DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
;// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
;// DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
;// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
;// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
;// OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
;// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;////////////////////////////////////////////////////////////////////////////////////////////////////////
;---------------------------------------------------------------
; Game specific asm file to intercept execution flow to obtain addresses, prevent writes etc.
;---------------------------------------------------------------

;---------------------------------------------------------------
; Public definitions so the linker knows which names are present in this file
PUBLIC cameraStructInterceptor
;PUBLIC resolutionScaleReadInterceptor
PUBLIC timestopReadInterceptor

;---------------------------------------------------------------

;---------------------------------------------------------------
; Externs which are used and set by the system. Read / write these
; values in asm to communicate with the system
EXTERN g_cameraEnabled: byte
EXTERN g_cameraStructAddress: qword
;EXTERN g_resolutionscaleStructAddress: qword
EXTERN g_timestopStructAddress: qword

;---------------------------------------------------------------

;---------------------------------------------------------------
; Own externs, defined in InterceptorHelper.cpp
EXTERN _cameraStructInterceptionContinue: qword
;EXTERN _resolutionScaleReadInterceptionContinue: qword
EXTERN _timestopReadInterceptionContinue: qword
;EXTERN _resolutionHookABSADD: qword

.data


.code


cameraStructInterceptor PROC
;// UE uses angles for rotation. In UE3 it uses packed 16 bit ints, in UE4 it uses floats, in degrees (0-360.0)
;Simply skip the writes if the camera is enabled.
;F2 0F 11 01 48 8B DA
;MechWarrior-Win64-Shipping.exe+2210325 - 57                    - push rdi
;MechWarrior-Win64-Shipping.exe+2210326 - 48 83 EC 20           - sub rsp,20 { 32 }
;MechWarrior-Win64-Shipping.exe+221032A - F2 0F10 02            - movsd xmm0,[rdx]
;MechWarrior-Win64-Shipping.exe+221032E - 48 8B F9              - mov rdi,rcx
;MechWarrior-Win64-Shipping.exe+2210331 - F2 0F11 01            - movsd [rcx],xmm0			<<<inject here	
;MechWarrior-Win64-Shipping.exe+2210335 - 48 8B DA              - mov rbx,rdx
;MechWarrior-Win64-Shipping.exe+2210338 - 8B 42 08              - mov eax,[rdx+08]
;MechWarrior-Win64-Shipping.exe+221033B - 89 41 08              - mov [rcx+08],eax
;MechWarrior-Win64-Shipping.exe+221033E - F2 0F10 42 0C         - movsd xmm0,[rdx+0C]
;MechWarrior-Win64-Shipping.exe+2210343 - F2 0F11 41 0C         - movsd [rcx+0C],xmm0
;MechWarrior-Win64-Shipping.exe+2210348 - 8B 42 14              - mov eax,[rdx+14]
;MechWarrior-Win64-Shipping.exe+221034B - 89 41 14              - mov [rcx+14],eax
;MechWarrior-Win64-Shipping.exe+221034E - 8B 42 18              - mov eax,[rdx+18]
;MechWarrior-Win64-Shipping.exe+2210351 - 89 41 18              - mov [rcx+18],eax			<<<return here
;MechWarrior-Win64-Shipping.exe+2210354 - 8B 42 1C              - mov eax,[rdx+1C]
;MechWarrior-Win64-Shipping.exe+2210357 - 89 41 1C              - mov [rcx+1C],eax
;MechWarrior-Win64-Shipping.exe+221035A - 8B 42 20              - mov eax,[rdx+20]
;MechWarrior-Win64-Shipping.exe+221035D - 89 41 20              - mov [rcx+20],eax
;MechWarrior-Win64-Shipping.exe+2210360 - 8B 42 24              - mov eax,[rdx+24]
;MechWarrior-Win64-Shipping.exe+2210363 - 89 41 24              - mov [rcx+24],eax
;MechWarrior-Win64-Shipping.exe+2210366 - 8B 42 28              - mov eax,[rdx+28]
;MechWarrior-Win64-Shipping.exe+2210369 - 89 41 28              - mov [rcx+28],eax
;MechWarrior-Win64-Shipping.exe+221036C - 8B 42 2C              - mov eax,[rdx+2C]
	cmp r8,00		;code writes to multiple addresses so we need to do some tests to find the correct call. In the correct call, the r8,r9 and r15 registers are all 0x0, so this is what we test for
	je test2
originalcode:
	movsd qword ptr [rcx],xmm0
	mov rbx,rdx
	mov dword ptr eax,[rdx+08]
	mov [rcx+08h],eax
	movsd xmm0,qword ptr [rdx+0Ch]
	movsd qword ptr [rcx+0Ch],xmm0
	mov dword ptr eax,[rdx+14h]
	mov [rcx+14h],eax
	mov dword ptr eax,[rdx+18h]
	mov [rcx+18h],eax
	jmp exit
test2:
	cmp r9,00
	je test3
	jne originalcode
test3:
	cmp r15,00
	je correctcam
	jne originalcode
correctcam:
	mov [g_cameraStructAddress],rcx
	cmp [g_cameraEnabled],01
	jne originalcode			;if camera is not enabled jump back to originalcode
	;movsd [rcx],xmm0		<< x and y position
	mov rbx,rdx
	;mov eax,[rdx+08h]		<< z position
	mov [rcx+08h],eax
	movsd xmm0, qword ptr [rdx+0Ch]
	;movsd [rcx+0Ch],xmm0	<< pitch and yaw
	mov eax,[rdx+14h]
	;mov [rcx+14h],eax		<< roll
	mov eax,[rdx+18h]
	;mov [rcx+18h],eax		<< fov
exit:
	jmp qword ptr [_cameraStructInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraStructInterceptor ENDP

timestopReadInterceptor PROC
; 48 3B C2 75 ?? | F3 0F 10 81 ?? ?? ?? ?? F3 0F 59 81 ?? ?? ?? ?? F3 0F 59 81 ?? ?? ?? ?? F3 0F 59 83 ?? ?? ?? ?? 48 83 C4 ?? 5B C3 FF 90 ?? ?? ?? ??
; AC7 has a multiplier which is used. If we set it to 0 the game speed is multiplied by 0, giving us a timestop.
;MechWarrior-Win64-Shipping.exe+1EBE5AC - 48 8D 15 F5CFD001     - lea rdx,[MechWarrior-Win64-Shipping.exe+3BCB5A8] { (7FF6FEC32EE0) }
;MechWarrior-Win64-Shipping.exe+1EBE5B3 - 48 3B C2              - cmp rax,rdx
;MechWarrior-Win64-Shipping.exe+1EBE5B6 - 75 26                 - jne MechWarrior-Win64-Shipping.exe+1EBE5DE
;MechWarrior-Win64-Shipping.exe+1EBE5B8 - F3 0F10 81 64030000   - movss xmm0,[rcx+00000364]		<<inject here
;MechWarrior-Win64-Shipping.exe+1EBE5C0 - F3 0F59 81 60030000   - mulss xmm0,[rcx+00000360]		
;MechWarrior-Win64-Shipping.exe+1EBE5C8 - F3 0F59 81 68030000   - mulss xmm0,[rcx+00000368]		<<return here
;MechWarrior-Win64-Shipping.exe+1EBE5D0 - F3 0F59 83 C0000000   - mulss xmm0,[rbx+000000C0]
;MechWarrior-Win64-Shipping.exe+1EBE5D8 - 48 83 C4 20           - add rsp,20 { 32 }
;MechWarrior-Win64-Shipping.exe+1EBE5DC - 5B                    - pop rbx
	mov [g_timestopStructAddress],rcx
originalCode:
	movss xmm0, dword ptr [rcx+00000364h]
	mulss xmm0, dword ptr [rcx+00000360h]
exit:
	jmp qword ptr [_timestopReadInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
timestopReadInterceptor ENDP

END