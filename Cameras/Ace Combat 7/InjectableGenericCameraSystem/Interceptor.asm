;////////////////////////////////////////////////////////////////////////////////////////////////////////
;// Part of Injectable Generic Camera System
;// Copyright(c) 2017, Frans Bouma
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
PUBLIC timestopInterceptor
PUBLIC resolutionscaleInterceptor
;---------------------------------------------------------------

;---------------------------------------------------------------
; Externs which are used and set by the system. Read / write these
; values in asm to communicate with the system
EXTERN g_cameraEnabled: byte
EXTERN g_ultraWidefix: byte
EXTERN g_cameraStructAddress: qword
EXTERN g_timestopStructAddress: qword
EXTERN g_resolutionscaleStructAddress: qword
;---------------------------------------------------------------

;---------------------------------------------------------------
; Own externs, defined in InterceptorHelper.cpp
EXTERN _cameraStructInterceptionContinue: qword
EXTERN _timestopInterceptionContinue: qword
EXTERN _resolutionscaleInterceptionContinue: qword
EXTERN _fovDelta: dword
.data
;---------------------------------------------------------------
; Scratch pad
.code


cameraStructInterceptor PROC
;// Used in-game and in photomode
;// UE uses angles for rotation. In UE3 it uses packed 16 bit ints, in UE4 it uses floats, in degrees (0-360.0)
;Relocated code. Simply skip the writes if the camera is enabled.
;Ace7Game.exe+EA6AE85 - 48 8D 8F 40040000     - lea rcx,[rdi+00000440]
;Ace7Game.exe+EA6AE8C - 83 A7 2C040000 FC     - and dword ptr [rdi+0000042C],-04 { 252 }
;Ace7Game.exe+EA6AE93 - F2 0F11 87 00040000   - movsd [rdi+00000400],xmm0  <<<intercept here
;Ace7Game.exe+EA6AE9B - F2 0F10 44 24 3C      - movsd xmm0,[rsp+3C]
;Ace7Game.exe+EA6AEA1 - F2 0F11 87 0C040000   - movsd [rdi+0000040C],xmm0
;Ace7Game.exe+EA6AEA9 - 0F10 44 24 48         - movups xmm0,[rsp+48]
;Ace7Game.exe+EA6AEAE - 89 87 08040000        - mov [rdi+00000408],eax
;Ace7Game.exe+EA6AEB4 - 8B 44 24 44           - mov eax,[rsp+44]
;Ace7Game.exe+EA6AEB8 - 89 87 14040000        - mov [rdi+00000414],eax
;Ace7Game.exe+EA6AEBE - 8B 44 24 5C           - mov eax,[rsp+5C]
;Ace7Game.exe+EA6AEC2 - 0F11 87 18040000      - movups [rdi+00000418],xmm0
;Ace7Game.exe+EA6AEC9 - 83 E0 03              - and eax,03 { 3 }					<<<continue here
;Ace7Game.exe+EA6AECC - F3 0F10 44 24 58      - movss xmm0,[rsp+58]
;Ace7Game.exe+EA6AED2 - 09 87 2C040000        - or [rdi+0000042C],eax
	mov [g_cameraStructAddress],rdi
	cmp byte ptr [g_cameraEnabled], 1					; check if the user enabled the camera. If so, just skip the write statements, otherwise just execute the original code.
	je exit
	cmp byte ptr [g_ultraWidefix], 1
	je uwfix
originalCode:
	movsd qword ptr [rdi+00000400h],xmm0
	movsd xmm0,qword ptr [rsp+3Ch]
	movsd qword ptr [rdi+0000040Ch],xmm0
	movups xmm0,xmmword ptr [rsp+48h]
	mov dword ptr [rdi+00000408h],eax
	mov eax,dword ptr [rsp+44h]
	mov dword ptr [rdi+00000414h],eax
	mov eax,dword ptr [rsp+5Ch]
	movups xmmword ptr [rdi+00000418h],xmm0
	jmp exit
uwfix:
	movsd qword ptr [rdi+00000400h],xmm0
	movsd xmm0,qword ptr [rsp+3Ch]
	movsd qword ptr [rdi+0000040Ch],xmm0
	movups xmm0,xmmword ptr [rsp+48h]
	mov dword ptr [rdi+00000408h],eax
	mov eax,dword ptr [rsp+44h]
	mov dword ptr [rdi+00000414h],eax
	mov eax,dword ptr [rsp+5Ch]
	movss xmm8, dword ptr [_fovDelta]
	addss xmm0,xmm8
	movups xmmword ptr [rdi+00000418h],xmm0
exit:
	jmp qword ptr [_cameraStructInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraStructInterceptor ENDP


timestopInterceptor PROC
; AC7 has a multiplier which is used to slow down the replay - the operation we are injecting at essentially multiplies the whole game speed by this value. So if we set it to 0 the game speed is multiplied by 0, giving us a timestop.
;Ace7Game.exe+E59C2A8 - F3 0F59 83 D0040000   - mulss xmm0,[rbx+000004D0]			<<<inject here/read timescale multiplier address
;Ace7Game.exe+E59C2B0 - F3 0F59 F0            - mulss xmm6,xmm0
;Ace7Game.exe+E59C2B4 - 0F28 CE               - movaps xmm1,xmm6
;Ace7Game.exe+E59C2B7 - FF 90 28060000        - call qword ptr [rax+00000628]		<< continue here
;Ace7Game.exe+E59C2BD - 0F28 C8               - movaps xmm1,xmm0
;Ace7Game.exe+E59C2C0 - 0F28 F0               - movaps xmm6,xmm0
    mov [g_timestopStructAddress],rbx
originalCode:
	mulss xmm0,dword ptr [rbx+000004D0h]					
	mulss xmm6,xmm0
	movaps xmm1,xmm6
exit:
	jmp qword ptr [_timestopInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
timestopInterceptor ENDP

resolutionscaleInterceptor PROC
;intercept address of the struct where the resolution address is stored. No need for any overwrites, we just need the address.
;Ace7Game.exe+EAEC51A - F3 0F10 00            - movss xmm0,[rax]								<<<intercept here/read resolutionscale address
;Ace7Game.exe+EAEC51E - F3 41 0F59 C1         - mulss xmm0,xmm9
;Ace7Game.exe+EAEC523 - F3 0F59 83 C4110000   - mulss xmm0,[rbx+000011C4]			
;Ace7Game.exe+EAEC52B - F3 0F11 83 C4110000   - movss [rbx+000011C4],xmm0						<<< continue here
;Ace7Game.exe+EAEC533 - 48 8B 05 66E30EF5     - mov rax,[Ace7Game.exe+3BDA8A0] { (29D940C0) }
	mov [g_resolutionscaleStructAddress],rax
originalCode:
	movss xmm0,dword ptr [rax]
	mulss xmm0,xmm9
	mulss xmm0,dword ptr [rbx+000011C4h]
exit:
	jmp qword ptr [_resolutionscaleInterceptionContinue]
resolutionscaleInterceptor ENDP

END