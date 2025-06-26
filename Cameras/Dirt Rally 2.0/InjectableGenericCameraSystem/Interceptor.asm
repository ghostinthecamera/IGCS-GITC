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
PUBLIC cameraWriteInjection1
PUBLIC cameraWriteInjection2
PUBLIC cameraWriteInjection3
PUBLIC cameraWriteInjection4
PUBLIC cameraWriteInjection5
PUBLIC fovWriteInjection1
PUBLIC fovWriteInjection2
PUBLIC fovWriteInjection3
PUBLIC fovWriteInjection4
PUBLIC carPositionInterceptor
PUBLIC hudToggleInterceptor
PUBLIC timescaleInterceptor
PUBLIC dofInterceptor

;---------------------------------------------------------------

;---------------------------------------------------------------
; Externs which are used and set by the system. Read / write these
; values in asm to communicate with the system
EXTERN g_cameraEnabled: byte
EXTERN g_cameraStructAddress: qword
EXTERN g_cameraQuaternionAddress: qword
EXTERN g_cameraPositionAddress: qword
EXTERN g_fovAbsoluteAddress: qword
EXTERN g_carPositionAddress: qword
EXTERN g_hudEnabled: byte
EXTERN g_timescaleAddress: qword
EXTERN g_dofStrengthAddress: qword
;---------------------------------------------------------------

;---------------------------------------------------------------
; Own externs, defined in InterceptorHelper.cpp
EXTERN _cameraStructInterceptionContinue: qword
EXTERN _cameraWriteInjection1Continue: qword
EXTERN _cameraWriteInjection2Continue: qword
EXTERN _cameraWriteInjection3Continue: qword
EXTERN _cameraWriteInjection4Continue: qword
EXTERN _cameraWriteInjection5Continue: qword
EXTERN _fovWriteInjection1Continue: qword
EXTERN _fovWriteInjection2Continue: qword
EXTERN _fovWriteInjection3Continue: qword
EXTERN _fovWriteInjection4Continue: qword
EXTERN _carPositionInjectionContinue: qword
EXTERN _hudToggleInjectionContinue: qword
EXTERN _timescaleInjectionContinue: qword
EXTERN _dofInjectionContinue: qword

.data

.code


cameraStructInterceptor PROC
;aob 4C 8B A1 F0 38 04 00
;dirtrally2.exe+AF8513 - 0F85 C6050000         - jne dirtrally2.exe+AF8ADF
;dirtrally2.exe+AF8519 - 48 8B 81 A8370400     - mov rax,[rcx+000437A8]	
;dirtrally2.exe+AF8520 - 4D 89 63 E8           - mov [r11-18],r12
;dirtrally2.exe+AF8524 - 4C 8B A1 F0380400     - mov r12,[rcx+000438F0]		<<<INJECT/R12 is cam base
;dirtrally2.exe+AF852B - 4D 89 73 E0           - mov [r11-20],r14
;dirtrally2.exe+AF852F - 4C 8B B0 60030000     - mov r14,[rax+00000360]
;dirtrally2.exe+AF8536 - 4D 89 7B D8           - mov [r11-28],r15			<<RETURN
;dirtrally2.exe+AF853A - 45 32 FF              - xor r15b,r15b
;dirtrally2.exe+AF853D - 4D 85 F6              - test r14,r14
	mov r12,[rcx+000438F0h]
	mov [g_cameraStructAddress],r12
	push rdi
	lea rdi,[r12+130h]
	mov [g_cameraQuaternionAddress],rdi
	xor rdi,rdi
	lea rdi,[r12+140h]
	mov [g_cameraPositionAddress],rdi
	pop rdi
	mov [r11-20h],r14
	mov r14,[rax+00000360h]
	jmp qword ptr [_cameraStructInterceptionContinue]
cameraStructInterceptor ENDP

