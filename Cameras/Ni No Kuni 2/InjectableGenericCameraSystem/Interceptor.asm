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
PUBLIC cameraFOVInterceptor
PUBLIC timescaleInterceptor
PUBLIC HUDinterceptor

;---------------------------------------------------------------

;---------------------------------------------------------------
; Externs which are used and set by the system. Read / write these
; values in asm to communicate with the system
EXTERN g_cameraEnabled: byte
EXTERN g_timestopEnabled: byte
EXTERN g_cameraStructAddress: qword
EXTERN g_fovStructAddress: qword
EXTERN g_timescaleAddress: qword
EXTERN g_HUDaddress: qword

;---------------------------------------------------------------

;---------------------------------------------------------------
; Own externs, defined in InterceptorHelper.cpp
EXTERN _cameraStructInterceptionContinue: qword
EXTERN _fovStructInterceptionContinue: qword
EXTERN _timescaleInterceptionContinue: qword
EXTERN _timestopOneAbsolute: qword
EXTERN _timestopTwoAbsolute: qword
EXTERN _structabsoluteAddrress: qword
EXTERN _HUDinterceptionContinue: qword

.data


.code

timevalue dd 0.0

cameraStructInterceptor PROC
;Nino2.exe+39DF07 - 0F10 00               - movups xmm0,[rax]					<INJECT
;Nino2.exe+39DF0A - 0F11 07               - movups [rdi],xmm0					<SKIP
;Nino2.exe+39DF0D - 4C 8D 9C 24 B8000000  - lea r11,[rsp+000000B8]
;Nino2.exe+39DF15 - 0F10 48 10            - movups xmm1,[rax+10]
;Nino2.exe+39DF19 - 0F11 4F 10            - movups [rdi+10],xmm1				<SKIP
;Nino2.exe+39DF1D - 0F10 40 20            - movups xmm0,[rax+20]
;Nino2.exe+39DF21 - 0F11 47 20            - movups [rdi+20],xmm0				<SKIP
;Nino2.exe+39DF25 - 0F10 48 30            - movups xmm1,[rax+30]
;Nino2.exe+39DF29 - 48 8B C7              - mov rax,rdi
;Nino2.exe+39DF2C - F3 0F10 05 ECC6A400   - movss xmm0,[Nino2.exe+DEA620] { (0.00) }
;Nino2.exe+39DF34 - 0F11 4F 30            - movups [rdi+30],xmm1
;Nino2.exe+39DF38 - F3 44 0F11 5F 04      - movss [rdi+04],xmm11
;Nino2.exe+39DF3E - F3 0F11 3F            - movss [rdi],xmm7					<SKIP
;Nino2.exe+39DF42 - F3 44 0F11 57 08      - movss [rdi+08],xmm10				<SKIP
;Nino2.exe+39DF48 - F3 44 0F11 47 10      - movss [rdi+10],xmm8					<SKIP
;Nino2.exe+39DF4E - F3 44 0F11 4F 14      - movss [rdi+14],xmm9					<SKIP
;Nino2.exe+39DF54 - F3 0F11 77 18         - movss [rdi+18],xmm6					<SKIP
;Nino2.exe+39DF59 - F3 44 0F11 67 20      - movss [rdi+20],xmm12				<SKIP
;Nino2.exe+39DF5F - F3 44 0F11 77 24      - movss [rdi+24],xmm14				<SKIP
;Nino2.exe+39DF65 - F3 44 0F11 6F 28      - movss [rdi+28],xmm13				<SKIP
;Nino2.exe+39DF6B - F3 0F10 4B 04         - movss xmm1,[rbx+04]					
;Nino2.exe+39DF70 - F3 0F10 13            - movss xmm2,[rbx]
;Nino2.exe+39DF74 - F3 0F10 5B 08         - movss xmm3,[rbx+08]
;Nino2.exe+39DF79 - F3 44 0F59 D9         - mulss xmm11,xmm1
;Nino2.exe+39DF7E - F3 44 0F59 C9         - mulss xmm9,xmm1
;Nino2.exe+39DF83 - F3 44 0F59 F1         - mulss xmm14,xmm1
;Nino2.exe+39DF88 - F3 0F59 FA            - mulss xmm7,xmm2
;Nino2.exe+39DF8C - F3 44 0F59 C2         - mulss xmm8,xmm2
;Nino2.exe+39DF91 - F3 44 0F58 DF         - addss xmm11,xmm7
;Nino2.exe+39DF96 - F3 44 0F59 E2         - mulss xmm12,xmm2
;Nino2.exe+39DF9B - 41 0F28 7B D8         - movaps xmm7,[r11-28]
;Nino2.exe+39DFA0 - F3 45 0F58 C8         - addss xmm9,xmm8
;Nino2.exe+39DFA5 - 45 0F28 43 C8         - movaps xmm8,[r11-38]
;Nino2.exe+39DFAA - F3 45 0F58 F4         - addss xmm14,xmm12
;Nino2.exe+39DFAF - F3 44 0F59 D3         - mulss xmm10,xmm3
;Nino2.exe+39DFB4 - 45 0F28 63 88         - movaps xmm12,[r11-78]
;Nino2.exe+39DFB9 - F3 0F59 F3            - mulss xmm6,xmm3
;Nino2.exe+39DFBD - F3 45 0F58 DA         - addss xmm11,xmm10
;Nino2.exe+39DFC2 - F3 44 0F59 EB         - mulss xmm13,xmm3
;Nino2.exe+39DFC7 - 45 0F28 53 A8         - movaps xmm10,[r11-58]
;Nino2.exe+39DFCC - F3 44 0F58 CE         - addss xmm9,xmm6
;Nino2.exe+39DFD1 - 41 0F28 73 E8         - movaps xmm6,[r11-18]
;Nino2.exe+39DFD6 - F3 45 0F58 F5         - addss xmm14,xmm13
;Nino2.exe+39DFDB - 44 0F28 6C 24 30      - movaps xmm13,[rsp+30]
;Nino2.exe+39DFE1 - 44 0F57 D8            - xorps xmm11,xmm0
;Nino2.exe+39DFE5 - F3 44 0F11 5F 0C      - movss [rdi+0C],xmm11			<<skip
;Nino2.exe+39DFEB - 45 0F28 5B 98         - movaps xmm11,[r11-68]
;Nino2.exe+39DFF0 - 44 0F57 C8            - xorps xmm9,xmm0
;Nino2.exe+39DFF4 - F3 44 0F11 4F 1C      - movss [rdi+1C],xmm9				<<skip
;Nino2.exe+39DFFA - 45 0F28 4B B8         - movaps xmm9,[r11-48]
;Nino2.exe+39DFFF - 44 0F57 F0            - xorps xmm14,xmm0
;Nino2.exe+39E003 - F3 44 0F11 77 2C      - movss [rdi+2C],xmm14			<<skip
;Nino2.exe+39E009 - 44 0F28 74 24 20      - movaps xmm14,[rsp+20]			<<return
;Nino2.exe+39E00F - 49 8B E3              - mov rsp,r11				
;Nino2.exe+39E012 - 5F                    - pop rdi
;Nino2.exe+39E013 - 5B                    - pop rbx
	cmp r15,1
	jne originalcode
	mov [g_cameraStructAddress],rdi
	cmp byte ptr [g_cameraEnabled],1
	je skipcode
