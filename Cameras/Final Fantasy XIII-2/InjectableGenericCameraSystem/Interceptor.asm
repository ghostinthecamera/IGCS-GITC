;////////////////////////////////////////////////////////////////////////////////////////////////////////
;// Part of Injectable Generic Camera System
;// Copyright(c) 2020, Frans Bouma
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
.model flat,C
.stack 4096
;---------------------------------------------------------------
; Public definitions so the linker knows which names are present in this file
PUBLIC cameraStructInterceptor
PUBLIC cameraStructInterceptor2
PUBLIC cameraWrite1Interceptor
PUBLIC cameraWrite1Interceptor2
PUBLIC timestopInterceptor

;---------------------------------------------------------------

;---------------------------------------------------------------
; Externs which are used and set by the system. Read / write these
; values in asm to communicate with the system
EXTERN g_cameraEnabled: byte
EXTERN g_cameraStructAddress: dword
EXTERN g_cameraStructAddress2: dword
EXTERN g_timescaleaddress: dword



;---------------------------------------------------------------

;---------------------------------------------------------------
; Own externs, defined in InterceptorHelper.cpp
EXTERN _cameraStructInterceptionContinue: qword
EXTERN _cameraStruct2InterceptionContinue: qword
EXTERN _cameraWrite1InterceptionContinue: qword
EXTERN _cameraWrite1InterceptionContinue2: qword
EXTERN _timestopInterceptionContinue: qword


.data

.code


