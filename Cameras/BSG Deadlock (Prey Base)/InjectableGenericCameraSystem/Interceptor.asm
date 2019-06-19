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
.model flat,C
.stack 4096
;---------------------------------------------------------------
; Public definitions so the linker knows which names are present in this file
PUBLIC cameraStructInterceptor
PUBLIC cameraWrite1Interceptor
PUBLIC cameraWrite2Interceptor
;PUBLIC cameraWrite3Interceptor
;PUBLIC fovReadInterceptor
;---------------------------------------------------------------

;---------------------------------------------------------------
; Externs which are used and set by the system. Read / write these
; values in asm to communicate with the system
EXTERN g_cameraEnabled: byte
EXTERN g_cameraStructAddress: dword
EXTERN g_secondQuaternion: dword
;EXTERN g_fovConstructAddress: dword
;EXTERN g_timestopStructAddress: dword
;---------------------------------------------------------------

;---------------------------------------------------------------
; Own externs, defined in InterceptorHelper.cpp
EXTERN _cameraStructInterceptionContinue: dword
EXTERN _cameraWrite1InterceptionContinue: dword
EXTERN _cameraWrite2InterceptionContinue: dword
;EXTERN _cameraWrite3InterceptionContinue: dword
;EXTERN _fovReadInterceptionContinue: dword
;EXTERN _timestopInterceptionContinue: dword
.data

.code


cameraStructInterceptor PROC
;UnityPlayer.dll+8DFFF - 0F11 46 10             - movups [esi+10],xmm0
;UnityPlayer.dll+8E003 - 0F11 6E 20             - movups [esi+20],xmm5
;UnityPlayer.dll+8E007 - 0F 57 0C C7            - xorps xmm1,[edi+eax*8]	<<inject here and this is the code to read cam struct
;UnityPlayer.dll+8E00B - 66 0F 70 D9 AA         - pshufd xmm3,xmm1,-56
;UnityPlayer.dll+8E010 - 66 0F 70 D1 00         - pshufd xmm2,xmm1,00
;UnityPlayer.dll+8E015 - 0F59 16                - mulps xmm2,[esi]			<<return here
;UnityPlayer.dll+8E018 - 66 0F70 C1 55          - pshufd xmm0,xmm1,55
;UnityPlayer.dll+8E01D - 0F10 4E 10             - movups xmm1,[esi+10]
	xorps xmm1,[edi+eax*8]
	push edx
	lea edx,[edi+eax*8]
	mov [g_cameraStructAddress],edx
	pop edx
	pshufd xmm3,xmm1,-56
	pshufd xmm2,xmm1,00
	jmp dword ptr [_cameraStructInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraStructInterceptor ENDP


cameraWrite1Interceptor PROC
;UnityPlayer.dll+929FB - 0F55 0D 508BF90F      - andnps xmm1,[UnityPlayer.dll+EA8B50] { (0) }
;UnityPlayer.dll+92A02 - 0F56 D1               - orps xmm2,xmm1
;UnityPlayer.dll+92A05 - 0FC2 C2 04            - cmpps xmm0,xmm204 { 4 }
;UnityPlayer.dll+92A09 - 0F11 51 10            - movups [ecx+10],xmm2				<<inject here
;UnityPlayer.dll+92A0D - 0F50 C0               - movmskps eax,xmm0
;UnityPlayer.dll+92A10 - 85 C0                 - test eax,eax						<<return here
	cmp byte ptr [g_cameraEnabled], 1
	je exit
originalCode:
	movups [ecx+10h],xmm2
exit:
	movmskps eax,xmm0
	jmp dword ptr [_cameraWrite1InterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraWrite1Interceptor ENDP

cameraWrite2Interceptor PROC
;UnityPlayer.dll+51A1E6 - 0F5E D0               - divps xmm2,xmm0
;UnityPlayer.dll+51A1E9 - 0F54 D1               - andps xmm2,xmm1
;UnityPlayer.dll+51A1EC - 0F55 CC               - andnps xmm1,xmm4
;UnityPlayer.dll+51A1EF - 0F56 D1               - orps xmm2,xmm1
;UnityPlayer.dll+51A1F2 - 0F11 50 74            - movups [eax+74],xmm2			<<inject here
;UnityPlayer.dll+51A1F6 - 8B 46 28              - mov eax,[esi+28]
;UnityPlayer.dll+51A1F9 - 0F10 40 34            - movups xmm0,[eax+34]			<<< return here
;UnityPlayer.dll+51A1FD - 0F11 80 84000000      - movups [eax+00000084],xmm0
;UnityPlayer.dll+51A204 - 0F10 40 44            - movups xmm0,[eax+44]
;UnityPlayer.dll+51A208 - 0F11 80 94000000      - movups [eax+00000094],xmm0
	mov [g_secondQuaternion],eax
	cmp byte ptr [g_cameraEnabled], 1
	je exit
originalcode:
	movups [eax+74h],xmm2
exit:
	mov eax,[esi+28h]
	jmp dword ptr [_cameraWrite2InterceptionContinue]
cameraWrite2Interceptor ENDP

END