cameraWriteInjection1 PROC
;0F C6 D2 27 F3 0F 10 D1 0F C6 D2 27 0F 29 12 48
;dirtrally2.exe+C5252C - 0FC6 D2 C6            - shufps xmm2,xmm2,-3A { 198 }
;dirtrally2.exe+C52530 - 0F28 C7               - movaps xmm0,xmm7
;dirtrally2.exe+C52533 - 0F28 3C 24            - movaps xmm7,[rsp]
;dirtrally2.exe+C52537 - F3 0F10 D1            - movss xmm2,xmm1
;dirtrally2.exe+C5253B - 0FC6 D2 C6            - shufps xmm2,xmm2,-3A { 198 }
;dirtrally2.exe+C5253F - 0F28 C8               - movaps xmm1,xmm0
;dirtrally2.exe+C52542 - 0FC6 D2 27            - shufps xmm2,xmm2,27 { 39 }		<INJECT
;dirtrally2.exe+C52546 - F3 0F10 D1            - movss xmm2,xmm1
;dirtrally2.exe+C5254A - 0FC6 D2 27            - shufps xmm2,xmm2,27 { 39 }
;dirtrally2.exe+C5254E - 0F29 12               - movaps [rdx],xmm2				<<SKIP
;dirtrally2.exe+C52551 - 48 83 C4 28           - add rsp,28 { 40 }				<<RETURN
	shufps xmm2,xmm2,27h
	movss xmm2,xmm1
	shufps xmm2,xmm2,27h
	cmp byte ptr [g_cameraEnabled], 1
	jne originalcode
	cmp [g_cameraQuaternionAddress],rdx
	je skip
originalcode:
	movaps [rdx],xmm2
skip:
   jmp qword ptr [_cameraWriteInjection1Continue]
cameraWriteInjection1 ENDP


cameraWriteInjection2 PROC
;0F 29 03 F3 0F 5C 4B 70
;dirtrally2.exe+A3971E - E8 6D36AFFF           - call dirtrally2.exe+52CD90
;dirtrally2.exe+A39723 - 0F28 45 00            - movaps xmm0,[rbp+00]
;dirtrally2.exe+A39727 - F3 0F10 4D A0         - movss xmm1,[rbp-60]
;dirtrally2.exe+A3972C - 0F29 03               - movaps [rbx],xmm0			<<INJECT/SKIP
;dirtrally2.exe+A3972F - F3 0F5C 4B 70         - subss xmm1,[rbx+70]
;dirtrally2.exe+A39734 - F3 0F59 CF            - mulss xmm1,xmm7
;dirtrally2.exe+A39738 - F3 0F58 4B 70         - addss xmm1,[rbx+70]
;dirtrally2.exe+A3973D - F3 0F11 4B 70         - movss [rbx+70],xmm1		<<skip this for FOV too
;dirtrally2.exe+A39742 - F3 0F10 4D A8         - movss xmm1,[rbp-58]		<<return
;dirtrally2.exe+A39747 - F3 0F5C 4B 78         - subss xmm1,[rbx+78]
;dirtrally2.exe+A3974C - F3 0F59 CF            - mulss xmm1,xmm7			
;dirtrally2.exe+A39750 - F3 0F58 4B 78         - addss xmm1,[rbx+78]
;dirtrally2.exe+A39755 - F3 0F11 4B 78         - movss [rbx+78],xmm1
;dirtrally2.exe+A3975A - F3 0F10 83 AC000000   - movss xmm0,[rbx+000000AC]
	cmp byte ptr [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraQuaternionAddress],rbx
	je skip
noskip:
	movaps [rbx],xmm0
skip:
	subss xmm1,dword ptr [rbx+70h]
	mulss xmm1,xmm7
	addss xmm1,dword ptr [rbx+70h]
	cmp byte ptr [g_cameraEnabled], 1
	jne noskipB
	cmp [g_cameraQuaternionAddress],rbx
	je skipB
noskipB:
	movss dword ptr [rbx+70h],xmm1
skipB:
   jmp qword ptr [_cameraWriteInjection2Continue]
cameraWriteInjection2 ENDP

cameraWriteInjection3 PROC
;0F 29 43 10 0F 5C 73 50
;dirtrally2.exe+A396FE - 0F59 D7               - mulps xmm2,xmm7
;dirtrally2.exe+A39701 - 0F55 CA               - andnps xmm1,xmm2
;dirtrally2.exe+A39704 - 0F58 4B 10            - addps xmm1,[rbx+10]
;dirtrally2.exe+A39708 - 0F55 C1               - andnps xmm0,xmm1
;dirtrally2.exe+A3970B - 0F29 43 10            - movaps [rbx+10],xmm0		<<INJECT/SKIP
;dirtrally2.exe+A3970F - 0F5C 73 50            - subps xmm6,[rbx+50]
;dirtrally2.exe+A39713 - 0F59 F7               - mulps xmm6,xmm7
;dirtrally2.exe+A39716 - 0F58 73 50            - addps xmm6,[rbx+50]
;dirtrally2.exe+A3971A - 0F29 73 50            - movaps [rbx+50],xmm6		<<RETURN
;dirtrally2.exe+A3971E - E8 6D36AFFF           - call dirtrally2.exe+52CD90
	cmp byte ptr [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraQuaternionAddress],rbx
	je skip