originalcode:
	movups xmm0,[rax]
	movups [rdi],xmm0
	lea r11,[rsp+000000B8h]
	movups xmm1,[rax+10h]
	movups [rdi+10h],xmm1
	movups xmm0,[rax+20h]
	movups [rdi+20h],xmm0
	movups xmm1,[rax+30h]
	mov rax,rdi
	push rax
	mov rax, [_structabsoluteAddrress]
	movss xmm0, dword ptr [rax]
	pop rax
	movups [rdi+30h],xmm1
	movss dword ptr [rdi+04h],xmm11
	movss dword ptr [rdi],xmm7
	movss dword ptr [rdi+08h],xmm10
	movss dword ptr [rdi+10h],xmm8
	movss dword ptr [rdi+14h],xmm9
	movss dword ptr [rdi+18h],xmm6
	movss dword ptr [rdi+20h],xmm12
	movss dword ptr [rdi+24h],xmm14
	movss dword ptr [rdi+28h],xmm13
	movss xmm1,dword ptr [rbx+04]
	movss xmm2,dword ptr [rbx]
	movss xmm3,dword ptr [rbx+08h]
	mulss xmm11,xmm1
	mulss xmm9,xmm1
	mulss xmm14,xmm1
	mulss xmm7,xmm2
	mulss xmm8,xmm2
	addss xmm11,xmm7
	mulss xmm12,xmm2
	movaps xmm7,[r11-28h]
	addss xmm9,xmm8
	movaps xmm8,[r11-38h]
	addss xmm14,xmm12
	mulss xmm10,xmm3
	movaps xmm12,[r11-78h]
	mulss xmm6,xmm3
	addss xmm11,xmm10
	mulss xmm13,xmm3
	movaps xmm10,[r11-58h]
	addss xmm9,xmm6
	movaps xmm6,[r11-18h]
	addss xmm14,xmm13
	movaps xmm13,[rsp+30h]
	xorps xmm11,xmm0
	movss dword ptr [rdi+0Ch],xmm11
	movaps xmm11,[r11-68h]
	xorps xmm9,xmm0
	movss dword ptr [rdi+1Ch],xmm9
	movaps xmm9,[r11-48h]
	xorps xmm14,xmm0
	movss dword ptr [rdi+2Ch],xmm14
	jmp exit
