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
PUBLIC DOFinterceptor
PUBLIC ARread
;---------------------------------------------------------------

;---------------------------------------------------------------
; Externs which are used and set by the system. Read / write these
; values in asm to communicate with the system
EXTERN g_cameraEnabled: byte
EXTERN g_cameraStructAddress: dword
EXTERN g_dofstructaddress: dword
EXTERN g_ARvalueaddress: dword
;---------------------------------------------------------------

;---------------------------------------------------------------
; Own externs, defined in InterceptorHelper.cpp
EXTERN _cameraStructInterceptionContinue: dword
EXTERN _dofstructInterceptionContinue: dword
EXTERN _ARInterceptionContinue: dword



.data

.code

cameraStructInterceptor PROC
;ffxiiiimg.exe+A7C691 - 8B EC                 - mov ebp,esp
;ffxiiiimg.exe+A7C693 - 83 EC 14              - sub esp,14 { 20 }
;ffxiiiimg.exe+A7C696 - 89 4D EC              - mov [ebp-14],ecx
;ffxiiiimg.exe+A7C699 - 8B 45 EC              - mov eax,[ebp-14]
;ffxiiiimg.exe+A7C69C - 83 C0 08              - add eax,08 { 8 }
;ffxiiiimg.exe+A7C69F - 89 45 F0              - mov [ebp-10],eax
;ffxiiiimg.exe+A7C6A2 - 8B 4D 08              - mov ecx,[ebp+08]
;ffxiiiimg.exe+A7C6A5 - 8B 55 F0              - mov edx,[ebp-10]
;ffxiiiimg.exe+A7C6A8 - 8B 01                 - mov eax,[ecx]			<<<inject here
;ffxiiiimg.exe+A7C6AA - 89 02                 - mov [edx],eax			<<<<row 1 write
;ffxiiiimg.exe+A7C6AC - 8B 41 04              - mov eax,[ecx+04]
;ffxiiiimg.exe+A7C6AF - 89 42 04              - mov [edx+04],eax		<<<<row 1 second value write
;ffxiiiimg.exe+A7C6B2 - 8B 41 08              - mov eax,[ecx+08]
;ffxiiiimg.exe+A7C6B5 - 89 42 08              - mov [edx+08],eax		<<<row 1 second value write
;ffxiiiimg.exe+A7C6B8 - 8B 49 0C              - mov ecx,[ecx+0C]
;ffxiiiimg.exe+A7C6BB - 89 4A 0C              - mov [edx+0C],ecx
;ffxiiiimg.exe+A7C6BE - 8B 55 F0              - mov edx,[ebp-10]
;ffxiiiimg.exe+A7C6C1 - 83 C2 10              - add edx,10 { 16 }
;ffxiiiimg.exe+A7C6C4 - 89 55 FC              - mov [ebp-04],edx
;ffxiiiimg.exe+A7C6C7 - 8B 45 08              - mov eax,[ebp+08]
;ffxiiiimg.exe+A7C6CA - 83 C0 10              - add eax,10 { 16 }
;ffxiiiimg.exe+A7C6CD - 8B 4D FC              - mov ecx,[ebp-04]
;ffxiiiimg.exe+A7C6D0 - 8B 10                 - mov edx,[eax]
;ffxiiiimg.exe+A7C6D2 - 89 11                 - mov [ecx],edx			<<<<row 2 first write
;ffxiiiimg.exe+A7C6D4 - 8B 50 04              - mov edx,[eax+04]
;ffxiiiimg.exe+A7C6D7 - 89 51 04              - mov [ecx+04],edx		<<<<row 2 second write
;ffxiiiimg.exe+A7C6DA - 8B 50 08              - mov edx,[eax+08]
;ffxiiiimg.exe+A7C6DD - 89 51 08              - mov [ecx+08],edx		<<<row 2 third write
;ffxiiiimg.exe+A7C6E0 - 8B 40 0C              - mov eax,[eax+0C]
;ffxiiiimg.exe+A7C6E3 - 89 41 0C              - mov [ecx+0C],eax
;ffxiiiimg.exe+A7C6E6 - 8B 4D F0              - mov ecx,[ebp-10]
;ffxiiiimg.exe+A7C6E9 - 83 C1 20              - add ecx,20 { 32 }
;ffxiiiimg.exe+A7C6EC - 89 4D F8              - mov [ebp-08],ecx
;ffxiiiimg.exe+A7C6EF - 8B 55 08              - mov edx,[ebp+08]
;ffxiiiimg.exe+A7C6F2 - 83 C2 20              - add edx,20 { 32 }
;ffxiiiimg.exe+A7C6F5 - 8B 45 F8              - mov eax,[ebp-08]
;ffxiiiimg.exe+A7C6F8 - 8B 0A                 - mov ecx,[edx]
;ffxiiiimg.exe+A7C6FA - 89 08                 - mov [eax],ecx			<<<<row 3 first write
;ffxiiiimg.exe+A7C6FC - 8B 4A 04              - mov ecx,[edx+04]
;ffxiiiimg.exe+A7C6FF - 89 48 04              - mov [eax+04],ecx		<<<row 3 second write
;ffxiiiimg.exe+A7C702 - 8B 4A 08              - mov ecx,[edx+08]
;ffxiiiimg.exe+A7C705 - 89 48 08              - mov [eax+08],ecx		<<<row 3 third write
;ffxiiiimg.exe+A7C708 - 8B 52 0C              - mov edx,[edx+0C]
;ffxiiiimg.exe+A7C70B - 89 50 0C              - mov [eax+0C],edx
;ffxiiiimg.exe+A7C70E - 8B 45 F0              - mov eax,[ebp-10]
;ffxiiiimg.exe+A7C711 - 83 C0 30              - add eax,30 { 48 }
;ffxiiiimg.exe+A7C714 - 89 45 F4              - mov [ebp-0C],eax
;ffxiiiimg.exe+A7C717 - 8B 4D 08              - mov ecx,[ebp+08]
;ffxiiiimg.exe+A7C71A - 83 C1 30              - add ecx,30 { 48 }
;ffxiiiimg.exe+A7C71D - 8B 55 F4              - mov edx,[ebp-0C]
;ffxiiiimg.exe+A7C720 - 8B 01                 - mov eax,[ecx]
;ffxiiiimg.exe+A7C722 - 89 02                 - mov [edx],eax			<<<<coords x write
;ffxiiiimg.exe+A7C724 - 8B 41 04              - mov eax,[ecx+04]
;ffxiiiimg.exe+A7C727 - 89 42 04              - mov [edx+04],eax		<<<<cords y write
;ffxiiiimg.exe+A7C72A - 8B 41 08              - mov eax,[ecx+08]
;ffxiiiimg.exe+A7C72D - 89 42 08              - mov [edx+08],eax		<<<<coords z write
;ffxiiiimg.exe+A7C730 - 8B 49 0C              - mov ecx,[ecx+0C]
;ffxiiiimg.exe+A7C733 - 89 4A 0C              - mov [edx+0C],ecx
	mov [g_cameraStructAddress], edx
	cmp byte ptr [g_cameraEnabled], 1
	je writeSkip

	mov eax,[ecx]
	mov [edx],eax
	mov eax,[ecx+04h]
	mov [edx+04h],eax
	mov eax,[ecx+08h]
	mov [edx+08h],eax
	mov ecx,[ecx+0Ch]
	mov [edx+0Ch],ecx
	mov edx,[ebp-10h]
	add edx,10h
	mov [ebp-04h],edx
	mov eax,[ebp+08h]
	add eax,10h
	mov ecx,[ebp-04h]
	mov edx,[eax]
	mov [ecx],edx
	mov edx,[eax+04h]
	mov [ecx+04h],edx
	mov edx,[eax+08h]
	mov [ecx+08h],edx
	mov eax,[eax+0Ch]
	mov [ecx+0Ch],eax
	mov ecx,[ebp-10h]
	add ecx,20h
	mov [ebp-08h],ecx
	mov edx,[ebp+08h]
	add edx,20h
	mov eax,[ebp-08h]
	mov ecx,[edx]
	mov [eax],ecx
	mov ecx,[edx+04h]
	mov [eax+04h],ecx
	mov ecx,[edx+08h]
	mov [eax+08h],ecx
	mov edx,[edx+0Ch]
	mov [eax+0Ch],edx
	mov eax,[ebp-10h]
	add eax,30h
	mov [ebp-0Ch],eax
	mov ecx,[ebp+08h]
	add ecx,30h
	mov edx,[ebp-0Ch]
	mov eax,[ecx]
	mov [edx],eax
	mov eax,[ecx+04h]
	mov [edx+04h],eax
	mov eax,[ecx+08h]
	mov [edx+08h],eax
	mov ecx,[ecx+0Ch]
	mov [edx+0Ch],ecx
	jmp exit