noskip:
	movaps [rbx+10h],xmm0
skip:
	subps xmm6,[rbx+50h]
	mulps xmm6,xmm7
	addps xmm6,[rbx+50h]
   jmp qword ptr [_cameraWriteInjection3Continue]
cameraWriteInjection3 ENDP


cameraWriteInjection4 PROC
;0F C6 D2 27 0F 29 56 50 0F
;dirtrally2.exe+A39CCB - F3 0F58 C8            - addss xmm1,xmm0
;dirtrally2.exe+A39CCF - 0F57 C0               - xorps xmm0,xmm0
;dirtrally2.exe+A39CD2 - F3 0F11 8E A4010000   - movss [rsi+000001A4],xmm1
;dirtrally2.exe+A39CDA - 0F28 C8               - movaps xmm1,xmm0
;dirtrally2.exe+A39CDD - 0F28 56 50            - movaps xmm2,[rsi+50]
;dirtrally2.exe+A39CE1 - 0FC6 D2 27            - shufps xmm2,xmm2,27 { 39 }
;dirtrally2.exe+A39CE5 - F3 0F10 D1            - movss xmm2,xmm1
;dirtrally2.exe+A39CE9 - 0FC6 D2 27            - shufps xmm2,xmm2,27 { 39 }				<<INJECT
;dirtrally2.exe+A39CED - 0F29 56 50            - movaps [rsi+50],xmm2
;dirtrally2.exe+A39CF1 - 0F28 86 20010000      - movaps xmm0,[rsi+00000120]
;dirtrally2.exe+A39CF8 - 66 0F7F 06            - movdqa [rsi],xmm0						<<SKIP
;dirtrally2.exe+A39CFC - E8 6F61C8FF           - call dirtrally2.AK::MusicEngine::Term	<<RETURN
;dirtrally2.exe+A39D01 - 4C 8D 9C 24 20020000  - lea r11,[rsp+00000220]
;dirtrally2.exe+A39D09 - 49 8B 5B 10           - mov rbx,[r11+10]
	shufps xmm2,xmm2,27h
	movaps [rsi+50h],xmm2
	movaps xmm0,[rsi+00000120h]
	cmp byte ptr [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraQuaternionAddress],rsi
	je skip
noskip:
	movdqa [rsi],xmm0
skip:
	jmp qword ptr [_cameraWriteInjection4Continue]
cameraWriteInjection4 ENDP

cameraWriteInjection5 PROC
;F3 0F 10 5C 24 58 0F 14 D8 0F
;dirtrally2.exe+ADD080 - FF 50 10              - call qword ptr [rax+10]
;dirtrally2.exe+ADD083 - F3 0F10 44 24 40      - movss xmm0,[rsp+40]
;dirtrally2.exe+ADD089 - 0F57 C9               - xorps xmm1,xmm1
;dirtrally2.exe+ADD08C - F3 0F10 54 24 48      - movss xmm2,[rsp+48]
;dirtrally2.exe+ADD092 - F3 0F10 5C 24 58      - movss xmm3,[rsp+58]		<<INJECT
;dirtrally2.exe+ADD098 - 0F14 D8               - unpcklps xmm3,xmm0
;dirtrally2.exe+ADD09B - 0F14 D1               - unpcklps xmm2,xmm1
;dirtrally2.exe+ADD09E - 0F14 DA               - unpcklps xmm3,xmm2
;dirtrally2.exe+ADD0A1 - 0F29 1E               - movaps [rsi],xmm3			<<SKIP ROTATION AND POSITION
;dirtrally2.exe+ADD0A4 - 48 83 C4 20           - add rsp,20 { 32 }			<<RETURN
;dirtrally2.exe+ADD0A8 - 5F                    - pop rdi
	movss xmm3, dword ptr [rsp+58h]
	unpcklps xmm3,xmm0
	unpcklps xmm2,xmm1
	unpcklps xmm3,xmm2
	cmp byte ptr [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraPositionAddress],rsi
	je skip
noskip:
	movaps [rsi],xmm3
skip:
	jmp qword ptr [_cameraWriteInjection5Continue]
cameraWriteInjection5 ENDP

