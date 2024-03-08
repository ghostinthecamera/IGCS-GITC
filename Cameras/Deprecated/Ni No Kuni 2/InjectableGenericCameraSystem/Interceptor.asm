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
PUBLIC activeCameraInterceptor
PUBLIC cameraStruct
PUBLIC cameraWrite
PUBLIC timescaleInterceptor
PUBLIC HUDinterceptor

;---------------------------------------------------------------

;---------------------------------------------------------------
; Externs which are used and set by the system. Read / write these
; values in asm to communicate with the system
EXTERN g_cameraEnabled: byte
EXTERN g_cameraStructAddress: qword
EXTERN g_activecamAddress: qword
EXTERN g_timescaleAddress: qword
EXTERN g_HUDaddress: qword

;---------------------------------------------------------------

;---------------------------------------------------------------
; Own externs, defined in InterceptorHelper.cpp
EXTERN _activeCameraContinue: qword
EXTERN _timescaleInterceptionContinue: qword
EXTERN _HUDinterceptionContinue: qword
EXTERN _cameraStructContinue: qword
EXTERN _cameraWriteContinue: qword

.data


.code

timevalue dd 0.0

cameraStruct PROC
;Nino2.exe+496870 - C3                    - ret 
;Nino2.exe+496871 - 0FB7 C0               - movzx eax,ax
;Nino2.exe+496874 - 48 FF C8              - dec rax
;Nino2.exe+496877 - 8B C0                 - mov eax,eax						<<insert
;Nino2.exe+496879 - 4C 8D 04 40           - lea r8,[rax+rax*2]
;Nino2.exe+49687D - 49 C1 E0 05           - shl r8,05 { 5 }
;Nino2.exe+496881 - 4C 03 41 10           - add r8,[rcx+10]					<<value at rcx+10 is cam
;Nino2.exe+496885 - 4D 85 C9              - test r9,r9						<<return
;Nino2.exe+496888 - 74 09                 - je Nino2.exe+496893
;Nino2.exe+49688A - 41 0FB7 40 3C         - movzx eax,word ptr [r8+3C]
;Nino2.exe+49688F - 66 41 89 01           - mov [r9],ax
	mov eax,eax
	lea r8,[rax+rax*2h]
	shl r8,05
	add r8,[rcx+10h]
	push rdi
	mov rdi,[rcx+10h]
	mov [g_cameraStructAddress],rdi
	pop rdi
	jmp qword ptr [_cameraStructContinue]
cameraStruct ENDP

cameraWrite PROC
;Nino2.exe+4968EA - 8B C0                 - mov eax,eax
;Nino2.exe+4968EC - 48 8D 04 40           - lea rax,[rax+rax*2]
;Nino2.exe+4968F0 - 48 C1 E0 05           - shl rax,05 { 5 }
;Nino2.exe+4968F4 - 48 03 41 10           - add rax,[rcx+10]
;Nino2.exe+4968F8 - B9 FF7F0000           - mov ecx,00007FFF { 32767 }
;Nino2.exe+4968FD - 0F29 00               - movaps [rax],xmm0			<<inject
;Nino2.exe+496900 - 41 0F28 48 10         - movaps xmm1,[r8+10]
;Nino2.exe+496905 - 0F29 48 10            - movaps [rax+10],xmm1
;Nino2.exe+496909 - 41 0F28 40 20         - movaps xmm0,[r8+20]
;Nino2.exe+49690E - 0F29 40 20            - movaps [rax+20],xmm0
;Nino2.exe+496912 - 66 21 4A 02           - and [rdx+02],cx				<<return
;Nino2.exe+496916 - 0FB6 4C 24 28         - movzx ecx,byte ptr [rsp+28]
;Nino2.exe+49691B - 66 C1 E1 0F           - shl cx,0F { 15 }
;Nino2.exe+49691F - 66 09 4A 02           - or [rdx+02],cx
;Nino2.exe+496923 - 8B 0A                 - mov ecx,[rdx]
	cmp byte ptr [g_cameraEnabled],1
	jne originalcode
	cmp [g_cameraStructAddress],rax
	je skipcode
originalcode:
	movaps [rax],xmm0
	movaps xmm1,[r8+10h]
	movaps [rax+10h],xmm1
	movaps xmm0,[r8+20h]
	movaps [rax+20h],xmm0
	jmp exit
skipcode:
	movaps xmm1,[r8+10h]
	movaps xmm0,[r8+20h]
exit:
	jmp qword ptr [_cameraWriteContinue]
cameraWrite ENDP


activeCameraInterceptor PROC
;Nino2.exe+7CFA38 - 48 33 C4              - xor rax,rsp
;Nino2.exe+7CFA3B - 48 89 45 80           - mov [rbp-80],rax
;Nino2.exe+7CFA3F - 80 B9 AE010000 00     - cmp byte ptr [rcx+000001AE],00 { 0 }
;Nino2.exe+7CFA46 - 48 8B F9              - mov rdi,rcx						<<inject
;Nino2.exe+7CFA49 - 48 8B 71 08           - mov rsi,[rcx+08]					<<rsi has address of cam
;Nino2.exe+7CFA4D - F3 0F10 BE 8C020000   - movss xmm7,[rsi+0000028C]
;Nino2.exe+7CFA55 - F3 0F11 7C 24 48      - movss [rsp+48],xmm7				<<RETURN
;Nino2.exe+7CFA5B - 74 05                 - je Nino2.exe+7CFA62
	mov rdi,rcx
	mov rsi,[rcx+08h]
	mov [g_activecamAddress],rsi
	movss xmm7,dword ptr [rsi+0000028Ch]
	jmp qword ptr [_activeCameraContinue]	; jmp back into the original game code, which is the location after the original statements above.
activeCameraInterceptor ENDP

timescaleInterceptor PROC	
;Nino2.exe+9D6165 - F3 0F10 88 187C3500   - movss xmm1,[rax+00357C18]			<<INJECT
;Nino2.exe+9D616D - F3 0F10 90 1C7C3500   - movss xmm2,[rax+00357C1C]			<<<timestop
;Nino2.exe+9D6175 - 0FB7 80 287C3500      - movzx eax,word ptr [rax+00357C28]	<<return
;Nino2.exe+9D617C - F3 0F5C D1            - subss xmm2,xmm1
;Nino2.exe+9D6180 - 66 0F6E C0            - movd xmm0,eax
;Nino2.exe+9D6184 - 0F5B C0               - cvtdq2ps xmm0,xmm0
;Nino2.exe+9D6187 - F3 0F59 D0            - mulss xmm2,xmm0
;Nino2.exe+9D618B - F3 0F59 15 9D2D4100   - mulss xmm2,[Nino2.exe+DE8F30] { (0.00) }
	movss xmm1, dword ptr [rax+00357C18h]
	movss xmm2, dword ptr [rax+00357C1Ch]
	mov [g_timescaleAddress], rax
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