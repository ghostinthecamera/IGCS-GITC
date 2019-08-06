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
PUBLIC resolutionScaleReadInterceptor
PUBLIC timestopReadInterceptor

;---------------------------------------------------------------

;---------------------------------------------------------------
; Externs which are used and set by the system. Read / write these
; values in asm to communicate with the system
EXTERN g_cameraEnabled: byte
EXTERN g_cameraStructAddress: qword
EXTERN g_resolutionscaleStructAddress: qword
EXTERN g_timestopStructAddress: qword

;---------------------------------------------------------------

;---------------------------------------------------------------
; Own externs, defined in InterceptorHelper.cpp
EXTERN _cameraStructInterceptionContinue: qword
EXTERN _resolutionScaleReadInterceptionContinue: qword
EXTERN _timestopReadInterceptionContinue: qword
EXTERN _resolutionHookABSADD: qword

.data


.code


cameraStructInterceptor PROC
;// UE uses angles for rotation. In UE3 it uses packed 16 bit ints, in UE4 it uses floats, in degrees (0-360.0)
;Simply skip the writes if the camera is enabled.
;F2 0F 11 87 x x x x 8B 44 24 x 89 87 x x x x F2 0F 10 44 24 x
;ZoneUE4-Win64-Shipping.exe+1DB9556 - 89 8F 00040000        - mov [rdi+00000400],ecx
;ZoneUE4-Win64-Shipping.exe+1DB955C - F2 0F10 44 24 40      - movsd xmm0,[rsp+40]
;ZoneUE4-Win64-Shipping.exe+1DB9562 - F2 0F11 87 10040000   - movsd [rdi+00000410],xmm0		<<<<inject here
;ZoneUE4-Win64-Shipping.exe+1DB956A - 8B 44 24 48           - mov eax,[rsp+48]
;ZoneUE4-Win64-Shipping.exe+1DB956E - 89 87 18040000        - mov [rdi+00000418],eax
;ZoneUE4-Win64-Shipping.exe+1DB9574 - F2 0F10 44 24 4C      - movsd xmm0,[rsp+4C]
;ZoneUE4-Win64-Shipping.exe+1DB957A - F2 0F11 87 1C040000   - movsd [rdi+0000041C],xmm0
;ZoneUE4-Win64-Shipping.exe+1DB9582 - 8B 44 24 54           - mov eax,[rsp+54]
;ZoneUE4-Win64-Shipping.exe+1DB9586 - 89 87 24040000        - mov [rdi+00000424],eax
;ZoneUE4-Win64-Shipping.exe+1DB958C - 0F10 44 24 58         - movups xmm0,[rsp+58]
;ZoneUE4-Win64-Shipping.exe+1DB9591 - 0F11 87 28040000      - movups [rdi+00000428],xmm0
;ZoneUE4-Win64-Shipping.exe+1DB9598 - F3 0F10 44 24 68      - movss xmm0,[rsp+68]			<<<<<return here
;ZoneUE4-Win64-Shipping.exe+1DB959E - F3 0F11 87 38040000   - movss [rdi+00000438],xmm0
	mov [g_cameraStructAddress],rdi
	cmp byte ptr [g_cameraEnabled], 1
	je exit
originalCode:
	movsd qword ptr [rdi+00000410h],xmm0
	mov eax,dword ptr [rsp+48h]
	mov dword ptr [rdi+00000418h],eax
	movsd xmm0,qword ptr [rsp+4Ch]
	movsd qword ptr [rdi+0000041Ch],xmm0
	mov eax,dword ptr [rsp+54h]
	mov dword ptr[rdi+00000424h],eax
	movups xmm0,xmmword ptr[rsp+58h]
	movups xmmword ptr [rdi+00000428h],xmm0
exit:
	jmp qword ptr [_cameraStructInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraStructInterceptor ENDP

timestopReadInterceptor PROC
; F3 0F 59 83 x x x x F3 0F 59 F0 48 8B 03 0F 28 D7
; AC7 has a multiplier which is used. If we set it to 0 the game speed is multiplied by 0, giving us a timestop.
;ZoneUE4-Win64-Shipping.exe+1C65F4A - F3 0F10 83 E4040000   - movss xmm0,[rbx+000004E4]
;ZoneUE4-Win64-Shipping.exe+1C65F52 - F3 0F59 83 E0040000   - mulss xmm0,[rbx+000004E0]
;ZoneUE4-Win64-Shipping.exe+1C65F5A - F3 0F59 83 E8040000   - mulss xmm0,[rbx+000004E8]		<<< inject here/time value read
;ZoneUE4-Win64-Shipping.exe+1C65F62 - F3 0F59 F0            - mulss xmm6,xmm0
;ZoneUE4-Win64-Shipping.exe+1C65F66 - 48 8B 03              - mov rax,[rbx]
;ZoneUE4-Win64-Shipping.exe+1C65F69 - 0F28 D7               - movaps xmm2,xmm7				<<<< return here
;ZoneUE4-Win64-Shipping.exe+1C65F6C - 0F28 CE               - movaps xmm1,xmm6
;ZoneUE4-Win64-Shipping.exe+1C65F6F - 48 8B CB              - mov rcx,rbx
	mov [g_timestopStructAddress],rbx
originalCode:
	mulss xmm0,dword ptr [rbx+000004E8h]
	mulss xmm6,xmm0
	mov rax,[rbx]
exit:
	jmp qword ptr [_timestopReadInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
timestopReadInterceptor ENDP

resolutionScaleReadInterceptor PROC
;F3 0F 10 00 0F 57 F6 F3 44 0F 10 0D x x x x
;ZoneUE4-Win64-Shipping.exe+1DF2DB9 - F3 0F10 00				 - movss xmm0,[rax]			<<<inject/resolution scale
;ZoneUE4-Win64-Shipping.exe+1DF2DBD - 0F57 F6					 - xorps xmm6,xmm6
;ZoneUE4-Win64-Shipping.exe+1DF2DC0 - F3 44 0F10 0D 5BD12801	 - movss xmm9,[ZoneUE4-Win64-Shipping.exe+307FF24] { (0.01) }
;ZoneUE4-Win64-Shipping.exe+1DF2DC9 - 0F2F C6					 - comiss xmm0,xmm6			<<<return here
	mov [g_resolutionscaleStructAddress],rax
originalCode:
	movss xmm0, dword ptr [rax]
	xorps xmm6,xmm6
	push rax
	mov rax, [_resolutionHookABSADD]
	movss xmm9, dword ptr [rax]
	pop rax
exit:
	jmp qword ptr [_resolutionScaleReadInterceptionContinue] ; jmp back into the original game code, which is the location after the original statements above.
resolutionScaleReadInterceptor ENDP

END