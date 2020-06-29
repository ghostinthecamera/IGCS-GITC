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

;---------------------------------------------------------------
; Public definitions so the linker knows which names are present in this file
PUBLIC cameraStructInterceptor
PUBLIC timestopReadInterceptor
;---------------------------------------------------------------

;---------------------------------------------------------------
; Externs which are used and set by the system. Read / write these
; values in asm to communicate with the system
EXTERN g_cameraEnabled: byte
EXTERN g_cameraStructAddress: qword
EXTERN g_timestopStructAddress: qword
EXTERN _timestopAbsolute: qword

;---------------------------------------------------------------

;---------------------------------------------------------------
; Own externs, defined in InterceptorHelper.cpp
EXTERN _cameraStructInterceptionContinue: qword
EXTERN _timestopReadInterceptionContinue: qword

.data

.code

cameraStructInterceptor PROC
;for IGCS Camera: 41 0F 28 C4 41 0F 28 CC 0F 55 44 24 ??
;Grid_dx12.exe+B1F8B2 - 41 0F28 C4            - movaps xmm0,xmm12  <<inject here and copy RSI
;Grid_dx12.exe+B1F8B6 - 41 0F28 CC            - movaps xmm1,xmm12
;Grid_dx12.exe+B1F8BA - 0F55 44 24 20         - andnps xmm0,[rsp+20]
;Grid_dx12.exe+B1F8BF - 41 0F54 FC            - andps xmm7,xmm12
;Grid_dx12.exe+B1F8C3 - 0F55 4C 24 30         - andnps xmm1,[rsp+30]
;Grid_dx12.exe+B1F8C8 - 0F29 44 24 70         - movaps [rsp+70],xmm0
;Grid_dx12.exe+B1F8CD - 41 0F28 C4            - movaps xmm0,xmm12
	mov [g_cameraStructAddress], rsi
	movaps xmm0,xmm12
	movaps xmm1,xmm12
	andnps xmm0,[rsp+20h]
	andps xmm7,xmm12
	jmp qword ptr [_cameraStructInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraStructInterceptor ENDP

timestopReadInterceptor PROC
;Grid_dx12.exe+1EF132 - 74 0D                 - je Grid_dx12.exe+1EF141
;Grid_dx12.exe+1EF134 - 0F57 C9               - xorps xmm1,xmm1
;Grid_dx12.exe+1EF137 - E8 44249500           - call Grid_dx12.exe+B41580
;Grid_dx12.exe+1EF13C - E9 C9010000           - jmp Grid_dx12.exe+1EF30A
;Grid_dx12.exe+1EF141 - F3 0F10 4B 54         - movss xmm1,[rbx+54]				<<< inject here
;Grid_dx12.exe+1EF146 - 48 8B 4B 58           - mov rcx,[rbx+58]
;Grid_dx12.exe+1EF14A - E8 31249500           - call Grid_dx12.exe+B41580
;Grid_dx12.exe+1EF14F - E9 B6010000           - jmp Grid_dx12.exe+1EF30A			<<<return here
;Grid_dx12.exe+1EF154 - F3 0F10 43 18         - movss xmm0,[rbx+18]
;Grid_dx12.exe+1EF159 - 0F2E C6               - ucomiss xmm0,xmm6
	mov [g_timestopStructAddress], rbx
	movss xmm1, dword ptr [rbx+54h]
	mov rcx,[rbx+58h]
	call [_timestopAbsolute]
exit:
	jmp qword ptr [_timestopReadInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
timestopReadInterceptor ENDP

END