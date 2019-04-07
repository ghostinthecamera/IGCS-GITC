;////////////////////////////////////////////////////////////////////////////////////////////////////////
;// Part of Injectable Generic Camera System
;// Copyright(c) 2016, Frans Bouma
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
;---------------------------------------------------------------

;---------------------------------------------------------------
; Externs defined in Core.cpp, which are used and set by the system. Read / write these
; values in asm to communicate with the system
EXTERN _cameraStructAddress: dword
;EXTERN _cameraQuaternionWrite: dword
;EXTERN _gamespeedStructAddress: dword
EXTERN _cameraEnabled: byte
;EXTERN _timeStopped: byte
;---------------------------------------------------------------

;---------------------------------------------------------------
; Own externs, defined in InterceptorHelper.cpp
EXTERN _cameraStructInterceptionContinue: dword
EXTERN _cameraQuaternionWriteInterceptionContinue: dword
;EXTERN _cameraWriteInterceptionContinue2: dword
;EXTERN _fovWriteInterceptionContinue1: dword
;EXTERN _fovWriteInterceptionContinue2: dword
;EXTERN _gamespeedInterceptionContinue: dword


.code 
cameraAddressInterceptor PROC
;UnityPlayer.dll+923EC - 03 47 10              - add eax,[edi+10]
;UnityPlayer.dll+923EF - 0F10 00               - movups xmm0,[eax]
;UnityPlayer.dll+923F2 - 0FC2 C1 04            - cmpps xmm0,xmm104 { 4 }
;UnityPlayer.dll+923F6 - 0F11 08               - movups [eax],xmm1				<<inject here
;UnityPlayer.dll+923F9 - 0F50 C0               - movmskps eax,xmm0				
;UnityPlayer.dll+923FC - A8 07                 - test al,07 { 7 }				<<return here
	; Game jmps to this location due to the hook set in C function SetCameraStructInterceptorHook
	mov [_cameraStructAddress],eax						; intercept address of camera struct
	cmp byte ptr [_cameraEnabled], 1
	je exit
originalcode:
	movups dword ptr [eax],xmm1
exit:
	movmskps eax,xmm0
	jmp [_cameraStructInterceptionContinue]				; jmp back into the original game code, which is the location after the original statements above.
cameraAddressInterceptor ENDP


cameraQuaternionWrite PROC
;UnityPlayer.dll+929FB - 0F55 0D 508BF90F      - andnps xmm1,[UnityPlayer.dll+EA8B50] { (0) }
;UnityPlayer.dll+92A02 - 0F56 D1               - orps xmm2,xmm1
;UnityPlayer.dll+92A05 - 0FC2 C2 04            - cmpps xmm0,xmm204 { 4 }
;UnityPlayer.dll+92A09 - 0F11 51 10            - movups [ecx+10],xmm2				<<inject here
;UnityPlayer.dll+92A0D - 0F50 C0               - movmskps eax,xmm0
;UnityPlayer.dll+92A10 - 85 C0                 - test eax,eax						<<return here
	cmp byte ptr [_cameraEnabled], 1
	je exit
originalCode:
	movups dword ptr [ecx+10h],xmm2
exit:
	movmskps eax,xmm0
	jmp [_cameraQuaternionWriteInterceptionContinue]
cameraQuaternionWrite ENDP

;_TEXT ENDS

END