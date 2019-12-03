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
PUBLIC cameraStructInterceptor2
PUBLIC cameraWrite1Interceptor
PUBLIC cameraWrite2Interceptor

;---------------------------------------------------------------

;---------------------------------------------------------------
; Externs which are used and set by the system. Read / write these
; values in asm to communicate with the system
EXTERN g_cameraEnabled: byte
EXTERN g_cameraStructAddress: qword
EXTERN g_cameraStructAddress2: qword

;---------------------------------------------------------------

;---------------------------------------------------------------
; Own externs, defined in InterceptorHelper.cpp
EXTERN _cameraStructInterceptionContinue: qword
EXTERN _cameraStructInterception2Continue: qword
EXTERN _cameraWrite1InterceptionContinue: qword
EXTERN _cameraWrite2InterceptionContinue: qword

.data

.code

cameraStructInterceptor2 PROC
; camera address interceptor is also a the FoV write blocker.
;dirtrally2.exe+9F1FA5 - 57                    - push rdi
;dirtrally2.exe+9F1FA6 - 48 83 EC 50           - sub rsp,50 { 80 }
;dirtrally2.exe+9F1FAA - 48 8B 01              - mov rax,[rcx]
;dirtrally2.exe+9F1FAD - 48 8B DA              - mov rbx,rdx
;dirtrally2.exe+9F1FB0 - FF 90 C0000000        - call qword ptr [rax+000000C0]  <<inject here
;dirtrally2.exe+9F1FB6 - 48 8B D0              - mov rdx,rax
;dirtrally2.exe+9F1FB9 - 48 8D 4C 24 20        - lea rcx,[rsp+20]
;dirtrally2.exe+9F1FBE - 48 8B F8              - mov rdi,rax
;dirtrally2.exe+9F1FC1 - E8 AA5E6BFF           - call dirtrally2.exe+A7E70
;dirtrally2.exe+9F1FC6 - 0F28 00               - movaps xmm0,[rax]
;dirtrally2.exe+9F1FC9 - 66 0F7F 43 20         - movdqa [rbx+20],xmm0
	call qword ptr [rax+000000C0h]
	mov [g_cameraStructAddress2], rax
	mov rdx,rax
	lea rcx,[rsp+20h]
	jmp qword ptr [_cameraStructInterception2Continue]	; jmp back into the original game code, which is the location after the original statements above.
cameraStructInterceptor2 ENDP

cameraStructInterceptor PROC
; camera address interceptor also blocks writes to matrix
;dirtrally2.exe+9F1FA5 - 57                    - push rdi
;dirtrally2.exe+9F1FA6 - 48 83 EC 50           - sub rsp,50 { 80 }
;dirtrally2.exe+9F1FAA - 48 8B 01              - mov rax,[rcx]
;dirtrally2.exe+9F1FAD - 48 8B DA              - mov rbx,rdx
;dirtrally2.exe+9F1FB0 - FF 90 C0000000        - call qword ptr [rax+000000C0]  
;dirtrally2.exe+9F1FB6 - 48 8B D0              - mov rdx,rax
;dirtrally2.exe+9F1FB9 - 48 8D 4C 24 20        - lea rcx,[rsp+20]
;dirtrally2.exe+9F1FBE - 48 8B F8              - mov rdi,rax
;dirtrally2.exe+9F1FC1 - E8 AA5E6BFF           - call dirtrally2.exe+A7E70
;dirtrally2.exe+9F1FC6 - 0F28 00               - movaps xmm0,[rax]		<<inject here
;dirtrally2.exe+9F1FC9 - 66 0F7F 43 20         - movdqa [rbx+20],xmm0		<<< matrix X row write
;dirtrally2.exe+9F1FCE - 0F28 48 10            - movaps xmm1,[rax+10]
;dirtrally2.exe+9F1FD2 - 66 0F7F 4B 10         - movdqa [rbx+10],xmm1		<<< matrix y row write
;dirtrally2.exe+9F1FD7 - 0F28 40 20            - movaps xmm0,[rax+20]
;dirtrally2.exe+9F1FDB - 66 0F7F 43 30         - movdqa [rbx+30],xmm0		<<< matrix z row write
;dirtrally2.exe+9F1FE0 - 0F57 C0               - xorps xmm0,xmm0
;dirtrally2.exe+9F1FE3 - 0F28 4F 10            - movaps xmm1,[rdi+10]
;dirtrally2.exe+9F1FE7 - 66 0F7F 4B 40         - movdqa [rbx+40],xmm1		<<< camera coord write
;dirtrally2.exe+9F1FEC - F3 0F10 57 70         - movss xmm2,[rdi+70]
;dirtrally2.exe+9F1FF1 - 0F2F D0               - comiss xmm2,xmm0		<<return
;dirtrally2.exe+9F1FF4 - 76 09                 - jna dirtrally2.exe+9F1FFF
;dirtrally2.exe+9F1FF6 - 0F2F 15 2BBA6700      - comiss xmm2,[dirtrally2.exe+106DA28] { (6.28) }
;dirtrally2.exe+9F1FFD - 76 05                 - jna dirtrally2.exe+9F2004
	mov [g_cameraStructAddress], rbx
	cmp [g_cameraEnabled], 1
	je skipwrites		;check if camera enabled, if so, we jump to the code with the writes removed/commented out
	movaps xmm0,[rax]
	movdqa [rbx+20h],xmm0
	movaps xmm1,[rax+10h]
	movdqa [rbx+10h],xmm1
	movaps xmm0,[rax+20h]
	movdqa [rbx+30h],xmm0
	xorps xmm0,xmm0
	movaps xmm1,[rdi+10h]
	movdqa [rbx+40h],xmm1
	movss xmm2,dword ptr [rdi+70h]
	jmp exit
