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
PUBLIC cameraPositionInterceptor
PUBLIC cameraRotationInterceptor
PUBLIC vignetteInterceptor
PUBLIC timescaleInterceptor
PUBLIC playerStructInterceptor
PUBLIC playerLightCheckInterceptor

;---------------------------------------------------------------

;---------------------------------------------------------------
; Externs which are used and set by the system. Read / write these
; values in asm to communicate with the system
EXTERN g_cameraEnabled: byte
EXTERN g_cameraStructAddress: qword
EXTERN g_vignetteAddress: qword
EXTERN g_timescaleAddress: qword
EXTERN g_playerStructAddress: qword
EXTERN g_disableFlashlight: byte

;---------------------------------------------------------------

;---------------------------------------------------------------
; Own externs, defined in InterceptorHelper.cpp
EXTERN _cameraStructInterceptionContinue: qword
EXTERN _cameraPositionInterceptionContinue: qword
EXTERN _cameraRotationInterceptionContinue: qword
EXTERN _vignetteInterceptionContinue: qword
EXTERN _timescaleInterceptionContinue: qword
EXTERN _playerStructInterceptionContinue: qword
EXTERN _playerLightCheckInterceptionContinue: qword
EXTERN _playerLightCheckJumpTarget: qword

.data

.code


cameraStructInterceptor PROC
;for IGCS Camera: F3 0F 11 4A ?? E9 ?? ?? ?? ?? | 48 8B 48 ?? 48 8B 81 ?? ?? ?? ?? 48 85 C0 74 03 8B 40 ?? 85 C0 0F 84 ?? ?? ?? ??
;re7.exe+242E4DB - F3 0F10 05 0D5CA906   - movss xmm0,[re7.AmdPowerXpressRequestHighPerformance+1B04] { (0.00) }
;re7.exe+242E4E3 - F3 0F11 42 30         - movss [rdx+30],xmm0
;re7.exe+242E4E8 - F3 0F10 0D 045CA906   - movss xmm1,[re7.AmdPowerXpressRequestHighPerformance+1B08] { (0.00) }
;re7.exe+242E4F0 - F3 0F11 4A 34         - movss [rdx+34],xmm1
;re7.exe+242E4F5 - F3 0F10 05 FB5BA906   - movss xmm0,[re7.AmdPowerXpressRequestHighPerformance+1B0C] { (0.00) }
;re7.exe+242E4FD - F3 0F11 42 38         - movss [rdx+38],xmm0
;re7.exe+242E502 - F3 0F10 0D F25BA906   - movss xmm1,[re7.AmdPowerXpressRequestHighPerformance+1B10] { (1.00) }
;re7.exe+242E50A - F3 0F11 4A 3C         - movss [rdx+3C],xmm1
;re7.exe+242E50F - E9 C6000000           - jmp re7.exe+242E5DA
;re7.exe+242E514 - 48 8B 48 18           - mov rcx,[rax+18]			<< inject here/ rcx has address
;re7.exe+242E518 - 48 8B 81 D0000000     - mov rax,[rcx+000000D0]
;re7.exe+242E51F - 48 85 C0              - test rax,rax				
;re7.exe+242E522 - 74 03                 - je re7.exe+242E527		<<return here
;re7.exe+242E524 - 8B 40 1C              - mov eax,[rax+1C]
;re7.exe+242E527 - 85 C0                 - test eax,eax
;re7.exe+242E529 - 0F84 11FFFFFF         - je re7.exe+242E440
;re7.exe+242E52F - 48 8B 89 D0000000     - mov rcx,[rcx+000000D0]
;re7.exe+242E536 - 48 8D 55 F7           - lea rdx,[rbp-09]
;re7.exe+242E53A - 0F29 B4 24 A0000000   - movaps [rsp+000000A0],xmm6
;re7.exe+242E542 - 48 8B 49 20           - mov rcx,[rcx+20]
;re7.exe+242E546 - E8 35E9FFFF           - call re7.exe+242CE80
;re7.exe+242E54B - F3 0F10 0D 5DFE7B03   - movss xmm1,[re7.exe+5BEE3B0] { (0.00) }
	mov rcx,[rax+18h]
	mov [g_cameraStructAddress], rcx
	mov rax,[rcx+000000D0h]
	test rax,rax