writeSkip:
	mov eax,[ecx]
	;mov [edx],eax
	mov eax,[ecx+04h]
	;mov [edx+04h],eax
	mov eax,[ecx+08h]
	;mov [edx+08h],eax
	mov ecx,[ecx+0Ch]
	mov [edx+0Ch],ecx
	mov edx,[ebp-10h]
	add edx,10h
	mov [ebp-04h],edx
	mov eax,[ebp+08h]
	add eax,10h
	mov ecx,[ebp-04h]

	mov edx,[eax]
	;mov [ecx],edx
	mov edx,[eax+04h]
	;mov [ecx+04h],edx
	mov edx,[eax+08h]
	;mov [ecx+08h],edx
	mov eax,[eax+0Ch]
	mov [ecx+0Ch],eax
	mov ecx,[ebp-10h]
	add ecx,20h
	mov [ebp-08h],ecx
	mov edx,[ebp+08h]
	add edx,20h
	mov eax,[ebp-08h]

	mov ecx,[edx]
	;mov [eax],ecx
	mov ecx,[edx+04h]
	;mov [eax+04h],ecx
	mov ecx,[edx+08h]
	;mov [eax+08h],ecx
	mov edx,[edx+0Ch]
	mov [eax+0Ch],edx
	mov eax,[ebp-10h]
	add eax,30h
	mov [ebp-0Ch],eax
	mov ecx,[ebp+08h]
	add ecx,30h
	mov edx,[ebp-0Ch]

	mov eax,[ecx]
	;mov [edx],eax
	mov eax,[ecx+04h]
	;mov [edx+04h],eax
	mov eax,[ecx+08h]
	;mov [edx+08h],eax
	mov ecx,[ecx+0Ch]
	mov [edx+0Ch],ecx