fovWriteInjection1 PROC
;F3 0F 11 47 70 8B 83
;dirtrally2.exe+A66392 - 0F56 C8               - orps xmm1,xmm0
;dirtrally2.exe+A66395 - 0F29 4F 50            - movaps [rdi+50],xmm1
;dirtrally2.exe+A66399 - E8 4201FCFF           - call dirtrally2.exe+A264E0
;dirtrally2.exe+A6639E - 48 8D 8B 20070000     - lea rcx,[rbx+00000720]
;dirtrally2.exe+A663A5 - 4C 8B C7              - mov r8,rdi
;dirtrally2.exe+A663A8 - 49 8B D6              - mov rdx,r14
;dirtrally2.exe+A663AB - 0F28 00               - movaps xmm0,[rax]
;dirtrally2.exe+A663AE - 66 0F7F 07            - movdqa [rdi],xmm0
;dirtrally2.exe+A663B2 - 8B 43 18              - mov eax,[rbx+18]
;dirtrally2.exe+A663B5 - 89 47 78              - mov [rdi+78],eax
;dirtrally2.exe+A663B8 - F3 0F10 43 1C         - movss xmm0,[rbx+1C]
;dirtrally2.exe+A663BD - F3 0F59 05 AB727D00   - mulss xmm0,[dirtrally2.exe+123D670] { (0.02) }
;dirtrally2.exe+A663C5 - F3 0F11 47 70         - movss [rdi+70],xmm0			<check cam and skip
;dirtrally2.exe+A663CA - 8B 83 E0070000        - mov eax,[rbx+000007E0]
;dirtrally2.exe+A663D0 - 89 87 90010000        - mov [rdi+00000190],eax
;dirtrally2.exe+A663D6 - 48 8B 01              - mov rax,[rcx]
;dirtrally2.exe+A663D9 - C6 83 28070000 01     - mov byte ptr [rbx+00000728],01 { 1 }
;dirtrally2.exe+A663E0 - FF 50 10              - call qword ptr [rax+10]
;dirtrally2.exe+A663E3 - C6 83 28010000 00     - mov byte ptr [rbx+00000128],00 { 0 }
;dirtrally2.exe+A663EA - 49 8B 46 10           - mov rax,[r14+10]
;dirtrally2.exe+A663EE - 48 85 C0              - test rax,rax;
	cmp byte ptr [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraQuaternionAddress], rdi
	je skip
noskip:
	movss dword ptr [rdi+70h],xmm0
skip:
	mov eax,[rbx+000007E0h]
	mov [rdi+00000190h],eax
	jmp qword ptr [_fovWriteInjection1Continue]
fovWriteInjection1 ENDP

fovWriteInjection2 PROC
;F3 41 0F 11 46 70 8B 46 18
;dirtrally2.exe+A65627 - F3 0F10 35 41807D00   - movss xmm6,[dirtrally2.exe+123D670] { (0.02) }
;dirtrally2.exe+A6562F - 0F28 BC 24 D0000000   - movaps xmm7,[rsp+000000D0]
;dirtrally2.exe+A65637 - 66 41 0F7F 06         - movdqa [r14],xmm0
;dirtrally2.exe+A6563C - 66 41 0F7F 4E 10      - movdqa [r14+10],xmm1
;dirtrally2.exe+A65642 - 49 89 86 A0000000     - mov [r14+000000A0],rax
;dirtrally2.exe+A65649 - F3 0F10 46 1C         - movss xmm0,[rsi+1C]
;dirtrally2.exe+A6564E - F3 0F59 C6            - mulss xmm0,xmm6
;dirtrally2.exe+A65652 - F3 41 0F11 46 70      - movss [r14+70],xmm0		<<check cam and skip/inject
;dirtrally2.exe+A65658 - 8B 46 18              - mov eax,[rsi+18]
;dirtrally2.exe+A6565B - 41 89 46 78           - mov [r14+78],eax
;dirtrally2.exe+A6565F - F3 0F10 8E 5C070000   - movss xmm1,[rsi+0000075C]
;dirtrally2.exe+A65667 - F3 0F59 0D 75267900   - mulss xmm1,[dirtrally2.exe+11F7CE4] { (57.30) }	<<return
;dirtrally2.exe+A6566F - F3 0F10 41 0C         - movss xmm0,[rcx+0C]
;dirtrally2.exe+A65674 - F3 0F10 51 10         - movss xmm2,[rcx+10]
;dirtrally2.exe+A65679 - 0F2F C8               - comiss xmm1,xmm0
;dirtrally2.exe+A6567C - 0F57 15 ADEC7800      - xorps xmm2,[dirtrally2.exe+11F4330] { (-2147483648) }
;dirtrally2.exe+A65683 - 77 07                 - ja dirtrally2.exe+A6568C
;dirtrally2.exe+A65685 - 0F28 C1               - movaps xmm0,xmm1
;dirtrally2.exe+A65688 - F3 0F5F C2            - maxss xmm0,xmm2
;dirtrally2.exe+A6568C - 4D 8D 86 38010000     - lea r8,[r14+00000138]
	cmp byte ptr [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraQuaternionAddress], r14
	je skip
