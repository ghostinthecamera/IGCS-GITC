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
PUBLIC debugcaminterceptor
PUBLIC dofStruct
PUBLIC timescaleinterceptor
PUBLIC fovintercept
PUBLIC playerpointerinterceptor
PUBLIC entitytimeinterceptor
;---------------------------------------------------------------

;---------------------------------------------------------------
; Externs which are used and set by the system. Read / write these
; values in asm to communicate with the system
EXTERN g_cameraEnabled: byte
EXTERN g_playersonly: byte
EXTERN g_cameraStructAddress: qword
EXTERN g_dofstructaddress: qword
EXTERN g_timescaleaddress: qword
EXTERN g_uwfovaddress: qword
;EXTERN _fovdelta: dword
EXTERN g_playerpointer: qword
;---------------------------------------------------------------

;---------------------------------------------------------------
; Own externs, defined in InterceptorHelper.cpp
EXTERN _cameraStructInterceptionContinue: dword
EXTERN _debugcamInterceptionContinue: qword
EXTERN _dofInterceptionContinue: qword
EXTERN _timestructinterceptionContinue: qword
EXTERN _fovinterceptContinue: qword
EXTERN _playerpointerContinue: qword
EXTERN _entitytimeContinue: qword

.data

.code

timevalue dd 0.0

cameraStructInterceptor PROC
;sekiro.exe+734702 - 8B 42 54              - mov eax,[rdx+54]
;sekiro.exe+734705 - 89 41 54              - mov [rcx+54],eax
;sekiro.exe+734708 - 8B 42 58              - mov eax,[rdx+58]
;sekiro.exe+73470B - 89 41 58              - mov [rcx+58],eax
;sekiro.exe+73470E - 8B 42 5C              - mov eax,[rdx+5C]
;sekiro.exe+734711 - 48 8D 54 24 20        - lea rdx,[rsp+20]
;sekiro.exe+734716 - 89 41 5C              - mov [rcx+5C],eax
;sekiro.exe+734719 - 49 8B C8              - mov rcx,r8
;sekiro.exe+73471C - E8 1FFBFFFF           - call sekiro.exe+734240
;sekiro.exe+734721 - 0F28 00               - movaps xmm0,[rax]				<<intercept here
;sekiro.exe+734724 - 66 0F7F 43 10         - movdqa [rbx+10],xmm0			<<row 1 write
;sekiro.exe+734729 - 0F28 48 10            - movaps xmm1,[rax+10]
;sekiro.exe+73472D - 66 0F7F 4B 20         - movdqa [rbx+20],xmm1			<<row 2 write
;sekiro.exe+734732 - 0F28 40 20            - movaps xmm0,[rax+20]
;sekiro.exe+734736 - 66 0F7F 43 30         - movdqa [rbx+30],xmm0			<<row 3 write
;sekiro.exe+73473B - 0F28 48 30            - movaps xmm1,[rax+30]
;sekiro.exe+73473F - 66 0F7F 4B 40         - movdqa [rbx+40],xmm1			<<row 4 write (coords)
;sekiro.exe+734744 - 48 83 C4 60           - add rsp,60 { 96 }
;sekiro.exe+734748 - 5B                    - pop rbx
	cmp r9,0
	jne skipcamstruct
	mov [g_cameraStructAddress], rbx
skipcamstruct:
	cmp byte ptr [g_cameraEnabled], 1
	je skipwrites
	movaps xmm0,[rax]
    movdqa [rbx+10h],xmm0
	movaps xmm1,[rax+10h]
	movdqa [rbx+20h],xmm1
	movaps xmm0,[rax+20h]
	movdqa [rbx+30h],xmm0
	movaps xmm1,[rax+30h]
	movdqa [rbx+40h],xmm1
	jmp exit
skipwrites:
	movaps xmm0,[rax]
    ;movdqa [rbx+10],xmm0
	movaps xmm1,[rax+10h]
	;movdqa [rbx+20],xmm1
	movaps xmm0,[rax+20h]
	;movdqa [rbx+30],xmm0
	movaps xmm1,[rax+30h]
	;movdqa [rbx+40],xmm1