exit:
	jmp dword ptr [_cameraStructInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraStructInterceptor ENDP
	
DOFinterceptor PROC
;ffxiiiimg.exe+B1E94F - F3 0F10 40 28         - movss xmm0,[eax+28]
;ffxiiiimg.exe+B1E954 - F3 0F5C 41 2C         - subss xmm0,[ecx+2C]
;ffxiiiimg.exe+B1E959 - 8B 55 C0              - mov edx,[ebp-40]
;ffxiiiimg.exe+B1E95C - F3 0F11 42 40         - movss [edx+40],xmm0    <<<inject here - edx contains the dof struct so we need to get this
;ffxiiiimg.exe+B1E961 - 8B 45 C0              - mov eax,[ebp-40]
;ffxiiiimg.exe+B1E964 - 8B 4D C0              - mov ecx,[ebp-40]
;ffxiiiimg.exe+B1E967 - F3 0F10 40 28         - movss xmm0,[eax+28]
;ffxiiiimg.exe+B1E96C - F3 0F58 41 30         - addss xmm0,[ecx+30]	   <<<<return here
	mov [g_dofstructaddress],edx
	movss dword ptr [edx+40h],xmm0
	mov eax,[ebp-40h]
	mov ecx,[ebp-40h]
	movss xmm0, dword ptr[eax+28h]
exit:
	jmp dword ptr [_dofstructInterceptionContinue]
DOFinterceptor ENDP


ARread PROC
;ffxiiiimg.exe+228A5A - F3 0F11 84 24 88010000  - movss [esp+00000188],xmm0
;ffxiiiimg.exe+228A63 - 6A 01                 - push 01 { 1 }
;ffxiiiimg.exe+228A65 - 8B 8C 24 74030000     - mov ecx,[esp+00000374]
;ffxiiiimg.exe+228A6C - 51                    - push ecx
;ffxiiiimg.exe+228A6D - D9 81 FC030000        - fld dword ptr [ecx+000003FC]			<<<intercept here - this is the AR Read
;ffxiiiimg.exe+228A73 - D9 1C 24              - fstp dword ptr [esp]
;ffxiiiimg.exe+228A76 - F3 0F10 84 24 8C010000  - movss xmm0,[esp+0000018C]				<<< return after this
	mov [g_ARvalueaddress],ecx
	fld dword ptr [ecx+000003FCh]
	fstp dword ptr [esp]
	movss xmm0,dword ptr [esp+0000018Ch]
exit:
	jmp dword ptr [_ARInterceptionContinue]
ARread ENDP

END