exit:
	jmp qword ptr [_cameraStructInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraStructInterceptor ENDP


cameraPositionInterceptor PROC
;89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 80 B9 ?? ?? ?? ?? 00 C6 81 ?? ?? ?? ?? 01 75 32
;re7.exe+24159EF - CC                    - int 3 
;re7.exe+24159F0 - 8B 02                 - mov eax,[rdx]
;re7.exe+24159F2 - 4C 8B C9              - mov r9,rcx
;re7.exe+24159F5 - 89 41 30              - mov [rcx+30],eax							<< inject here, cmp to g_cameraStructAddress and skip if equal
;re7.exe+24159F8 - 8B 42 04              - mov eax,[rdx+04]
;re7.exe+24159FB - 89 41 34              - mov [rcx+34],eax
;re7.exe+24159FE - 8B 42 08              - mov eax,[rdx+08]
;re7.exe+2415A01 - 89 41 38              - mov [rcx+38],eax
;re7.exe+2415A04 - 80 B9 01010000 00     - cmp byte ptr [rcx+00000101],00 { 0 }		<<return here
;re7.exe+2415A0B - C6 81 00010000 01     - mov byte ptr [rcx+00000100],01 { 1 }
;re7.exe+2415A12 - 75 32                 - jne re7.exe+2415A46
;re7.exe+2415A14 - 48 8B 49 60           - mov rcx,[rcx+60];
	cmp byte ptr [g_cameraEnabled], 1
	jne skipCheck
	cmp rcx, [g_cameraStructAddress]
	je skip
skipCheck:
	mov dword ptr [rcx+30h],eax
	mov eax,[rdx+04h]
	mov dword ptr [rcx+34h],eax
	mov eax,[rdx+08h]
	mov dword ptr [rcx+38h],eax
	jmp exit
skip:
	;mov [rcx+30h],eax
	mov eax,[rdx+04h]
	;mov [rcx+34h],eax
	mov eax,[rdx+08h]
	;mov [rcx+38h],eax
exit:
	jmp qword ptr [_cameraPositionInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraPositionInterceptor ENDP

cameraRotationInterceptor PROC
;8B 02 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 0C 89 41 4C 80 B9 ?? ?? ?? ?? 00 C6 81 ?? ?? ?? ?? 01 75 33 48 8B CF 48 85 FF 74 2B
;re7.exe+2416269 - 75 44                 - jne re7.exe+24162AF
;re7.exe+241626B - C6 83 01010000 01     - mov byte ptr [rbx+00000101],01 { 1 }
;re7.exe+2416272 - 80 B9 01010000 00     - cmp byte ptr [rcx+00000101],00 { 0 }
;re7.exe+2416279 - EB 53                 - jmp re7.exe+24162CE
;re7.exe+241627B - 8B 02                 - mov eax,[rdx]						<<inject here, cmp to g_cameraStructAddress and skip if equal
;re7.exe+241627D - 89 41 40              - mov [rcx+40],eax
;re7.exe+2416280 - 8B 42 04              - mov eax,[rdx+04]
;re7.exe+2416283 - 89 41 44              - mov [rcx+44],eax
;re7.exe+2416286 - 8B 42 08              - mov eax,[rdx+08]
;re7.exe+2416289 - 89 41 48              - mov [rcx+48],eax
;re7.exe+241628C - 8B 42 0C              - mov eax,[rdx+0C]
;re7.exe+241628F - 89 41 4C              - mov [rcx+4C],eax
;re7.exe+2416292 - 80 B9 01010000 00     - cmp byte ptr [rcx+00000101],00 { 0 }	<<return here
;re7.exe+2416299 - C6 81 00010000 01     - mov byte ptr [rcx+00000100],01 { 1 }
;re7.exe+24162A0 - 75 33                 - jne re7.exe+24162D5
;re7.exe+24162A2 - 48 8B CF              - mov rcx,rdi
;re7.exe+24162A5 - 48 85 FF              - test rdi,rdi
;re7.exe+24162A8 - 74 2B                 - je re7.exe+24162D5
	cmp byte ptr [g_cameraEnabled], 1
	jne skipCheck
	cmp rcx, [g_cameraStructAddress]
	je skip
skipCheck:
	mov eax,[rdx]
	mov dword ptr [rcx+40h],eax
	mov eax,[rdx+04h]
	mov dword ptr [rcx+44h],eax
	mov eax,[rdx+08h]
	mov dword ptr [rcx+48h],eax
	mov eax,[rdx+0Ch]
	mov dword ptr [rcx+4Ch],eax
	jmp exit
skip:
	mov eax,[rdx]
	;mov dword ptr [rcx+40h],eax
	mov eax,[rdx+04h]
	;mov dword ptr [rcx+44h],eax
	mov eax,[rdx+08h]
	;mov dword ptr [rcx+48h],eax
	mov eax,[rdx+0Ch]
	;mov dword ptr [rcx+4Ch],eax
exit:
	jmp qword ptr [_cameraRotationInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraRotationInterceptor ENDP

vignetteInterceptor PROC
;re7.exe+2E85A34 - F3 0F10 80 84010000   - movss xmm0,[rax+00000184]
;re7.exe+2E85A3C - C3                    - ret 
;re7.exe+2E85A3D - CC                    - int 3 
;re7.exe+2E85A3E - CC                    - int 3 
;re7.exe+2E85A3F - CC                    - int 3 
;re7.exe+2E85A40 - 48 8B 41 30           - mov rax,[rcx+30]			<<inject here, 
;re7.exe+2E85A44 - 8B 80 0C 02 00 00     - mov eax,[rax+0000020C]
;re7.exe+2E85A4A - C3                    - ret 
;re7.exe+2E85A4B - CC                    - int 3 
;re7.exe+2E85A4C - CC                    - int 3 
;re7.exe+2E85A4D - CC                    - int 3	
;re7.exe+2E85A4E - CC                    - int 3 
	mov rax,[rcx+30h]
	mov [g_vignetteAddress], rax
	mov eax,dword ptr [rax+0000020Ch]
	ret 
vignetteInterceptor ENDP

timescaleInterceptor PROC
;re7.exe+240D4D4 - F3 0F11 83 4C030000   - movss [rbx+0000034C],xmm0
;re7.exe+240D4DC - F3 0F10 83 4C030000   - movss xmm0,[rbx+0000034C]
;re7.exe+240D4E4 - F3 0F59 83 48030000   - mulss xmm0,[rbx+00000348]	<<inject here, read rbx
;re7.exe+240D4EC - F3 0F11 83 4C030000   - movss [rbx+0000034C],xmm0	
;re7.exe+240D4F4 - 0F5A C0               - cvtps2pd xmm0,xmm0			<<return here
;re7.exe+240D4F7 - F2 41 0F5E C0         - divsd xmm0,xmm8
;re7.exe+240D4FC - 66 0F5A C0            - cvtpd2ps xmm0,xmm0
	mulss xmm0, dword ptr [rbx+00000348h]
	mov [g_timescaleAddress], rbx
	movss dword ptr [rbx+0000034Ch],xmm0
	jmp qword ptr [_timescaleInterceptionContinue]
timescaleInterceptor ENDP

playerStructInterceptor PROC
;re7.exe+23A17AF - F3 44 0F11 9B A0020000  - movss [rbx+000002A0],xmm11
;re7.exe+23A17B8 - F3 0F11 BB A4020000   - movss [rbx+000002A4],xmm7
;re7.exe+23A17C0 - F3 44 0F11 8B A8020000  - movss [rbx+000002A8],xmm9
;re7.exe+23A17C9 - F3 44 0F11 93 AC020000  - movss [rbx+000002AC],xmm10
;re7.exe+23A17D2 - 48 83 78 18 00        - cmp qword ptr [rax+18],00 { 0 }
;re7.exe+23A17D7 - 75 60                 - jne re7.exe+23A1839
;re7.exe+23A17D9 - 48 8B 43 78           - mov rax,[rbx+78]
;re7.exe+23A17DD - 48 85 C0              - test rax,rax
;re7.exe+23A17E0 - 74 48                 - je re7.exe+23A182A
;re7.exe+23A17E2 - F3 0F10 40 30         - movss xmm0,[rax+30]			<<intercept here, rax has player struct address
;re7.exe+23A17E7 - 48 8D 54 24 20        - lea rdx,[rsp+20]
;re7.exe+23A17EC - F3 0F10 48 34         - movss xmm1,[rax+34]
;re7.exe+23A17F1 - 48 8B C8              - mov rcx,rax					<<return here
;re7.exe+23A17F4 - F3 0F10 50 38         - movss xmm2,[rax+38]
;re7.exe+23A17F9 - F3 0F11 83 A0030000   - movss [rbx+000003A0],xmm0
;re7.exe+23A1801 - F3 0F11 8B A4030000   - movss [rbx+000003A4],xmm1
;re7.exe+23A1809 - F3 0F11 93 A8030000   - movss [rbx+000003A8],xmm2
	mov [g_playerStructAddress], rax
	movss xmm0,dword ptr[rax+30h]
	lea rdx,[rsp+20h]
	movss xmm1,dword ptr [rax+34h]
	jmp qword ptr [_playerStructInterceptionContinue]
playerStructInterceptor ENDP

playerLightCheckInterceptor PROC
; This codecave replaces the following 5 original instructions:
; 1. mov rax,[rdi+60]
; 2. test al,01
; 3. jne re7.exe+2416158
; 4. test rax,rax
; 5. je re7.exe+2416158
; this is quite a complex interception, due to the number of jumps involved.

    mov rax,[rdi+60h]						; Replicates instruction 1: mov rax,[rdi+60]
    test al, 1								; Replicates instruction 2: test al,01
    cmp byte ptr [g_disableFlashlight], 1	; Check if your custom logic should run
    jne original_flags_preserved

; --- Custom Logic Path (g_disableFlashlight is 1) ---
    ; This path overwrites the flags from the 'test al, 1' instruction
    push rax
    mov rax, [g_cameraStructAddress]
    cmp rdi, rax
    pop rax
    ; The flags are now set by this CMP instruction

original_flags_preserved:
; --- Shared Logic Path ---
    ; Replicates instruction 3: jne re7.exe+2416158
    ; This JNE uses the flags from either the original 'test' or your custom 'cmp'
    jne jump_target_label 
    ; --- Fallthrough Logic (if the first JNE was NOT taken) ---
    test rax,rax							; Replicates instruction 4: test rax,rax
    je jump_target_label					; Replicates instruction 5: je re7.exe+2416158
    jmp qword ptr [_playerLightCheckInterceptionContinue] ; If NEITHER of the conditional jumps were taken, we continue execution after the overwritten block.

jump_target_label:
    ; Both original conditional jumps went to the same address.
    ; This label sends us there.
    jmp qword ptr [_playerLightCheckJumpTarget]

playerLightCheckInterceptor ENDP

END