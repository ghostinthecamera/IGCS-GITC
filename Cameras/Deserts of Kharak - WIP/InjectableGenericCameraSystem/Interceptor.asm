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
;PUBLIC cameraWrite1Interceptor
;PUBLIC cameraWrite2Interceptor
;PUBLIC resolutionScaleReadInterceptor
;PUBLIC todWriteInterceptor
;PUBLIC timestopReadInterceptor
;PUBLIC fogReadInterceptor

;---------------------------------------------------------------

;---------------------------------------------------------------
; Externs which are used and set by the system. Read / write these
; values in asm to communicate with the system
EXTERN g_cameraEnabled: byte
EXTERN g_cameraStructAddress: qword
;EXTERN g_resolutionScaleAddress: qword
;EXTERN g_todStructAddress: qword
;EXTERN g_timestopStructAddress: qword
;EXTERN g_fogStructAddress: qword

;---------------------------------------------------------------

;---------------------------------------------------------------
; Own externs, defined in InterceptorHelper.cpp
EXTERN _cameraStructInterceptionContinue: qword
;EXTERN _cameraWrite1InterceptionContinue: qword
;EXTERN _cameraWrite2InterceptionContinue: qword
;EXTERN _fovReadInterceptionContinue: qword
;EXTERN _resolutionScaleReadInterceptionContinue: qword
;EXTERN _todWriteInterceptionContinue: qword
;EXTERN _timestopReadInterceptionContinue: qword
;EXTERN _fogReadInterceptionContinue: qword

.data


.code


cameraStructInterceptor PROC
; this gets the fov - but also can use RBX-offsets to get camera coords+quaternion. 
;DesertsOfKharak64.exe+ABFF7 - F3 0F10 83 84020000   - movss xmm0,[rbx+00000284]		<<inject here << fov+base
;DesertsOfKharak64.exe+ABFFF - 0F28 F7               - movaps xmm6,xmm7
;DesertsOfKharak64.exe+AC002 - F3 0F5E 05 9E31FC00   - divss xmm0,180.00
;DesertsOfKharak64.exe+AC00A - F3 0F5C F0            - subss xmm6,xmm0					<<return here
	mov [g_cameraStructAddress], rbx
	;mov dword ptr [rbx+324], 01010101
	movss xmm0, dword ptr [rbx+284h]
	movaps xmm6,xmm7
	
	push rax
	mov qword ptr [rax], [43340000h]
	divss xmm0,qword ptr [rax]
	pop rax

exit:
	jmp qword ptr [_cameraStructInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraStructInterceptor ENDP

END