skipcode:
   	movups xmm0,[rax]
	;movups [rdi],xmm0
	lea r11,[rsp+000000B8h]
	movups xmm1,[rax+10h]
	;movups [rdi+10h],xmm1
	movups xmm0,[rax+20h]
	;movups [rdi+20h],xmm0
	movups xmm1,[rax+30h]
	mov rax,rdi
	push rax
	mov rax, [_structabsoluteAddrress]
	movss xmm0, dword ptr [rax]
	pop rax
	;movups [rdi+30h],xmm1
	;movss [rdi+04h],xmm11
	;movss [rdi],xmm7
	;movss [rdi+08h],xmm10
	;movss [rdi+10h],xmm8
	;movss [rdi+14h],xmm9
	;movss [rdi+18h],xmm6
	;movss [rdi+20h],xmm12
	;movss [rdi+24h],xmm14
	;movss [rdi+28h],xmm13
	movss xmm1,dword ptr [rbx+04]
	movss xmm2,dword ptr [rbx]
	movss xmm3,dword ptr [rbx+08h]
	mulss xmm11,xmm1
	mulss xmm9,xmm1
	mulss xmm14,xmm1
	mulss xmm7,xmm2
	mulss xmm8,xmm2
	addss xmm11,xmm7
	mulss xmm12,xmm2
	movaps xmm7,[r11-28h]
	addss xmm9,xmm8
	movaps xmm8,[r11-38h]
	addss xmm14,xmm12
	mulss xmm10,xmm3
	movaps xmm12,[r11-78h]
	mulss xmm6,xmm3
	addss xmm11,xmm10
	mulss xmm13,xmm3
	movaps xmm10,[r11-58h]
	addss xmm9,xmm6
	movaps xmm6,[r11-18h]
	addss xmm14,xmm13
	movaps xmm13,[rsp+30h]
	xorps xmm11,xmm0
	;movss dword ptr [rdi+0Ch],xmm11
	movaps xmm11,[r11-68h]
	xorps xmm9,xmm0
	;movss dword ptr [rdi+1Ch],xmm9
	movaps xmm9,[r11-48h]
	xorps xmm14,xmm0
	;movss dword ptr [rdi+2Ch],xmm14
