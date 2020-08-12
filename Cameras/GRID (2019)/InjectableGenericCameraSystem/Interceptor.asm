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
PUBLIC fovReadInterceptor
PUBLIC lodSettingInterceptor
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
EXTERN _fovReadInterceptionContinue: qword
EXTERN _lodSettingInterceptionContinue: qword

.data

.code

cameraStructInterceptor PROC
;for IGCS Camera: 41 0F 11 86 ?? ?? ?? ?? 41 0F 11 8E ?? ?? ?? ?? E8 ?? ?? ?? ?? 0F B6 84 24
;Grid_dx12.exe+A2C7C5 - 48 8D 4C 24 40        - lea rcx,[rsp+40]
;Grid_dx12.exe+A2C7CA - E8 D1E5FDFF           - call Grid_dx12.exe+A0ADA0
;Grid_dx12.exe+A2C7CF - 0F28 84 24 20010000   - movaps xmm0,[rsp+00000120]
;Grid_dx12.exe+A2C7D7 - 49 8D 8E 60010000     - lea rcx,[r14+00000160]
;Grid_dx12.exe+A2C7DE - 0F28 8C 24 30010000   - movaps xmm1,[rsp+00000130]
;Grid_dx12.exe+A2C7E6 - 48 8D 94 24 40010000  - lea rdx,[rsp+00000140]
;Grid_dx12.exe+A2C7EE - 41 0F11 86 40010000   - movups [r14+00000140],xmm0			<<coords/inject here
;Grid_dx12.exe+A2C7F6 - 41 0F11 8E 50010000   - movups [r14+00000150],xmm1			<<quaternion
;Grid_dx12.exe+A2C7FE - E8 4DCCE2FF           - call Grid_dx12.exe+859450			<<<return here
;Grid_dx12.exe+A2C803 - 0FB6 84 24 B0010000   - movzx eax,byte ptr [rsp+000001B0]
;Grid_dx12.exe+A2C80B - 41 88 86 D0010000     - mov [r14+000001D0],al
;Grid_dx12.exe+A2C812 - 0FB6 84 24 B1010000   - movzx eax,byte ptr [rsp+000001B1]
;Grid_dx12.exe+A2C81A - 41 88 86 D1010000     - mov [r14+000001D1],al
;Grid_dx12.exe+A2C821 - 0FB6 84 24 B2010000   - movzx eax,byte ptr [rsp+000001B2]
;Grid_dx12.exe+A2C829 - 41 88 86 D2010000     - mov [r14+000001D2],al
	mov [g_cameraStructAddress], r14
	cmp byte ptr [g_cameraEnabled],1
	je exit
	movups [r14+00000140h],xmm0	
	movups [r14+00000150h],xmm1
exit:
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

fovReadInterceptor PROC
;Grid_dx12.exe+859472 - 89 41 28              - mov [rcx+28],eax
;Grid_dx12.exe+859475 - 8B 42 2C              - mov eax,[rdx+2C]
;Grid_dx12.exe+859478 - 89 41 2C              - mov [rcx+2C],eax
;Grid_dx12.exe+85947B - 8B 42 30              - mov eax,[rdx+30]
;Grid_dx12.exe+85947E - 89 41 30              - mov [rcx+30],eax			<<<<inject here
;Grid_dx12.exe+859481 - 8B 42 34              - mov eax,[rdx+34]
;Grid_dx12.exe+859484 - 89 41 34              - mov [rcx+34],eax			<<hfov write
;Grid_dx12.exe+859487 - 8B 42 38              - mov eax,[rdx+38]
;Grid_dx12.exe+85948A - 89 41 38              - mov [rcx+38],eax			<<vfov write
;Grid_dx12.exe+85948D - 8B 42 3C              - mov eax,[rdx+3C]			<<< return here
;Grid_dx12.exe+859490 - 89 41 3C              - mov [rcx+3C],eax
;Grid_dx12.exe+859493 - 8B 42 40              - mov eax,[rdx+40]
;Grid_dx12.exe+859496 - 89 41 40              - mov [rcx+40],eax
;Grid_dx12.exe+859499 - 8B 42 44              - mov eax,[rdx+44]
;Grid_dx12.exe+85949C - 89 41 44              - mov [rcx+44],eax
;Grid_dx12.exe+85949F - 8B 42 48              - mov eax,[rdx+48]
;Grid_dx12.exe+8594A2 - 89 41 48              - mov [rcx+48],eax
	cmp byte ptr [g_cameraEnabled],0
	je exit
	push rbx
	lea rbx,[rcx-160h]
	cmp rbx, [g_cameraStructAddress]
	pop rbx
	jne exit
	mov dword ptr [rcx+30h],eax
	mov eax, dword ptr [rdx+34h]
	;mov dword ptr [rcx+34h],eax
	mov eax,dword ptr [rdx+38h]
	;mov dword ptr [rcx+38h],eax
	jmp exitreturn
exit:
	mov dword ptr [rcx+30h],eax
	mov eax, dword ptr [rdx+34h]
	mov dword ptr [rcx+34h],eax
	mov eax,dword ptr [rdx+38h]
	mov dword ptr [rcx+38h],eax
exitreturn:
	jmp qword ptr [_fovReadInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
fovReadInterceptor ENDP

lodSettingInterceptor PROC
;Grid_dx12.exe+A70D03 - 42 80 7C 00 18 00     - cmp byte ptr [rax+r8+18],00 { 0 }
;Grid_dx12.exe+A70D09 - 0F84 E4000000         - je Grid_dx12.exe+A70DF3
;Grid_dx12.exe+A70D0F - F3 41 0F10 86 48090000  - movss xmm0,[r14+00000948]
;Grid_dx12.exe+A70D18 - B0 01                 - mov al,01 { 1 }
;Grid_dx12.exe+A70D1A - F3 41 0F11 86 4C090000  - movss [r14+0000094C],xmm0
;Grid_dx12.exe+A70D23 - 41 88 86 A00A0000     - mov [r14+00000AA0],al			<< inject here
;Grid_dx12.exe+A70D2A - 41 89 96 9C0A0000     - mov [r14+00000A9C],edx			<<< change to mov 0x0
;Grid_dx12.exe+A70D31 - 84 C0                 - test al,al						<<< return here
;Grid_dx12.exe+A70D33 - 0F84 B5010000         - je Grid_dx12.exe+A70EEE
;Grid_dx12.exe+A70D39 - 49 8B 46 40           - mov rax,[r14+40]
;Grid_dx12.exe+A70D3D - 4C 8B A0 88000000     - mov r12,[rax+00000088]
;Grid_dx12.exe+A70D44 - 4D 85 E4              - test r12,r12
;Grid_dx12.exe+A70D47 - 0F84 99010000         - je Grid_dx12.exe+A70EE6
;Grid_dx12.exe+A70D4D - 48 8B 81 58010000     - mov rax,[rcx+00000158]
;Grid_dx12.exe+A70D54 - 45 33 FF              - xor r15d,r15d
;Grid_dx12.exe+A70D57 - 48 2B 81 50010000     - sub rax,[rcx+00000150]
	cmp byte ptr [g_cameraEnabled],1
	je writelod
	mov byte ptr [r14+00000AA0h],al
	mov dword ptr [r14+00000A9Ch],edx
	jmp exit
writelod:
	mov byte ptr [r14+00000AA0h],al
	mov dword ptr [r14+00000A9Ch],00000000
exit:
	jmp qword ptr [_lodSettingInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
lodSettingInterceptor ENDP

END