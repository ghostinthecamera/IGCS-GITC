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
PUBLIC cameraWrite1Interceptor
PUBLIC cameraWrite2Interceptor
;PUBLIC resolutionScaleReadInterceptor
;PUBLIC todWriteInterceptor
PUBLIC timestopReadInterceptor
;PUBLIC fogReadInterceptor

;---------------------------------------------------------------

;---------------------------------------------------------------
; Externs which are used and set by the system. Read / write these
; values in asm to communicate with the system
EXTERN g_cameraEnabled: byte
EXTERN g_cameraStructAddress: qword
EXTERN _effectivecamstruct: qword
;EXTERN g_resolutionScaleAddress: qword
;EXTERN g_todStructAddress: qword
EXTERN g_timestopStructAddress: qword
;EXTERN g_fogStructAddress: qword

;---------------------------------------------------------------

;---------------------------------------------------------------
; Own externs, defined in InterceptorHelper.cpp
EXTERN _cameraStructInterceptionContinue: qword
EXTERN _divssAbsoluteAdd: qword

EXTERN _cameraWrite1InterceptionContinue: qword
EXTERN _cameraWrite2InterceptionContinue: qword
;EXTERN _fovReadInterceptionContinue: qword
;EXTERN _resolutionScaleReadInterceptionContinue: qword
;EXTERN _todWriteInterceptionContinue: qword
EXTERN _timestopReadInterceptionContinue: qword
;EXTERN _fogReadInterceptionContinue: qword

.data



.code


cameraStructInterceptor PROC
; this gets the fov - but also can use RBX-offsets to get camera coords+quaternion. 
;DesertsOfKharak64.exe+ABFF7 - F3 0F10 83 84020000   - movss xmm0,[rbx+00000284]		<<inject here << fov+base
;DesertsOfKharak64.exe+ABFFF - 0F28 F7               - movaps xmm6,xmm7
;DesertsOfKharak64.exe+AC002 - F3 0F5E 05 9E31FC00   - divss xmm0,[DesertsOfKharak64.exe+106F1A8] { (180.00) }
;DesertsOfKharak64.exe+AC00A - F3 0F5C F0            - subss xmm6,xmm0					<<return here
	mov [g_cameraStructAddress], rbx
	movss xmm0, dword ptr [rbx+284h]
	mov [rbx+324h],byte ptr 1
	mov [rbx+325h],byte ptr 1
	mov [rbx+326h],byte ptr 1
	mov [rbx+327h],byte ptr 1
	movaps xmm6,xmm7
	push rax
	mov rax, [_divssAbsoluteAdd]
	divss xmm0, dword ptr [rax]
	pop rax
