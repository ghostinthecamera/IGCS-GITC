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
PUBLIC cameraFOVInterceptor
PUBLIC timescaleInterceptor
PUBLIC HUDinterceptor

;---------------------------------------------------------------

;---------------------------------------------------------------
; Externs which are used and set by the system. Read / write these
; values in asm to communicate with the system
EXTERN g_cameraEnabled: byte
EXTERN g_timestopEnabled: byte
EXTERN g_cameraStructAddress: qword
EXTERN g_fovStructAddress: qword
EXTERN g_timescaleAddress: qword
EXTERN g_HUDaddress: qword

;---------------------------------------------------------------

;---------------------------------------------------------------
; Own externs, defined in InterceptorHelper.cpp
EXTERN _cameraStructInterceptionContinue: qword
EXTERN _fovStructInterceptionContinue: qword
EXTERN _timescaleInterceptionContinue: qword
EXTERN _timestopOneAbsolute: qword
EXTERN _timestopTwoAbsolute: qword
EXTERN _HUDinterceptionContinue: qword

.data


.code

timevalue dd 0.0

cameraStructInterceptor PROC
;Nino2.exe+39DEFA - F3 0F59 C7            - mulss xmm0,xmm7
;Nino2.exe+39DEFE - F3 0F5C F0            - subss xmm6,xmm0
;Nino2.exe+39DF02 - E8 99C7E3FF           - call Nino2.exe+1DA6A0
;Nino2.exe+39DF07 - 0F10 00               - movups xmm0,[rax]
;_camBase2        - 0F11 07               - movups [rdi],xmm0			<<<inject here
;Nino2.exe+39DF0D - 4C 8D 9C 24 B8000000  - lea r11,[rsp+000000B8]
;Nino2.exe+39DF15 - 0F10 48 10            - movups xmm1,[rax+10]
;Nino2.exe+39DF19 - 0F11 4F 10            - movups [rdi+10],xmm1		<<<return here
;Nino2.exe+39DF1D - 0F10 40 20            - movups xmm0,[rax+20]
;Nino2.exe+39DF21 - 0F11 47 20            - movups [rdi+20],xmm0
;Nino2.exe+39DF25 - 0F10 48 30            - movups xmm1,[rax+30]
;Nino2.exe+39DF29 - 48 8B C7              - mov rax,rdi
;Nino2.exe+39DF2C - F3 0F10 05 ECC6A400   - movss xmm0,[Nino2.exe+DEA620] { (0.00) }
;Nino2.exe+39DF34 - 0F11 4F 30            - movups [rdi+30],xmm1
	cmp r15,1
	jne skip
	mov [g_cameraStructAddress],rdi
	;cmp byte ptr [g_cameraEnabled],1
skip:
	movups [rdi],xmm0
	lea r11,[rsp+000000B8h]
	movups xmm1,[rax+10h]
	movups [rdi+10h],xmm1
exit:
	jmp qword ptr [_cameraStructInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraStructInterceptor ENDP

cameraFOVInterceptor PROC
;Nino2.exe+B3DAE0 - 0F2F 83 9C020000      - comiss xmm0,[rbx+0000029C]
;Nino2.exe+B3DAE7 - 72 0F                 - jb Nino2.exe+B3DAF8
;Nino2.exe+B3DAE9 - 48 8B CB              - mov rcx,rbx
;Nino2.exe+B3DAEC - E8 2F7593FF           - call Nino2.exe+475020
;Nino2.exe+B3DAF1 - C6 83 AC020000 00     - mov byte ptr [rbx+000002AC],00 { 0 }
;Nino2.exe+B3DAF8 - 0F28 83 60010000      - movaps xmm0,[rbx+00000160]			<<intercept here
;Nino2.exe+B3DAFF - 0F29 87 80000000      - movaps [rdi+00000080],xmm0
;Nino2.exe+B3DB06 - 0F28 8B 70010000      - movaps xmm1,[rbx+00000170]			<<return here
;Nino2.exe+B3DB0D - 0F29 8F 90000000      - movaps [rdi+00000090],xmm1
;Nino2.exe+B3DB14 - 0F28 83 80010000      - movaps xmm0,[rbx+00000180]
;Nino2.exe+B3DB1B - 0F29 87 A0000000      - movaps [rdi+000000A0],xmm0
;Nino2.exe+B3DB22 - 0F28 8B 90010000      - movaps xmm1,[rbx+00000190]
	mov [g_fovStructAddress],rbx
	;cmp byte ptr [g_cameraEnabled],1
	movaps xmm0,[rbx+00000160h]
	movaps [rdi+00000080h],xmm0
exit:
	jmp qword ptr [_fovStructInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraFOVInterceptor ENDP

timescaleInterceptor PROC
;Nino2.exe+9D627F - FF 50 08              - call qword ptr [rax+08]
;Nino2.exe+9D6282 - E8 89001900           - call Nino2.exe+B66310
;Nino2.exe+9D6287 - F3 0F11 35 35168B00   - movss [Nino2.exe+12878C4],xmm6 { (0.00) }				<<intercept here
;Nino2.exe+9D628F - F3 0F11 3D 5D168B00   - movss [Nino2.exe+12878F4],xmm7 { (0.02) }				
;Nino2.exe+9D6297 - E8 047B9DFF           - call Nino2.exe+3ADDA0									<<return here
;Nino2.exe+9D629C - E8 3F7C9DFF           - call Nino2.exe+3ADEE0
	cmp byte ptr [g_timestopEnabled],1
	je stopTime
	push rcx
	push rdx
	mov rcx, [_timestopOneAbsolute]
	mov rdx, [_timestopTwoAbsolute]
	movss dword ptr [rcx],xmm6
	movss dword ptr [rdx],xmm7
	pop rcx
	pop rdx
	jmp exit
stopTime:
	push rbx
	push rcx
	push rdx
	mov ebx, [timevalue]
	mov rcx, [_timestopOneAbsolute]
	mov rdx, [_timestopTwoAbsolute]
	mov dword ptr [rcx], ebx
	movss dword ptr [rdx],xmm7
	pop rbx
	pop rcx
	pop rdx
exit:
	jmp qword ptr [_timescaleInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
timescaleInterceptor ENDP

HUDinterceptor PROC
;Nino2.exe+4F98D7 - 8B 81 D0390000        - mov eax,[rcx+000039D0]
;Nino2.exe+4F98DD - F3 0F11 44 24 24      - movss [rsp+24],xmm0
;Nino2.exe+4F98E3 - 48 C7 44 24 1C 02000000 - mov qword ptr [rsp+1C],00000002 { 2 }
;Nino2.exe+4F98EC - 80 BA 44 61 00 00 00  - cmp byte ptr [rdx+00006144],00 { 0 }		<<<Intercept here
;Nino2.exe+4F98F3 - 48 89 4C 24 10        - mov [rsp+10],rcx
;Nino2.exe+4F98F8 - 89 44 24 18           - mov [rsp+18],eax
;Nino2.exe+4F98FC - 74 4E                 - je Nino2.exe+4F994C							<<<return here
;Nino2.exe+4F98FE - 0FB7 82 40610000      - movzx eax,word ptr [rdx+00006140]
;Nino2.exe+4F9905 - B9 00020000           - mov ecx,00000200 { 512 }
	mov [g_HUDaddress],rdx
	cmp byte ptr [rdx+00006144h],00
	mov [rsp+10h],rcx
	mov dword ptr [rsp+18h],eax
	jmp qword ptr [_HUDinterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
HUDinterceptor ENDP


END