cameraStructInterceptor PROC
;ffxiii2img.exe+1DB0CD - 68 371B0000           - push 00001B37 { 6967 }
;ffxiii2img.exe+1DB0D2 - 68 E8CB9E00           - push ffxiii2img.exe+97CBE8 { ("C:\project\tredi-alba\alba\src\app\common\scene\Scene.cpp") }
;ffxiii2img.exe+1DB0D7 - E8 74F71600           - call ffxiii2img.exe+34A850
;ffxiii2img.exe+1DB0DC - 83 C4 0C              - add esp,0C { 12 }
;ffxiii2img.exe+1DB0DF - 8B 86 C4000000        - mov eax,[esi+000000C4]			<<inject/read edi, has the current active cam
;ffxiii2img.exe+1DB0E5 - 8B 8E C8000000        - mov ecx,[esi+000000C8]		
;ffxiii2img.exe+1DB0EB - 3B C1                 - cmp eax,ecx
;ffxiii2img.exe+1DB0ED - 0F8D 4B040000         - jnl ffxiii2img.exe+1DB53E		<<return
;ffxiii2img.exe+1DB0F3 - 8B 56 4C              - mov edx,[esi+4C]
	mov eax,[esi+000000C4h]
	mov ecx,[esi+000000C8h]
	cmp eax,ecx
	mov [g_cameraStructAddress],edi
	jmp dword ptr [_cameraStructInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraStructInterceptor ENDP

cameraStructInterceptor2 PROC
;this is for the viewmatrix - but we have the rotation matrices
;ffxiii2img.exe+6B6C71 - 6A 00                 - push 00 { 0 }
;ffxiii2img.exe+6B6C73 - 8B CE                 - mov ecx,esi
;ffxiii2img.exe+6B6C75 - FF D2                 - call edx
;ffxiii2img.exe+6B6C77 - 8B 4F 04              - mov ecx,[edi+04]			<<inject
;ffxiii2img.exe+6B6C7A - 89 81 B0000000        - mov [ecx+000000B0],eax			<<eax
;ffxiii2img.exe+6B6C80 - 89 99 B8000000        - mov [ecx+000000B8],ebx
;ffxiii2img.exe+6B6C86 - 8B 06                 - mov eax,[esi]				<<return
;ffxiii2img.exe+6B6C88 - 8B 50 24              - mov edx,[eax+24]
;ffxiii2img.exe+6B6C8B - 6A 01                 - push 01 { 1 }
;ffxiii2img.exe+6B6C8D - 8B CE                 - mov ecx,esi
;ffxiii2img.exe+6B6C8F - FF D2                 - call edx
	mov ecx,[edi+04h]
	mov [ecx+000000B0h],eax
	mov [ecx+000000B8h],ebx
	mov [g_cameraStructAddress2],eax
	jmp dword ptr [_cameraStruct2InterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraStructInterceptor2 ENDP




cameraWrite1Interceptor2 PROC
;this is for the viewmatrix - but we have the rotation matrices
;ffxiii2img.exe+6E83BF - CC                    - int 3 
;ffxiii2img.exe+6E83C0 - F3 0F7E 44 24 04      - movq xmm0,[esp+04]				<<INJECT
;ffxiii2img.exe+6E83C6 - 66 0FD6 41 04         - movq [ecx+04],xmm0
;ffxiii2img.exe+6E83CB - F3 0F7E 44 24 0C      - movq xmm0,[esp+0C]
;ffxiii2img.exe+6E83D1 - 66 0FD6 41 0C         - movq [ecx+0C],xmm0
;ffxiii2img.exe+6E83D6 - F3 0F7E 44 24 14      - movq xmm0,[esp+14]
;ffxiii2img.exe+6E83DC - 66 0FD6 41 14         - movq [ecx+14],xmm0
;ffxiii2img.exe+6E83E1 - F3 0F7E 44 24 1C      - movq xmm0,[esp+1C]
;ffxiii2img.exe+6E83E7 - 66 0FD6 41 1C         - movq [ecx+1C],xmm0
;ffxiii2img.exe+6E83EC - F3 0F7E 44 24 24      - movq xmm0,[esp+24]
;ffxiii2img.exe+6E83F2 - 66 0FD6 41 24         - movq [ecx+24],xmm0
;ffxiii2img.exe+6E83F7 - F3 0F7E 44 24 2C      - movq xmm0,[esp+2C]
;ffxiii2img.exe+6E83FD - 66 0FD6 41 2C         - movq [ecx+2C],xmm0
;ffxiii2img.exe+6E8402 - F3 0F7E 44 24 34      - movq xmm0,[esp+34]
;ffxiii2img.exe+6E8408 - 66 0FD6 41 34         - movq [ecx+34],xmm0
;ffxiii2img.exe+6E840D - F3 0F7E 44 24 3C      - movq xmm0,[esp+3C]
;ffxiii2img.exe+6E8413 - B8 00100000           - mov eax,00001000 { 4096 }
;ffxiii2img.exe+6E8418 - 66 0FD6 41 3C         - movq [ecx+3C],xmm0
;ffxiii2img.exe+6E841D - 66 09 81 54010000     - or [ecx+00000154],ax		<<RETURN
;ffxiii2img.exe+6E8424 - C2 4000               - ret 0040 { 64 }
;ffxiii2img.exe+6E8427 - CC                    - int 3 
;ffxiii2img.exe+6E8428 - CC                    - int 3 
	cmp [g_cameraEnabled],1
	jne originalcode
	cmp ecx,[g_cameraStructAddress2]
	je skipwrite
originalcode:
	movq xmm0,qword ptr [esp+04h]
	movq qword ptr [ecx+04h],xmm0
	movq xmm0,qword ptr [esp+0Ch]
	movq qword ptr  [ecx+0Ch],xmm0
	movq xmm0,qword ptr [esp+14h]
	movq qword ptr [ecx+14h],xmm0
	movq xmm0,qword ptr [esp+1Ch]
	movq qword ptr [ecx+1Ch],xmm0
	movq xmm0,qword ptr [esp+24h]
	movq qword ptr [ecx+24h],xmm0
	movq xmm0,qword ptr [esp+2Ch]
	movq qword ptr [ecx+2Ch],xmm0
	movq xmm0,qword ptr [esp+34h]
	movq qword ptr [ecx+34h],xmm0
	movq xmm0,qword ptr [esp+3Ch]
	mov eax,00001000h
	movq qword ptr [ecx+3Ch],xmm0
	jmp exit
skipwrite:
    jmp exit
    movq xmm0,qword ptr [esp+04h]
	;movq qword ptr [ecx+04h],xmm0
	movq xmm0,qword ptr [esp+0Ch]
	;movq qword ptr  [ecx+0Ch],xmm0
	movq xmm0,qword ptr [esp+14h]
	;movq qword ptr [ecx+14h],xmm0
	movq xmm0,qword ptr [esp+1Ch]
	;movq qword ptr [ecx+1Ch],xmm0
	movq xmm0,qword ptr [esp+24h]
	;movq qword ptr [ecx+24h],xmm0
	movq xmm0,qword ptr [esp+2Ch]
	;movq qword ptr [ecx+2Ch],xmm0
	movq xmm0,qword ptr [esp+34h]
	;movq qword ptr [ecx+34h],xmm0
	movq xmm0,qword ptr [esp+3Ch]
	mov eax,00001000h
	;movq qword ptr [ecx+3Ch],xmm0
exit:
	jmp dword ptr [_cameraWrite1InterceptionContinue2]
cameraWrite1Interceptor2 ENDP




cameraWrite1Interceptor PROC
;ffxiii2img.exe+68701A - CC                    - int 3 
;ffxiii2img.exe+68701B - CC                    - int 3 
;ffxiii2img.exe+68701C - CC                    - int 3 
;ffxiii2img.exe+68701D - CC                    - int 3 
;ffxiii2img.exe+68701E - CC                    - int 3 
;ffxiii2img.exe+68701F - CC                    - int 3 
;ffxiii2img.exe+687020 - 8B C1                 - mov eax,ecx
;ffxiii2img.exe+687022 - 8B 4C 24 04           - mov ecx,[esp+04]
;ffxiii2img.exe+687026 - F3 0F7E 01            - movq xmm0,[ecx]			<<INJECT // nopping call to function crashes the game
;ffxiii2img.exe+68702A - 66 0FD6 00            - movq [eax],xmm0
;ffxiii2img.exe+68702E - F3 0F7E 41 08         - movq xmm0,[ecx+08]
;ffxiii2img.exe+687033 - 66 0FD6 40 08         - movq [eax+08],xmm0
;ffxiii2img.exe+687038 - F3 0F7E 41 10         - movq xmm0,[ecx+10]
;ffxiii2img.exe+68703D - 66 0FD6 40 10         - movq [eax+10],xmm0
;ffxiii2img.exe+687042 - F3 0F7E 41 18         - movq xmm0,[ecx+18]
;ffxiii2img.exe+687047 - 66 0FD6 40 18         - movq [eax+18],xmm0
;ffxiii2img.exe+68704C - F3 0F7E 41 20         - movq xmm0,[ecx+20]
;ffxiii2img.exe+687051 - 66 0FD6 40 20         - movq [eax+20],xmm0
;ffxiii2img.exe+687056 - F3 0F7E 41 28         - movq xmm0,[ecx+28]
;ffxiii2img.exe+68705B - 66 0FD6 40 28         - movq [eax+28],xmm0
;ffxiii2img.exe+687060 - F3 0F7E 41 30         - movq xmm0,[ecx+30]
;ffxiii2img.exe+687065 - 83 C1 30              - add ecx,30 { 48 }
;ffxiii2img.exe+687068 - 66 0FD6 40 30         - movq [eax+30],xmm0
;ffxiii2img.exe+68706D - F3 0F7E 41 08         - movq xmm0,[ecx+08]
;ffxiii2img.exe+687072 - 66 0FD6 40 38         - movq [eax+38],xmm0			<<RETURN
;ffxiii2img.exe+687077 - C2 0400               - ret 0004 { 4 }
;ffxiii2img.exe+68707A - CC                    - int 3 
;ffxiii2img.exe+68707B - CC                    - int 3 
;ffxiii2img.exe+68707C - CC                    - int 3 
;ffxiii2img.exe+68707D - CC                    - int 3 
	cmp [g_cameraEnabled],1
	jne originalcode

	push ebx
	mov ebx, [g_cameraStructAddress]
	add ebx,078h
	cmp ebx,eax
	pop ebx
	je skipwrite

	;push ebx
	;mov ebx, [g_cameraStructAddress]
	;add ebx,0B8h
	;cmp ebx,eax
	;pop ebx
	;je skipwrite

	;push ebx
	;mov ebx, [g_cameraStructAddress]
	;add ebx,150h
	;cmp ebx,eax
	;pop ebx
	;je skipwrite

originalcode:
	movq xmm0,qword ptr [ecx]
	movq qword ptr [eax],xmm0
	movq xmm0,qword ptr [ecx+08h]
	movq qword ptr [eax+08h],xmm0
	movq xmm0,qword ptr [ecx+10h]
	movq qword ptr [eax+10h],xmm0
	movq xmm0,qword ptr [ecx+18h]
	movq qword ptr [eax+18h],xmm0
	movq xmm0,qword ptr [ecx+20h]
	movq qword ptr [eax+20h],xmm0
	movq xmm0,qword ptr [ecx+28h]
	movq qword ptr [eax+28h],xmm0
	movq xmm0,qword ptr [ecx+30h]
	add ecx,30h
	movq qword ptr [eax+30h],xmm0
	movq xmm0,qword ptr [ecx+08h]
	movq qword ptr [eax+38h],xmm0
	jmp exit
skipwrite:
    jmp exit
    movq xmm0,qword ptr [ecx]
	;movq qword ptr [eax],xmm0
	movq xmm0,qword ptr [ecx+08h]
	;movq qword ptr [eax+08h],xmm0
	movq xmm0,qword ptr [ecx+10h]
	;movq qword ptr [eax+10h],xmm0
	movq xmm0,qword ptr [ecx+18h]
	;movq qword ptr [eax+18h],xmm0
	movq xmm0,qword ptr [ecx+20h]
	;movq qword ptr [eax+20h],xmm0
	movq xmm0,qword ptr [ecx+28h]
	;movq qword ptr [eax+28h],xmm0
	movq xmm0,qword ptr [ecx+30h]
	add ecx,30h
	;movq qword ptr [eax+30h],xmm0
	movq xmm0,qword ptr [ecx+08h]
	;movq qword ptr [eax+38h],xmm0
exit:
	jmp dword ptr [_cameraWrite1InterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraWrite1Interceptor ENDP



timestopInterceptor PROC
;ffxiii2img.exe+804F61 - 8B 10                 - mov edx,[eax]
;ffxiii2img.exe+804F63 - 8B C8                 - mov ecx,eax
;ffxiii2img.exe+804F65 - 8B 42 3C              - mov eax,[edx+3C]
;ffxiii2img.exe+804F68 - FF D0                 - call eax			<<<call to load timestop (in ecx)
;ffxiii2img.exe+804F6A - D9 5C 24 14           - fstp dword ptr [esp+14]	<<inject
;ffxiii2img.exe+804F6E - F3 0F2A 44 24 0C      - cvtsi2ss xmm0,[esp+0C]
;ffxiii2img.exe+804F74 - F3 0F59 44 24 14      - mulss xmm0,[esp+14]
;ffxiii2img.exe+804F7A - 8B 16                 - mov edx,[esi]				<<return
;ffxiii2img.exe+804F7C - 8B 42 14              - mov eax,[edx+14]
;ffxiii2img.exe+804F7F - F3 0F2C C8            - cvttss2si ecx,xmm0
	mov [g_timescaleaddress],ecx
	fstp dword ptr [esp+14h]
	cvtsi2ss xmm0,dword ptr [esp+0Ch]
	mulss xmm0,dword ptr[esp+14h]
	jmp dword ptr [_timestopInterceptionContinue]
timestopInterceptor ENDP

END