exit:
	jmp qword ptr [_cameraStructInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraStructInterceptor ENDP

cameraWrite1Interceptor PROC
;DesertsOfKharak64.Transform::SetPosition+70 - F3 0F10 43 48         - movss xmm0,[rbx+48]
;DesertsOfKharak64.Transform::SetPosition+75 - 0F2E 44 24 28         - ucomiss xmm0,[rsp+28]
;DesertsOfKharak64.Transform::SetPosition+7A - 7A 02                 - jp DesertsOfKharak64.Transform::SetPosition+7E
;DesertsOfKharak64.Transform::SetPosition+7C - 74 18                 - je DesertsOfKharak64.Transform::SetPosition+96
;DesertsOfKharak64.Transform::SetPosition+7E - 44 89 43 40           - mov [rbx+40],r8d													<<inject here
;DesertsOfKharak64.Transform::SetPosition+82 - 44 89 4B 44           - mov [rbx+44],r9d
;DesertsOfKharak64.Transform::SetPosition+86 - BA 01000000           - mov edx,00000001 { 1 }
;DesertsOfKharak64.Transform::SetPosition+8B - 48 8B CB              - mov rcx,rbx
;DesertsOfKharak64.Transform::SetPosition+8E - 89 43 48              - mov [rbx+48],eax					
;DesertsOfKharak64.Transform::SetPosition+91 - E8 4AE0FFFF           - call DesertsOfKharak64.Transform::SendTransformChanged			<<return here
;DesertsOfKharak64.Transform::SetPosition+96 - 48 83 C4 40           - add rsp,40 { 64 }
;DesertsOfKharak64.Transform::SetPosition+9A - 5B                    - pop rbx
;DesertsOfKharak64.Transform::SetPosition+9B - C3                    - ret 
  cmp byte ptr [g_cameraEnabled],1
  jne originalcode
  ;push r10
  ;push r8
  ;mov qword ptr r8, [g_cameraStructAddress]
  ;lea r10, [r8+1980h]
  ;cmp qword ptr r10, rbx
  cmp qword ptr rbx, [_effectivecamstruct]      ;same as above to ensure we have the right call
  ;pop r10
  ;pop r8
  je mycode									 ;jump to our code if the above variables are equal
originalcode:
  mov dword ptr [rbx+40h],r8d
  mov dword ptr [rbx+44h],r9d
  mov edx,00000001
  mov rcx,rbx
  mov dword ptr [rbx+48h],eax
  jmp exit
mycode:
  mov edx,00000001h
  mov rcx,rbx
exit:
  jmp qword ptr [_cameraWrite1InterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraWrite1Interceptor ENDP

cameraWrite2Interceptor PROC
;DesertsOfKharak64.Transform::SetRotationSafe+B2 - 0F2E 40 0C            - ucomiss xmm0,[rax+0C]
;DesertsOfKharak64.Transform::SetRotationSafe+B6 - 7A 02                 - jp DesertsOfKharak64.Transform::SetRotationSafe+BA
;DesertsOfKharak64.Transform::SetRotationSafe+B8 - 74 27                 - je DesertsOfKharak64.Transform::SetRotationSafe+E1
;DesertsOfKharak64.Transform::SetRotationSafe+BA - 8B 00                 - mov eax,[rax]
;DesertsOfKharak64.Transform::SetRotationSafe+BC - BA 02000000           - mov edx,00000002 { 2 }
;DesertsOfKharak64.Transform::SetRotationSafe+C1 - 48 8B CB              - mov rcx,rbx
;DesertsOfKharak64.Transform::SetRotationSafe+C4 - 89 43 30              - mov [rbx+30],eax				<<<inject here
;DesertsOfKharak64.Transform::SetRotationSafe+C7 - 41 8B 43 04           - mov eax,[r11+04]
;DesertsOfKharak64.Transform::SetRotationSafe+CB - 89 43 34              - mov [rbx+34],eax
;DesertsOfKharak64.Transform::SetRotationSafe+CE - 41 8B 43 08           - mov eax,[r11+08]
;DesertsOfKharak64.Transform::SetRotationSafe+D2 - 89 43 38              - mov [rbx+38],eax
;DesertsOfKharak64.Transform::SetRotationSafe+D5 - 41 8B 43 0C           - mov eax,[r11+0C]
;DesertsOfKharak64.Transform::SetRotationSafe+D9 - 89 43 3C              - mov [rbx+3C],eax				<<return here
;DesertsOfKharak64.Transform::SetRotationSafe+DC - E8 EFE0FFFF           - call DesertsOfKharak64.Transform::SendTransformChanged
;DesertsOfKharak64.Transform::SetRotationSafe+E1 - 48 83 C4 50           - add rsp,50 { 80 }
;DesertsOfKharak64.Transform::SetRotationSafe+E5 - 5B                    - pop rbx
;DesertsOfKharak64.Transform::SetRotationSafe+E6 - C3                    - ret 
  cmp byte ptr [g_cameraEnabled],1
  jne originalcode
  ;push r10
  ;push r8
  ;mov qword ptr r8, [g_cameraStructAddress]
  ;lea r10, [r8+1980h]
  ;cmp qword ptr r10, rbx
  cmp qword ptr rbx, [_effectivecamstruct] 
  ;cmp dword ptr [rbx+88Ch], 01010000h      ;same as above to ensure we have the right call
  ;pop r10
  ;pop r8
  je exit
originalcode:
  mov dword ptr [rbx+30h],eax
  mov eax,dword ptr [r11+04h]
  mov dword ptr [rbx+34h],eax
  mov eax,dword ptr [r11+08h]
  mov dword ptr [rbx+38h],eax
  mov eax,dword ptr [r11+0Ch]
  mov dword ptr [rbx+3Ch],eax
exit:
  jmp qword ptr [_cameraWrite2InterceptionContinue]	 ;jmp back into the original game code, which is the location after the original statements above.
cameraWrite2Interceptor ENDP

timestopReadInterceptor PROC
;DesertsOfKharak64.GlobalCallbacks::Get+1070 - 48 8D 8B A8000000     - lea rcx,[rbx+000000A8]
;DesertsOfKharak64.GlobalCallbacks::Get+1077 - 8B 90 CC000000        - mov edx,[rax+000000CC]	<<<< timevalue read/intercept here
;DesertsOfKharak64.GlobalCallbacks::Get+107D - 0F14 F6               - unpcklps xmm6,xmm6
;DesertsOfKharak64.GlobalCallbacks::Get+1080 - 4C 8D 44 24 20        - lea r8,[rsp+20]
;DesertsOfKharak64.GlobalCallbacks::Get+1085 - 0F5A C6               - vcvtps2pd xmm0,xmm6		<<<<<return here
;DesertsOfKharak64.GlobalCallbacks::Get+1088 - F2 0F11 44 24 20      - movsd [rsp+20],xmm0
;DesertsOfKharak64.GlobalCallbacks::Get+108E - 89 93 C0000000        - mov [rbx+000000C0],edx
;DesertsOfKharak64.GlobalCallbacks::Get+1094 - 8B 93 A0000000        - mov edx,[rbx+000000A0]
;DesertsOfKharak64.GlobalCallbacks::Get+109A - 48 8B 83 98000000     - mov rax,[rbx+00000098]
;DesertsOfKharak64.GlobalCallbacks::Get+10A1 - F2 0F10 83 90000000   - movsd xmm0,[rbx+00000090]
;DesertsOfKharak64.GlobalCallbacks::Get+10A9 - FF 83 88000000        - inc [rbx+00000088]
;DesertsOfKharak64.GlobalCallbacks::Get+10AF - 89 51 10              - mov [rcx+10],edx
	mov [g_timestopStructAddress],rax
	mov edx,dword ptr [rax+000000CCh]
	unpcklps xmm6,xmm6
	lea r8,[rsp+20h]
exit:
    jmp qword ptr [_timestopReadInterceptionContinue]	 ;jmp back into the original game code, which is the location after the original statements above.
timestopReadInterceptor ENDP

END