exit:
	jmp qword ptr [_cameraStructInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraStructInterceptor ENDP

fovintercept PROC
;sekiro.exe+73C688 - F3 0F10 08            - movss xmm1,[rax]						
;sekiro.exe+73C68C - F3 0F59 0D 04CAB402   - mulss xmm1,[sekiro.exe+3289098]
;sekiro.exe+73C694 - F3 0F5C 4E 50         - subss xmm1,[rsi+50]
;sekiro.exe+73C699 - F3 41 0F59 8E 34010000  - mulss xmm1,[r14+00000134]
;sekiro.exe+73C6A2 - F3 0F58 4E 50         - addss xmm1,[rsi+50]					<<inject			
;sekiro.exe+73C6A7 - F3 0F11 4E 50         - movss [rsi+50],xmm1					
;sekiro.exe+73C6AC - F3 0F10 33            - movss xmm6,[rbx]						
;sekiro.exe+73C6B0 - F3 45 0F10 86 34010000  - movss xmm8,[r14+00000134]			<<return
;sekiro.exe+73C6B9 - E8 924A0F00           - call sekiro.exe+831150
	mov [g_uwfovaddress],rax
	addss xmm1, dword ptr [rsi+50h]
	movss dword ptr [rsi+50h],xmm1
	movss xmm6, dword ptr [rbx]
	jmp qword ptr [_fovinterceptContinue]
fovintercept ENDP


debugcaminterceptor PROC
;sekiro.exe+B22902 - 8B 41 50              - mov eax,[rcx+50]
;sekiro.exe+B22905 - 89 43 50              - mov [rbx+50],eax						<<fov write
;sekiro.exe+B22908 - 8B 41 54              - mov eax,[rcx+54]
;sekiro.exe+B2290B - 89 43 54              - mov [rbx+54],eax
;sekiro.exe+B2290E - 8B 41 58              - mov eax,[rcx+58]
;sekiro.exe+B22911 - 89 43 58              - mov [rbx+58],eax
;sekiro.exe+B22914 - 8B 41 5C              - mov eax,[rcx+5C]
;sekiro.exe+B22917 - 89 43 5C              - mov [rbx+5C],eax
;sekiro.exe+B2291A - E8 21 19 C1 FF        - call sekiro.exe+734240
;sekiro.exe+B2291F - 0F28 00               - movaps xmm0,[rax]						<<inject here
;sekiro.exe+B22922 - 66 0F7F 43 10         - movdqa [rbx+10],xmm0					<<row 1
;sekiro.exe+B22927 - 0F28 48 10            - movaps xmm1,[rax+10]
;sekiro.exe+B2292B - 66 0F7F 4B 20         - movdqa [rbx+20],xmm1					<<row 2
;sekiro.exe+B22930 - 0F28 40 20            - movaps xmm0,[rax+20]
;sekiro.exe+B22934 - 66 0F7F 43 30         - movdqa [rbx+30],xmm0					<<row 3
;sekiro.exe+B22939 - 0F28 48 30            - movaps xmm1,[rax+30]
;sekiro.exe+B2293D - 66 0F7F 4B 40         - movdqa [rbx+40],xmm1					<<row 4 coords
;sekiro.exe+B22942 - 83 67 28 C0           - and dword ptr [rdi+28],-40 { 192 }		<<return here
;sekiro.exe+B22946 - 48 8B 5C 24 70        - mov rbx,[rsp+70]
;sekiro.exe+B2294B - 48 83 C4 60           - add rsp,60 { 96 }
;sekiro.exe+B2294F - 5F                    - pop rdi
;sekiro.exe+B22950 - C3                    - ret 
;sekiro.exe+B22951 - A8 10                 - test al,10 { 16 }
	mov [g_cameraStructAddress], rbx
	cmp byte ptr [g_cameraEnabled], 1   ;check if camera is enabled, then we can jump to our custom code if it is
	je skipwrite
	movaps xmm0,[rax]		
	movdqa [rbx+10h],xmm0
	movaps xmm1,[rax+10h]
	movdqa [rbx+20h],xmm1					
	movaps xmm0,[rax+20h]
	movdqa [rbx+30h],xmm0					
	movaps xmm1,[rax+30h]
	movdqa [rbx+40h],xmm1
	jmp exit
skipwrite:
	movaps xmm0,[rax]		
	;movdqa [rbx+10h],xmm0
	movaps xmm1,[rax+10h]
	;movdqa [rbx+20h],xmm1					
	movaps xmm0,[rax+20h]
	;movdqa [rbx+30h],xmm0					
	movaps xmm1,[rax+30h]
	;movdqa [rbx+40h],xmm1
exit:
	jmp qword ptr [_debugcamInterceptionContinue] ;jmp back into code
debugcaminterceptor ENDP

dofStruct PROC
;sekiro.exe+11B7C02 - 41 8B 41 28           - mov eax,[r9+28]
;sekiro.exe+11B7C06 - 0F44 CA               - cmove ecx,edx
;sekiro.exe+11B7C09 - 8B D1                 - mov edx,ecx
;sekiro.exe+11B7C0B - 0FBA EA 07            - bts edx,07 { 7 }
;sekiro.exe+11B7C0F - 41 3B 40 28           - cmp eax,[r8+28]		<<<inject here/compare dof setting/inject here
;sekiro.exe+11B7C13 - 41 8B 41 2C           - mov eax,[r9+2C]
;sekiro.exe+11B7C17 - 0F44 D1               - cmove edx,ecx
;sekiro.exe+11B7C1A - 8B CA                 - mov ecx,edx
;sekiro.exe+11B7C1C - 0FBA E9 08            - bts ecx,08 { 8        
;sekiro.exe+11B7C20 - 41 3B 40 2C           - cmp eax,[r8+2C]		<<<<return here
;sekiro.exe+11B7C24 - 41 8B 41 30           - mov eax,[r9+30]
;sekiro.exe+11B7C28 - 0F44 CA               - cmove ecx,edx
	mov [g_dofstructaddress],r8
	cmp eax,[r8+28h]
	mov eax,[r9+2Ch]
	cmove edx,ecx
	mov ecx,edx
	bts ecx,08h
	jmp qword ptr [_dofInterceptionContinue]
dofStruct ENDP

timescaleinterceptor PROC
;sekiro.exe+1194F13: 4C 8D 05 9E 64 0E 02     - lea r8,[sekiro.exe+327B3B8]
;sekiro.exe+1194F1A: BA B1 00 00 00           - mov edx,000000B1
;sekiro.exe+1194F1F: 48 8D 0D 8A 07 7B 01     - lea rcx,[sekiro.exe+29456B0]
;sekiro.exe+1194F26: E8 75 79 84 00           - call sekiro.exe+19DC8A0
;sekiro.exe+1194F2B: 48 8B 05 16 A8 D0 02     - mov rax,[sekiro.exe+3E9F748]
;sekiro.exe+1194F32: F3 0F 10 88 44 03 00 00  - movss xmm1,[rax+00000344]
;sekiro.exe+1194F3A: F3 0F 59 88 68 02 00 00  - mulss xmm1,[rax+00000268]		<<inject here/timestruct read
;sekiro.exe+1194F42: F3 0F 59 88 64 03 00 00  - mulss xmm1,[rax+00000364]
;sekiro.exe+1194F4A: 48 8D 3D B7 30 7B 01     - lea rdi,[sekiro.exe+2948008]	<<return here
;sekiro.exe+1194F51: 48 89 7C 24 28           - mov [rsp+28],rdi
;sekiro.exe+1194F56: 0F 57 C0                 - xorps xmm0,xmm0
;sekiro.exe+1194F59: F3 0F 11 44 24 30        - movss [rsp+30],xmm0
;sekiro.exe+1194F5F: 48 8D 35 B2 30 7B 01     - lea rsi,[sekiro.ex
	mov [g_timescaleaddress],rax
	mulss xmm1,dword ptr [rax+00000268h]
	mulss xmm1,dword ptr [rax+00000364h]
	jmp qword ptr [_timestructinterceptionContinue]
timescaleinterceptor ENDP

playerpointerinterceptor PROC
;sekiro.exe+B7D984 - 45 33 F6              - xor r14d,r14d
;sekiro.exe+B7D987 - 41 8B FE              - mov edi,r14d
;sekiro.exe+B7D98A - 45 8B FE              - mov r15d,r14d
;sekiro.exe+B7D98D - 0F57 C0               - xorps xmm0,xmm0
;sekiro.exe+B7D990 - F3 0F7F 44 24 38      - movdqu [rsp+38],xmm0
;sekiro.exe+B7D996 - 48 8B 80 F81F0000     - mov rax,[rax+00001FF8]		<<inject here
;sekiro.exe+B7D99D - 48 8B 48 28           - mov rcx,[rax+28]
;sekiro.exe+B7D9A1 - 48 8B 49 10           - mov rcx,[rcx+10]			<<read player pointer
;sekiro.exe+B7D9A5 - 48 85 C9              - test rcx,rcx				<<return
;sekiro.exe+B7D9A8 - 74 40                 - je sekiro.exe+B7D9EA
;sekiro.exe+B7D9AA - E8 E1054800           - call sekiro.exe+FFDF90
;sekiro.exe+B7D9AF - 48 85 C0              - test rax,rax
	mov rax,[rax+00001FF8h]
	mov rcx,[rax+28h]
	mov [g_playerpointer],rcx
	mov rcx,[rcx+10h]
	jmp qword ptr [_playerpointerContinue]
playerpointerinterceptor ENDP


entitytimeinterceptor PROC
;sekiro.exe+B454E7 - E8 E41CFEFF           - call sekiro.exe+B271D0
;sekiro.exe+B454EC - 48 8B 88 F81F0000     - mov rcx,[rax+00001FF8]
;sekiro.exe+B454F3 - 48 8B 89 B8000000     - mov rcx,[rcx+000000B8]
;sekiro.exe+B454FA - 48 85 C9              - test rcx,rcx
;sekiro.exe+B454FD - 74 7D                 - je sekiro.exe+B4557C
;sekiro.exe+B454FF - E8 9C60FFFF           - call sekiro.exe+B3B5A0
;sekiro.exe+B45504 - 0F28 D0               - movaps xmm2,xmm0
;sekiro.exe+B45507 - F3 0F10 83 000D0000   - movss xmm0,[rbx+00000D00]			<<inject - moves intended per entity timescale factor into xmm0 (writable)
;sekiro.exe+B4550F - F3 0F59 83 60090000   - mulss xmm0,[rbx+00000960]			<< multiplies actual timescale by factor
;sekiro.exe+B45517 - F3 0F59 D0            - mulss xmm2,xmm0					<<return
;sekiro.exe+B4551B - 80 BB 0D0D0000 00     - cmp byte ptr [rbx+00000D0D],00 { 0 }
;sekiro.exe+B45522 - 74 03                 - je sekiro.exe+B45527
;sekiro.exe+B45524 - 0F57 D2               - xorps xmm2,xmm2
;sekiro.exe+B45527 - 48 8D 8B 30090000     - lea rcx,[rbx+00000930]
;sekiro.exe+B4552E - 48 8D 54 24 40        - lea rdx,[rsp+40]
	cmp byte ptr [g_playersonly],0
	je originalcode
	cmp rbx,qword ptr [g_playerpointer]
	je originalcode
	movss xmm0,dword ptr [timevalue]
	mulss xmm0,dword ptr [rbx+00000960h]
	jmp exit
originalcode:
	movss xmm0,dword ptr [rbx+00000D00h]
	mulss xmm0,dword ptr [rbx+00000960h]
exit:
	jmp qword ptr [_entitytimeContinue]
entitytimeinterceptor ENDP

END