exit:
	jmp qword ptr [_cameraStructInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraStructInterceptor ENDP

cameraFOVInterceptor PROC
;Nino2.exe+B3DAE0 - 0F2F 83 9C020000      - comiss xmm0,[rbx+0000029C]
;Nino2.exe+B3DAE7 - 72 0F                 - jb Nino2.exe+B3DAF8
;Nino2.exe+B3DAE9 - 48 8B CB              - mov rcx,rbx
;Nino2.exe+B3DAEC - E8 2F7593FF           - call Nino2.exe+475020
;Nino2.exe+B3DAF1 - C6 83 AC020000 00     - mov byte ptr [rbx+000002AC],00 { 0 }
;Nino2.exe+B3DAF8 - 0F28 83 60010000      - movaps xmm0,[rbx+00000160]			<<intercept here
;Nino2.exe+B3DAFF - 0F29 87 80000000      - movaps [rdi+00000080],xmm0
;Nino2.exe+B3DB06 - 0F28 8B 70010000      - movaps xmm1,[rbx+00000170]			<<return here
;Nino2.exe+B3DB0D - 0F29 8F 90000000      - movaps [rdi+00000090],xmm1
;Nino2.exe+B3DB14 - 0F28 83 80010000      - movaps xmm0,[rbx+00000180]
;Nino2.exe+B3DB1B - 0F29 87 A0000000      - movaps [rdi+000000A0],xmm0
;Nino2.exe+B3DB22 - 0F28 8B 90010000      - movaps xmm1,[rbx+00000190]
	mov [g_fovStructAddress],rbx
	;cmp byte ptr [g_cameraEnabled],1
	movaps xmm0,[rbx+00000160h]
	movaps [rdi+00000080h],xmm0
exit:
	jmp qword ptr [_fovStructInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraFOVInterceptor ENDP

timescaleInterceptor PROC	
;Nino2.exe+9D6165 - F3 0F10 88 187C3500   - movss xmm1,[rax+00357C18]			<<INJECT
;Nino2.exe+9D616D - F3 0F10 90 1C7C3500   - movss xmm2,[rax+00357C1C]			<<<timestop
;Nino2.exe+9D6175 - 0FB7 80 287C3500      - movzx eax,word ptr [rax+00357C28]	<<return
;Nino2.exe+9D617C - F3 0F5C D1            - subss xmm2,xmm1
;Nino2.exe+9D6180 - 66 0F6E C0            - movd xmm0,eax
;Nino2.exe+9D6184 - 0F5B C0               - cvtdq2ps xmm0,xmm0
;Nino2.exe+9D6187 - F3 0F59 D0            - mulss xmm2,xmm0
;Nino2.exe+9D618B - F3 0F59 15 9D2D4100   - mulss xmm2,[Nino2.exe+DE8F30] { (0.00) }
	movss xmm1, dword ptr [rax+00357C18h]
	movss xmm2, dword ptr [rax+00357C1Ch]
	mov [g_timescaleAddress], rax
	jmp qword ptr [_timescaleInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
timescaleInterceptor ENDP







;Nino2.exe+9D627F - FF 50 08              - call qword ptr [rax+08]
;Nino2.exe+9D6282 - E8 89001900           - call Nino2.exe+B66310
;Nino2.exe+9D6287 - F3 0F11 35 35168B00   - movss [Nino2.exe+12878C4],xmm6 { (0.00) }				<<intercept here
;Nino2.exe+9D628F - F3 0F11 3D 5D168B00   - movss [Nino2.exe+12878F4],xmm7 { (0.02) }				
;Nino2.exe+9D6297 - E8 047B9DFF           - call Nino2.exe+3ADDA0									<<return here
;Nino2.exe+9D629C - E8 3F7C9DFF           - call Nino2.exe+3ADEE0
	;cmp byte ptr [g_timestopEnabled],1
	;je stopTime
	;push rcx
	;push rdx
	;mov rcx, [_timestopOneAbsolute]
	;mov rdx, [_timestopTwoAbsolute]
	;movss dword ptr [rcx],xmm6
	;movss dword ptr [rdx],xmm7
	;pop rcx
	;pop rdx
	;jmp exit
;stopTime:
	;push rbx
	;push rcx
	;push rdx
	;mov ebx, [timevalue]
	;mov rcx, [_timestopOneAbsolute]
	;mov rdx, [_timestopTwoAbsolute]
	;mov dword ptr [rcx], ebx
	;movss dword ptr [rdx],xmm7
	;pop rbx
	;pop rcx
	;pop rdx
;exit:
	;jmp qword ptr [_timescaleInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
;timescaleInterceptor ENDP

HUDinterceptor PROC
;Nino2.exe+4F98D7 - 8B 81 D0390000        - mov eax,[rcx+000039D0]
;Nino2.exe+4F98DD - F3 0F11 44 24 24      - movss [rsp+24],xmm0
;Nino2.exe+4F98E3 - 48 C7 44 24 1C 02000000 - mov qword ptr [rsp+1C],00000002 { 2 }
;Nino2.exe+4F98EC - 80 BA 44 61 00 00 00  - cmp byte ptr [rdx+00006144],00 { 0 }		<<<Intercept here
;Nino2.exe+4F98F3 - 48 89 4C 24 10        - mov [rsp+10],rcx
;Nino2.exe+4F98F8 - 89 44 24 18           - mov [rsp+18],eax
;Nino2.exe+4F98FC - 74 4E                 - je Nino2.exe+4F994C							<<<return here
;Nino2.exe+4F98FE - 0FB7 82 40610000      - movzx eax,word ptr [rdx+00006140]
;Nino2.exe+4F9905 - B9 00020000           - mov ecx,00000200 { 512 }
	mov [g_HUDaddress],rdx
	cmp byte ptr [rdx+00006144h],00
	mov [rsp+10h],rcx
	mov dword ptr [rsp+18h],eax
	jmp qword ptr [_HUDinterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
HUDinterceptor ENDP


END