noskip:	
	movss dword ptr [r14+70h],xmm0
skip:
	mov eax,[rsi+18h]
	mov [r14+78h],eax
	movss xmm1, dword ptr [rsi+0000075Ch]
	jmp qword ptr [_fovWriteInjection2Continue]
fovWriteInjection2 ENDP

fovWriteInjection3 PROC
;0F B6 44 24 68 0F 5B C9
;dirtrally2.exe+A2C56C - 0F54 C1               - andps xmm0,xmm1
;dirtrally2.exe+A2C56F - 0F55 4C 24 30         - andnps xmm1,[rsp+30]
;dirtrally2.exe+A2C574 - 0F56 C1               - orps xmm0,xmm1
;dirtrally2.exe+A2C577 - 0F29 43 50            - movaps [rbx+50],xmm0
;dirtrally2.exe+A2C57B - 66 0F6E C0            - movd xmm0,eax
;dirtrally2.exe+A2C57F - 0FB7 44 24 20         - movzx eax,word ptr [rsp+20]
;dirtrally2.exe+A2C584 - 0F5B C0               - cvtdq2ps xmm0,xmm0
;dirtrally2.exe+A2C587 - 66 0F6E C8            - movd xmm1,eax
;dirtrally2.exe+A2C58B - F3 0F59 05 059F9700   - mulss xmm0,[dirtrally2.exe+13A6498] { (0.00) }
;dirtrally2.exe+A2C593 - 0FB6 44 24 68         - movzx eax,byte ptr [rsp+68]		<<inject
;dirtrally2.exe+A2C598 - 0F5B C9               - cvtdq2ps xmm1,xmm1
;dirtrally2.exe+A2C59B - F3 0F11 43 70         - movss [rbx+70],xmm0		<<check cam and return
;dirtrally2.exe+A2C5A0 - F3 0F59 0D 009F9700   - mulss xmm1,[dirtrally2.exe+13A64A8] { (0.00) }		<<annoyingly we need to get the absolute address here
;dirtrally2.exe+A2C5A8 - 66 0F6E C0            - movd xmm0,eax		<<return
;dirtrally2.exe+A2C5AC - 0F5B C0               - cvtdq2ps xmm0,xmm0
;dirtrally2.exe+A2C5AF - F3 0F11 4B 78         - movss [rbx+78],xmm1
;dirtrally2.exe+A2C5B4 - F3 0F59 05 CC138000   - mulss xmm0,[dirtrally2.exe+122D988] { (0.00) }
;dirtrally2.exe+A2C5BC - F3 0F11 83 AC000000   - movss [rbx+000000AC],xmm0
;dirtrally2.exe+A2C5C4 - 48 83 C4 40           - add rsp,40 { 64 }
;dirtrally2.exe+A2C5C8 - 5F                    - pop rdi
;dirtrally2.exe+A2C5C9 - 5E                    - pop rsi
;dirtrally2.exe+A2C5CA - 5B                    - pop rbx
;dirtrally2.exe+A2C5CB - C3                    - ret 
	movzx eax,byte ptr [rsp+68h]
	cvtdq2ps xmm1,xmm1
	cmp byte ptr [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraQuaternionAddress], rbx
	je skip
noskip:
	movss dword ptr [rbx+70h],xmm0
skip:
	mulss xmm1, dword ptr [g_fovAbsoluteAddress]
	jmp qword ptr [_fovWriteInjection3Continue]
fovWriteInjection3 ENDP

