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
PUBLIC borderInterceptor
PUBLIC fovReadInterceptor
;---------------------------------------------------------------

;---------------------------------------------------------------
; Externs which are used and set by the system. Read / write these
; values in asm to communicate with the system
EXTERN g_cameraEnabled: byte
EXTERN g_cameraStructAddress: dword
EXTERN g_fovConstructAddress: dword
;---------------------------------------------------------------

;---------------------------------------------------------------
; Own externs, defined in InterceptorHelper.cpp
EXTERN _cameraStructInterceptionContinue: dword
EXTERN _cameraWrite1InterceptionContinue: dword
EXTERN _borderInterceptionContinue: dword
EXTERN _fovReadInterceptionContinue: dword



.data

.code

bordervalue   dd 0.0

cameraStructInterceptor PROC
;UnityPlayer.dll+923EF - 0F10 00               - movups xmm0,[eax]
;UnityPlayer.dll+923F2 - 0FC2 C1 04            - cmpps xmm0,xmm104 { 4 }
;UnityPlayer.dll+923F6 - 0F11 08               - movups [eax],xmm1    <<<intercept here
;UnityPlayer.dll+923F9 - 0F50 C0               - movmskps eax,xmm0
;UnityPlayer.dll+923FC - A8 07                 - test al,07 { 7 }	<<<return here
	cmp esi, 00000005
	je correctaddress
originalcode:
	movups [eax],xmm1
	movmskps eax,xmm0
	jmp exit
correctaddress:
	mov [g_cameraStructAddress], eax
	cmp byte ptr [g_cameraEnabled], 1
	je writeskip
	movups [eax],xmm1
writeskip:
	movmskps eax,xmm0
exit:
	jmp dword ptr [_cameraStructInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraStructInterceptor ENDP


cameraWrite1Interceptor PROC
;UnityPlayer.dll+929FB - 0F55 0D 508BF90F      - andnps xmm1,[UnityPlayer.dll+EA8B50] { (0) }
;UnityPlayer.dll+92A02 - 0F56 D1               - orps xmm2,xmm1
;UnityPlayer.dll+92A05 - 0FC2 C2 04            - cmpps xmm0,xmm204 { 4 }
;UnityPlayer.dll+92A09 - 0F11 51 10            - movups [ecx+10],xmm2				<<inject here
;UnityPlayer.dll+92A0D - 0F50 C0               - movmskps eax,xmm0
;UnityPlayer.dll+92A10 - 85 C0                 - test eax,eax						<<return here
	cmp ecx, [g_cameraStructAddress]
	je correctcall
originalcode:
	movups [ecx+10h],xmm2
	movmskps eax,xmm0
	jmp exit
correctcall:
	cmp byte ptr [g_cameraEnabled], 1
	je quaternionskip
	movups [ecx+10h],xmm2
quaternionskip:
	movmskps eax,xmm0
exit:
	jmp dword ptr [_cameraWrite1InterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraWrite1Interceptor ENDP

fovReadInterceptor PROC
;UnityPlayer.dll"+4E5D01: F3 0F 11 45 FC           -  movss [ebp-04],xmm0
;UnityPlayer.dll"+4E5D06: D9 45 FC                 -  fld dword ptr [ebp-04]
;UnityPlayer.dll"+4E5D09: F3 0F 11 86 3C 01 00 00  -  movss [esi+0000013C],xmm0
;UnityPlayer.dll"+4E5D11: 5E                       -  pop esi
;UnityPlayer.dll"+4E5D12: 8B E5                    -  mov esp,ebp
;UnityPlayer.dll"+4E5D14: 5D                       -  pop ebp
;UnityPlayer.dll"+4E5D15: C3                       -  ret 
;UnityPlayer.dll"+4E5D16: F3 0F 10 86 3C 01 00 00  -  movss xmm0,[esi+0000013C]   <<intercept here
;UnityPlayer.dll"+4E5D1E: F3 0F 11 45 FC           -  movss [ebp-04],xmm0		  <<return here
;UnityPlayer.dll"+4E5D23: D9 45 FC                 -  fld dword ptr [ebp-04]
;UnityPlayer.dll"+4E5D26: 5E                       -  pop esi
;UnityPlayer.dll"+4E5D27: 8B E5                    -  mov esp,ebp
;UnityPlayer.dll"+4E5D29: 5D                       -  pop ebp
	mov [g_fovConstructAddress],esi
	movss xmm0, dword ptr [esi+0000013Ch] 
	jmp dword ptr [_fovReadInterceptionContinue]
fovReadInterceptor ENDP

borderInterceptor PROC
;UnityPlayer.dll+68DFF6 - 56                    - push esi
;UnityPlayer.dll+68DFF7 - 8B 75 08              - mov esi,[ebp+08]
;UnityPlayer.dll+68DFFA - 8B CE                 - mov ecx,esi
;UnityPlayer.dll+68DFFC - F3 0F10 86 A0 00 00 00   - movss xmm0,[esi+000000A0]			<<<intercept here
;UnityPlayer.dll+68E004 - FF 05 14223710        - inc [UnityPlayer.dll+10C2214] { (0) }  <<<return here
;UnityPlayer.dll+68E00A - 57                    - push edi
;UnityPlayer.dll+68E00B - 8D BE 9C000000        - lea edi,[esi+0000009C]
;UnityPlayer.dll+68E011 - F3 0F11 45 FC         - movss [ebp-04],xmm0
	cmp byte ptr [g_cameraEnabled], 1
	je borderremove
originalcode:
	movss xmm0, dword ptr [esi+000000A0h]
	jmp exit
borderremove:
	movss xmm0, [bordervalue]
exit:
	jmp dword ptr [_borderInterceptionContinue]
borderInterceptor ENDP

END