skipwrites:
	movaps xmm0,[rax]
	;movdqa [rbx+20h],xmm0			<<write commented out
	movaps xmm1,[rax+10h]
	;movdqa [rbx+10h],xmm1			<<write commented out
	movaps xmm0,[rax+20h]
	;movdqa [rbx+30h],xmm0			<<write commented out
	xorps xmm0,xmm0
	movaps xmm1,[rdi+10h]
	;movdqa [rbx+40h],xmm1			<<write commented out
	movss xmm2,dword ptr [rdi+70h]
exit:
	jmp qword ptr [_cameraStructInterceptionContinue]	;jmp back into the original game code, which is the location after the original statements above.
cameraStructInterceptor ENDP

cameraWrite1Interceptor PROC
;fov write
;AOB: F3 0F 11 53 ?? 8B 47 ?? 89 43 ?? 0F 28 47 ?? 66 0F 7F 43 ?? 48 8B 5C 24 ?? 48 83 C4 ?? 5F C3
;dirtrally2.exe+9F1FFF - F3 0F10 53 50         - movss xmm2,[rbx+50]  
;dirtrally2.exe+9F2004 - F3 0F11 53 50         - movss [rbx+50],xmm2		<<inject here
;dirtrally2.exe+9F2009 - 8B 47 78              - mov eax,[rdi+78]
;dirtrally2.exe+9F200C - 89 43 54              - mov [rbx+54],eax
;dirtrally2.exe+9F200F - 0F28 47 50            - movaps xmm0,[rdi+50]		<<fov write	
;dirtrally2.exe+9F2013 - 66 0F7F 43 60         - movdqa [rbx+60],xmm0		<<return here
;dirtrally2.exe+9F2018 - 48 8B 5C 24 60        - mov rbx,[rsp+60]
;dirtrally2.exe+9F201D - 48 83 C4 50           - add rsp,50 { 80 }
;dirtrally2.exe+9F2021 - 5F                    - pop rdi
	cmp [g_cameraEnabled], 1
	je exit
	movss dword ptr [rbx+50h],xmm2
exit:
	mov eax,[rdi+78h]
	mov [rbx+54h],eax
	movaps xmm0,[rdi+50h]
	jmp qword ptr [_cameraWrite1InterceptionContinue]  ;jmp back into the original game code, which is the location after the original statements above.
cameraWrite1Interceptor ENDP

cameraWrite2Interceptor PROC
;AOB: 0F 29 03 F3 0F 5C 4B ??
;dirtrally2.exe+A02DC3 - 0F28 45 00            - movaps xmm0,[rbp+00]
;dirtrally2.exe+A02DC7 - F3 0F10 4D A0         - movss xmm1,[rbp-60]		
;dirtrally2.exe+A02DCC - 0F29 03               - movaps [rbx],xmm0          <<< inject here/skip this
;dirtrally2.exe+A02DCF - F3 0F5C 4B 70         - subss xmm1,[rbx+70]
;dirtrally2.exe+A02DD4 - F3 0F59 CF            - mulss xmm1,xmm7			
;dirtrally2.exe+A02DD8 - F3 0F58 4B 70         - addss xmm1,dword ptr [rbx+70]	<<< return here
;dirtrally2.exe+A02DDD - F3 0F11 4B 70         - movss [rbx+70],xmm1
;dirtrally2.exe+A02DE2 - F3 0F10 4D A8         - movss xmm1,[rbp-58]
	cmp [g_cameraEnabled], 1
	je exit
	movaps [rbx],xmm0 
exit:
	subss xmm1,dword ptr [rbx+70h]
	mulss xmm1,xmm7
	addss xmm1,dword ptr [rbx+70h]
	jmp qword ptr [_cameraWrite2InterceptionContinue]  ;jmp back into the original game code, which is the location after the original statements above.
cameraWrite2Interceptor ENDP


END