fovWriteInjection4 PROC
;F3 0F 11 4B 70 F3 0F 10 4D A8
;dirtrally2.exe+A3971A - 0F29 73 50            - movaps [rbx+50],xmm6
;dirtrally2.exe+A3971E - E8 6D36AFFF           - call dirtrally2.exe+52CD90
;dirtrally2.exe+A39723 - 0F28 45 00            - movaps xmm0,[rbp+00]
;dirtrally2.exe+A39727 - F3 0F10 4D A0         - movss xmm1,[rbp-60]
;dirtrally2.exe+A3972C - 0F29 03               - movaps [rbx],xmm0
;dirtrally2.exe+A3972F - F3 0F5C 4B 70         - subss xmm1,[rbx+70]
;dirtrally2.exe+A39734 - F3 0F59 CF            - mulss xmm1,xmm7
;dirtrally2.exe+A39738 - F3 0F58 4B 70         - addss xmm1,[rbx+70]
;dirtrally2.exe+A3973D - F3 0F11 4B 70         - movss [rbx+70],xmm1		<<inject, check cam and skip
;dirtrally2.exe+A39742 - F3 0F10 4D A8         - movss xmm1,[rbp-58]
;dirtrally2.exe+A39747 - F3 0F5C 4B 78         - subss xmm1,[rbx+78]
;dirtrally2.exe+A3974C - F3 0F59 CF            - mulss xmm1,xmm7			<<return here
;dirtrally2.exe+A39750 - F3 0F58 4B 78         - addss xmm1,[rbx+78]
;dirtrally2.exe+A39755 - F3 0F11 4B 78         - movss [rbx+78],xmm1
;dirtrally2.exe+A3975A - F3 0F10 83 AC000000   - movss xmm0,[rbx+000000AC]
;dirtrally2.exe+A39762 - F3 0F10 4D DC         - movss xmm1,[rbp-24]
;dirtrally2.exe+A39767 - F3 0F5C C8            - subss xmm1,xmm0
;dirtrally2.exe+A3976B - F3 0F59 CF            - mulss xmm1,xmm7
;dirtrally2.exe+A3976F - F3 0F58 C8            - addss xmm1,xmm0
;dirtrally2.exe+A39773 - F3 0F11 8B AC000000   - movss [rbx+000000AC],xmm1
;dirtrally2.exe+A3977B - F3 0F10 83 90000000   - movss xmm0,[rbx+00000090]
	cmp byte ptr [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraQuaternionAddress], rbx
	je skip
noskip:	
	movss dword ptr [rbx+70h],xmm1
skip:
	movss xmm1,dword ptr [rbp-58h]
	subss xmm1,dword ptr[rbx+78h]
	jmp qword ptr [_fovWriteInjection4Continue]
fovWriteInjection4 ENDP

carPositionInterceptor PROC
;dirtrally2.exe+7490B0 - 48 83 EC 18           - sub rsp,18 { 24 }
;dirtrally2.exe+7490B4 - F3 0F10 81 C0020000   - movss xmm0,[rcx+000002C0]
;dirtrally2.exe+7490BC - F3 0F10 A9 C4020000   - movss xmm5,[rcx+000002C4]
;dirtrally2.exe+7490C4 - F3 0F10 89 C8020000   - movss xmm1,[rcx+000002C8]
;dirtrally2.exe+7490CC - F3 0F10 99 B0020000   - movss xmm3,[rcx+000002B0] <<INJECT/take rcx
;dirtrally2.exe+7490D4 - F3 0F10 A1 B8020000   - movss xmm4,[rcx+000002B8]
;dirtrally2.exe+7490DC - F3 0F59 ED            - mulss xmm5,xmm5			<<return
;dirtrally2.exe+7490E0 - 0F29 34 24            - movaps [rsp],xmm6
;dirtrally2.exe+7490E4 - F3 0F10 B1 B4020000   - movss xmm6,[rcx+000002B4]
;dirtrally2.exe+7490EC - F3 0F59 A9 64030000   - mulss xmm5,[rcx+00000364]
;dirtrally2.exe+7490F4 - F3 0F59 F6            - mulss xmm6,xmm6
;dirtrally2.exe+7490F8 - F3 0F59 C9            - mulss xmm1,xmm1
;dirtrally2.exe+7490FC - F3 0F59 C0            - mulss xmm0,xmm0
	movss xmm3,dword ptr [rcx+000002B0h]
	movss xmm4,dword ptr [rcx+000002B8h]
	mov [g_carPositionAddress],rcx
	jmp qword ptr [_carPositionInjectionContinue]
carPositionInterceptor ENDP

hudToggleInterceptor PROC
;dirtrally2.exe+D7D6A0 - 48 83 EC 58           - sub rsp,58 { 88 }
;dirtrally2.exe+D7D6A4 - 41 0F10 08            - movups xmm1,[r8]			<<r8 has the RGBA values that are supposed to be written, and these are copied into xmm1
;dirtrally2.exe+D7D6A8 - 48 8B 81 70010000     - mov rax,[rcx+00000170]		<<jump in here
;dirtrally2.exe+D7D6AF - 41 0F10 40 10         - movups xmm0,[r8+10]
;dirtrally2.exe+D7D6B4 - 0F29 4C 24 30         - movaps [rsp+30],xmm1		<<if we want to disable hud, clear out xmm1 and then proceed with this code. use g_hudenabled or something
;dirtrally2.exe+D7D6B9 - 0F29 44 24 40         - movaps [rsp+40],xmm0		<<can return here
;dirtrally2.exe+D7D6BE - 48 85 C0              - test rax,rax				
;dirtrally2.exe+D7D6C1 - 74 09                 - je dirtrally2.exe+D7D6CC
;dirtrally2.exe+D7D6C3 - 0F59 48 30            - mulps xmm1,[rax+30]
;dirtrally2.exe+D7D6C7 - 0F29 4C 24 30         - movaps [rsp+30],xmm1
;dirtrally2.exe+D7D6CC - 0FB6 84 24 88000000   - movzx eax,byte ptr [rsp+00000088]
;dirtrally2.exe+D7D6D4 - 4C 8D 44 24 30        - lea r8,[rsp+30]
;dirtrally2.exe+D7D6D9 - 88 44 24 28           - mov [rsp+28],al
;dirtrally2.exe+D7D6DD - 0FB6 84 24 80000000   - movzx eax,byte ptr [rsp+00000080]
;dirtrally2.exe+D7D6E5 - 88 44 24 20           - mov [rsp+20],al
;dirtrally2.exe+D7D6E9 - E8 C2FCFFFF           - call dirtrally2.exe+D7D3B0
;dirtrally2.exe+D7D6EE - 48 83 C4 58           - add rsp,58 { 88 }
	mov rax,[rcx+00000170h]
	movups xmm0,[r8+10h]
	cmp byte ptr [g_hudEnabled],1
	je originalcode
	xorps xmm1,xmm1			; clear out the RGBA values
originalcode:
	movaps [rsp+30h],xmm1	
	jmp qword ptr [_hudToggleInjectionContinue]
hudToggleInterceptor ENDP

;timescaleInterceptor PROC
;dirtrally2.exe+CB0F10 - 40 53                 - push rbx
;dirtrally2.exe+CB0F12 - 48 83 EC 30           - sub rsp,30 { 48 }
;dirtrally2.exe+CB0F16 - 80 79 78 00           - cmp byte ptr [rcx+78],00 { 0 }
;dirtrally2.exe+CB0F1A - 48 8B D9              - mov rbx,rcx
;dirtrally2.exe+CB0F1D - 0F29 74 24 20         - movaps [rsp+20],xmm6		<<jmp here
;dirtrally2.exe+CB0F22 - 74 07                 - je dirtrally2.exe+CB0F2B
;dirtrally2.exe+CB0F24 - F2 0F10 71 50         - movsd xmm6,[rcx+50]
;dirtrally2.exe+CB0F29 - EB 08                 - jmp dirtrally2.exe+CB0F33
;dirtrally2.exe+CB0F2B - F2 0F10 35 8D325400   - movsd xmm6,[dirtrally2.exe+11F41C0] { (1.00) }
;dirtrally2.exe+CB0F33 - 8B 49 48              - mov ecx,[rcx+48]			
;dirtrally2.exe+CB0F36 - 83 E9 01              - sub ecx,01 { 1 }			<<return here
;dirtrally2.exe+CB0F39 - 0F84 B9000000         - je dirtrally2.exe+CB0FF8
;dirtrally2.exe+CB0F3F - 83 E9 01              - sub ecx,01 { 1 }
;dirtrally2.exe+CB0F42 - 74 79                 - je dirtrally2.exe+CB0FBD
;dirtrally2.exe+CB0F44 - 83 E9 01              - sub ecx,01 { 1 }
;dirtrally2.exe+CB0F47 - 74 6D                 - je dirtrally2.exe+CB0FB6
;	movaps [rsp+20h],xmm6
;	je firstjump
;	movsd xmm6,qword ptr [rcx+50h]
;	jmp exit
;firstjump:
;	movsd xmm6,qword ptr [g_timescaleValue]
;	mov ecx,[rcx+48h]
;exit:
;	jmp qword ptr [_timescaleInjectionContinue]
;timescaleInterceptor ENDP

timescaleInterceptor PROC
;dirtrally2.exe+4B399C - 48 8B 89 E81A0000     - mov rcx,[rcx+00001AE8]
;dirtrally2.exe+4B39A3 - E8 28154F00           - call dirtrally2.exe+9A4ED0
;dirtrally2.exe+4B39A8 - 48 8B 47 20           - mov rax,[rdi+20]
;dirtrally2.exe+4B39AC - 48 8B 88 281C0000     - mov rcx,[rax+00001C28]
;dirtrally2.exe+4B39B3 - 8B 81 48020000        - mov eax,[rcx+00000248]			<<jmp here
;dirtrally2.exe+4B39B9 - F3 0F10 89 90020000   - movss xmm1,[rcx+00000290]		<<read rcx
;dirtrally2.exe+4B39C1 - 83 F8 04              - cmp eax,04 { 4 }				<<return here
;dirtrally2.exe+4B39C4 - 75 0D                 - jne dirtrally2.exe+4B39D3
;dirtrally2.exe+4B39C6 - 48 8B 47 28           - mov rax,[rdi+28]
;dirtrally2.exe+4B39CA - C6 80 081B0000 00     - mov byte ptr [rax+00001B08],00 { 0 }
;dirtrally2.exe+4B39D1 - EB 31                 - jmp dirtrally2.exe+4B3A04
	mov eax,[rcx+00000248h]
	movss xmm1,dword ptr [rcx+00000290h]
	mov [g_timescaleAddress],rcx
exit:
	jmp qword ptr [_timescaleInjectionContinue]
timescaleInterceptor ENDP

dofInterceptor PROC
;dirtrally2.AK::MusicEngine::Term+9D0 - 0F2E DE               - ucomiss xmm3,xmm6
;dirtrally2.AK::MusicEngine::Term+9D3 - 74 05                 - je dirtrally2.AK::MusicEngine::Term+9DA
;dirtrally2.AK::MusicEngine::Term+9D5 - 0F57 C0               - xorps xmm0,xmm0
;dirtrally2.AK::MusicEngine::Term+9D8 - EB 07                 - jmp dirtrally2.AK::MusicEngine::Term+9E1
;dirtrally2.AK::MusicEngine::Term+9DA - 0F28 05 AF77B600      - movaps xmm0,[dirtrally2.exe+1228000] { (0.01) }
;dirtrally2.AK::MusicEngine::Term+9E1 - 0F29 81 70070000      - movaps [rcx+00000770],xmm0
;dirtrally2.AK::MusicEngine::Term+9E8 - F3 0F10 81 AC070000   - movss xmm0,[rcx+000007AC]	<<rcx+7AC has dof strength //inject here //it doesnt get written from my testing so can just overwrite it on toggle
;dirtrally2.AK::MusicEngine::Term+9F0 - 0FC6 C0 00            - shufps xmm0,xmm0,00 { 0 }
;dirtrally2.AK::MusicEngine::Term+9F4 - 0F59 81 70070000      - mulps xmm0,[rcx+00000770]
;dirtrally2.AK::MusicEngine::Term+9FB - 0F29 81 70070000      - movaps [rcx+00000770],xmm0	<<return  here
;dirtrally2.AK::MusicEngine::Term+A02 - 80 79 58 00           - cmp byte ptr [rcx+58],00 { 0 }
;dirtrally2.AK::MusicEngine::Term+A06 - 75 12                 - jne dirtrally2.AK::MusicEngine::Term+A1A
;dirtrally2.AK::MusicEngine::Term+A08 - F3 0F10 81 A8070000   - movss xmm0,[rcx+000007A8]
;dirtrally2.AK::MusicEngine::Term+A10 - 0F2F C6               - comiss xmm0,xmm6
;dirtrally2.AK::MusicEngine::Term+A13 - 72 05                 - jb dirtrally2.AK::MusicEngine::Term+A1A
	movss xmm0,dword ptr [rcx+000007ACh]
	mov [g_dofStrengthAddress],rcx
	shufps xmm0,xmm0,00h
	mulps xmm0,[rcx+00000770h]
	jmp qword ptr [_dofInjectionContinue]
dofInterceptor ENDP

END