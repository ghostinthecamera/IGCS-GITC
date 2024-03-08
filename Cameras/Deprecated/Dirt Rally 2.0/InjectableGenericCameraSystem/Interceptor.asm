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
PUBLIC cameraWrite1
PUBLIC cameraWrite2
PUBLIC cameraWrite3
PUBLIC cameraWrite4
PUBLIC cameraWrite5
PUBLIC cameraWrite6
PUBLIC cameraWrite7
PUBLIC cameraWrite8
PUBLIC cameraWrite9
PUBLIC cameraWrite18
PUBLIC cameraWrite19
PUBLIC cameraWrite20
PUBLIC cameraWrite21
PUBLIC cameraWrite22
PUBLIC cameraWrite23
PUBLIC cameraWrite24
PUBLIC cameraWrite25
PUBLIC fovWrite
PUBLIC fovWrite2
PUBLIC fovWrite3
PUBLIC nearplane
PUBLIC nearplane2

;---------------------------------------------------------------

;---------------------------------------------------------------
; Externs which are used and set by the system. Read / write these
; values in asm to communicate with the system
EXTERN g_cameraEnabled: byte
EXTERN g_cameraStructAddress: qword
EXTERN g_positionAddress: qword


;---------------------------------------------------------------

;---------------------------------------------------------------
; Own externs, defined in InterceptorHelper.cpp
EXTERN _cameraStructInterceptionContinue: qword
EXTERN _cameraWrite1Continue: qword
EXTERN _cameraWrite2Continue: qword
EXTERN _cameraWrite3Continue: qword
EXTERN _cameraWrite4Continue: qword
EXTERN _cameraWrite5Continue: qword
EXTERN _cameraWrite6Continue: qword
EXTERN _cameraWrite7Continue: qword
EXTERN _cameraWrite8Continue: qword
EXTERN _cameraWrite9Continue: qword
EXTERN _cameraWrite10Continue: qword
EXTERN _cameraWrite18Continue: qword
EXTERN _cameraWrite19Continue: qword
EXTERN _cameraWrite20Continue: qword
EXTERN _cameraWrite21Continue: qword
EXTERN _cameraWrite22Continue: qword
EXTERN _cameraWrite23Continue: qword
EXTERN _cameraWrite24Continue: qword
EXTERN _cameraWrite25Continue: qword
EXTERN _fovContinue: qword
EXTERN _fov2Continue: qword
EXTERN _fov3Continue: qword
EXTERN _fovabsoluteAddress: qword
EXTERN _NPabsoluteAddress: qword
EXTERN _nearplane1Continue: qword
EXTERN _nearplane2Continue: qword

.data

.code

nearplane PROC
;66 0F 6E C0 0F 5B C0 F3 0F 11 4B 78; 14 jump / F3 0F 59 05 CC 13 80 00 AOB for absolute
;dirtrally2.exe+A2C57F - 0FB7 44 24 20         - movzx eax,word ptr [rsp+20]
;dirtrally2.exe+A2C584 - 0F5B C0               - cvtdq2ps xmm0,xmm0
;dirtrally2.exe+A2C587 - 66 0F6E C8            - movd xmm1,eax
;dirtrally2.exe+A2C58B - F3 0F59 05 059F9700   - mulss xmm0,[dirtrally2.exe+13A6498] { (0.00) }
;dirtrally2.exe+A2C593 - FF25 00000000 2F4846BCFB7F0000 - jmp DirtRally2CameraTools.dll+7482F
;dirtrally2.exe+A2C5A1 - 0F59 0D 009F9700      - mulps xmm1,[dirtrally2.exe+13A64A8] { (0.00) }
;dirtrally2.exe+A2C5A8 - 66 0F6E C0            - movd xmm0,eax				<<<inject
;dirtrally2.exe+A2C5AC - 0F5B C0               - cvtdq2ps xmm0,xmm0
;dirtrally2.exe+A2C5AF - F3 0F11 4B 78         - movss [rbx+78],xmm1		<<Skip
;dirtrally2.exe+A2C5B4 - F3 0F59 05 CC138000   - mulss xmm0,[dirtrally2.exe+122D988] { (0.00) }
;dirtrally2.exe+A2C5BC - F3 0F11 83 AC000000   - movss [rbx+000000AC],xmm0	<<return
;dirtrally2.exe+A2C5C4 - 48 83 C4 40           - add rsp,40 { 64 }
	movd xmm0,eax
	cvtdq2ps xmm0,xmm0
	cmp [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraStructAddress],rbx
	je skip
noskip:
	movss dword ptr [rbx+78h],xmm1
skip:
	push rax
	mov rax, [_NPabsoluteAddress]
	mulss xmm0, dword ptr [rax]
	pop rax
	jmp qword ptr [_nearplane1Continue]
nearplane ENDP

nearplane2 PROC
;dirtrally2.exe+A39747 - F3 0F5C 4B 78         - subss xmm1,[rbx+78]
;dirtrally2.exe+A3974C - F3 0F59 CF            - mulss xmm1,xmm7
;dirtrally2.exe+A39750 - F3 0F58 4B 78         - addss xmm1,[rbx+78]
;dirtrally2.exe+A39755 - F3 0F11 4B 78         - movss [rbx+78],xmm1		<<INJECT/SKIP
;dirtrally2.exe+A3975A - F3 0F10 83 AC000000   - movss xmm0,[rbx+000000AC]
;dirtrally2.exe+A39762 - F3 0F10 4D DC         - movss xmm1,[rbp-24]
;dirtrally2.exe+A39767 - F3 0F5C C8            - subss xmm1,xmm0			<<RETURN
;dirtrally2.exe+A3976B - F3 0F59 CF            - mulss xmm1,xmm7
	cmp [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraStructAddress],rbx
	je skip
noskip:
	movss dword ptr [rbx+78h],xmm1
skip:
	movss xmm0,dword ptr [rbx+000000ACh]
	movss xmm1,dword ptr [rbp-24h]
	jmp qword ptr [_nearplane2Continue]
nearplane2 ENDP

fovWrite PROC
;dirtrally2.exe+A63094 - 48 8B CB              - mov rcx,rbx
;dirtrally2.exe+A63097 - F3 0F11 47 70         - movss [rdi+70],xmm0
;dirtrally2.exe+A6309C - 8B 43 18              - mov eax,[rbx+18]
;dirtrally2.exe+A6309F - 89 47 78              - mov [rdi+78],eax
;dirtrally2.exe+A630A2 - C6 87 20010000 00     - mov byte ptr [rdi+00000120],00 { 0 }
	mov rcx,rbx
	cmp [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraStructAddress],rdi
	je skip
noskip:
	movss dword ptr [rdi+70h],xmm0
skip:
	mov eax, dword ptr [rbx+18h]
	mov dword ptr [rdi+78h],eax
	jmp qword ptr [_fovContinue]
fovWrite ENDP

fovWrite2 PROC
;dirtrally2.exe+A2C587 - 66 0F6E C8            - movd xmm1,eax
;dirtrally2.exe+A2C58B - F3 0F59 05 059F9700   - mulss xmm0,[dirtrally2.exe+13A6498] { (0.00) }
;dirtrally2.exe+A2C593 - 0FB6 44 24 68         - movzx eax,byte ptr [rsp+68]			<<INJECT
;dirtrally2.exe+A2C598 - 0F5B C9               - cvtdq2ps xmm1,xmm1
;dirtrally2.exe+A2C59B - F3 0F11 43 70         - movss [rbx+70],xmm0
;dirtrally2.exe+A2C5A0 - F3 0F59 0D 009F9700   - mulss xmm1,[dirtrally2.exe+13A64A8] { (0.00) }
;dirtrally2.exe+A2C5A8 - 66 0F6E C0            - movd xmm0,eax	<<RETURN
;dirtrally2.exe+A2C5AC - 0F5B C0               - cvtdq2ps xmm0,xmm0
	movzx eax,byte ptr [rsp+68h]
	cvtdq2ps xmm1,xmm1
	cmp [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraStructAddress],rbx
	je skip
noskip:
	movss dword ptr [rbx+70h],xmm0
skip:
	push rax
	mov rax, [_fovabsoluteAddress]
	mulss xmm1, dword ptr [rax]
	pop rax
	jmp qword ptr [_fov2Continue]
fovWrite2 ENDP

fovWrite3 PROC
;dirtrally2.exe+A3972C - 0F29 03               - movaps [rbx],xmm0
;dirtrally2.exe+A3972F - F3 0F5C 4B 70         - subss xmm1,[rbx+70]
;dirtrally2.exe+A39734 - F3 0F59 CF            - mulss xmm1,xmm7
;dirtrally2.exe+A39738 - F3 0F58 4B 70         - addss xmm1,[rbx+70]
;dirtrally2.exe+A3973D - F3 0F11 4B 70         - movss [rbx+70],xmm1		<<<INJECT
;dirtrally2.exe+A39742 - F3 0F10 4D A8         - movss xmm1,[rbp-58]
;dirtrally2.exe+A39747 - F3 0F5C 4B 78         - subss xmm1,[rbx+78]
;dirtrally2.exe+A3974C - F3 0F59 CF            - mulss xmm1,xmm7			<<RETURN
;dirtrally2.exe+A39750 - F3 0F58 4B 78         - addss xmm1,[rbx+78]
;dirtrally2.exe+A39755 - F3 0F11 4B 78         - movss [rbx+78],xmm1
;dirtrally2.exe+A3975A - F3 0F10 83 AC000000   - movss xmm0,[rbx+000000AC]
;dirtrally2.exe+A39762 - F3 0F10 4D DC         - movss xmm1,[rbp-24]

;newcode
;dirtrally2.exe+A39716 - 0F58 73 50            - addps xmm6,[rbx+50]
;dirtrally2.exe+A3971A - 0F29 73 50            - movaps [rbx+50],xmm6
;dirtrally2.exe+A3971E - E8 6D36AFFF           - call dirtrally2.exe+52CD90
;dirtrally2.exe+A39723 - 0F28 45 00            - movaps xmm0,[rbp+00]
;dirtrally2.exe+A39727 - F3 0F10 4D A0         - movss xmm1,[rbp-60]
;dirtrally2.exe+A3972C - 0F29 03               - movaps [rbx],xmm0		<<INJECT
;dirtrally2.exe+A3972F - F3 0F5C 4B 70         - subss xmm1,[rbx+70]	
;dirtrally2.exe+A39734 - F3 0F59 CF            - mulss xmm1,xmm7
;dirtrally2.exe+A39738 - F3 0F58 4B 70         - addss xmm1,[rbx+70]
;dirtrally2.exe+A3973D - F3 0F11 4B 70         - movss [rbx+70],xmm1	<<SKIP
;dirtrally2.exe+A39742 - F3 0F10 4D A8         - movss xmm1,[rbp-58]	<<RETURN
;dirtrally2.exe+A39747 - F3 0F5C 4B 78         - subss xmm1,[rbx+78]
;dirtrally2.exe+A3974C - F3 0F59 CF            - mulss xmm1,xmm7
;dirtrally2.exe+A39750 - F3 0F58 4B 78         - addss xmm1,[rbx+78]
;dirtrally2.exe+A39755 - F3 0F11 4B 78         - movss [rbx+78],xmm1
;dirtrally2.exe+A3975A - F3 0F10 83 AC000000   - movss xmm0,[rbx+000000AC]
;dirtrally2.exe+A39762 - F3 0F10 4D DC         - movss xmm1,[rbp-24]	
	cmp [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraStructAddress],rbx
	je skip
noskip:
	movaps [rbx],xmm0
	subss xmm1,dword ptr [rbx+70h]
	mulss xmm1,xmm7
	addss xmm1,dword ptr [rbx+70h]
	movss dword ptr [rbx+70h],xmm1
skip:
	jmp qword ptr [_fov3Continue]
fovWrite3 ENDP



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
	push rdi
	lea rdi,[r12+130h]
	mov [g_cameraStructAddress],rdi
	pop rdi
	mov [r11-20h],r14
	mov r14,[rax+00000360h]
	jmp qword ptr [_cameraStructInterceptionContinue]
cameraStructInterceptor ENDP

cameraWrite1 PROC
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
	cmp [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraStructAddress],rdx
	je skip
noskip:
	movaps [rdx],xmm2
skip:
   jmp qword ptr [_cameraWrite1Continue]
cameraWrite1 ENDP

cameraWrite2 PROC
;0F 29 03 F3 0F 5C 4B 70
;dirtrally2.exe+A3971E - E8 6D36AFFF           - call dirtrally2.exe+52CD90
;dirtrally2.exe+A39723 - 0F28 45 00            - movaps xmm0,[rbp+00]
;dirtrally2.exe+A39727 - F3 0F10 4D A0         - movss xmm1,[rbp-60]
;dirtrally2.exe+A3972C - 0F29 03               - movaps [rbx],xmm0			<<INJECT/SKIP
;dirtrally2.exe+A3972F - F3 0F5C 4B 70         - subss xmm1,[rbx+70]
;dirtrally2.exe+A39734 - F3 0F59 CF            - mulss xmm1,xmm7
;dirtrally2.exe+A39738 - F3 0F58 4B 70         - addss xmm1,[rbx+70]
;dirtrally2.exe+A3973D - F3 0F11 4B 70         - movss [rbx+70],xmm1		<<RETURN
;dirtrally2.exe+A39742 - F3 0F10 4D A8         - movss xmm1,[rbp-58]
	cmp [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraStructAddress],rbx
	je skip
noskip:
	movaps [rbx],xmm0
skip:
	subss xmm1,dword ptr [rbx+70h]
	mulss xmm1,xmm7
	addss xmm1,dword ptr [rbx+70h]
   jmp qword ptr [_cameraWrite2Continue]
cameraWrite2 ENDP


cameraWrite3 PROC
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
	cmp [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraStructAddress],rsi
	je skip
noskip:
	movdqa [rsi],xmm0
skip:
   jmp qword ptr [_cameraWrite3Continue]
cameraWrite3 ENDP

cameraWrite4 PROC
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
	cmp [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraStructAddress],rsi
	je skip
	push rcx
	mov rcx,[g_cameraStructAddress]
	add rcx,10h
	cmp rsi,rcx
	pop rcx
	je skip
noskip:
	movaps [rsi],xmm3
skip:
   jmp qword ptr [_cameraWrite4Continue]
cameraWrite4 ENDP

cameraWrite5 PROC
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
	cmp [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraStructAddress],rbx
	je skip
noskip:
	movaps [rbx+10h],xmm0
skip:
	subps xmm6,[rbx+50h]
	mulps xmm6,xmm7
	addps xmm6,[rbx+50h]
   jmp qword ptr [_cameraWrite5Continue]
cameraWrite5 ENDP

cameraWrite6 PROC
;66 41 0F 7F 00 66 66 41 0F 7F 48 10
;dirtrally2.exe+A21E31 - 41 0F28 88 40010000   - movaps xmm1,[r8+00000140]
;dirtrally2.exe+A21E39 - 66 41 0F7F 48 10      - movdqa [r8+10],xmm1
;dirtrally2.exe+A21E3F - 41 0F28 80 70010000   - movaps xmm0,[r8+00000170]
;dirtrally2.exe+A21E47 - 41 0F28 88 80010000   - movaps xmm1,[r8+00000180]
;dirtrally2.exe+A21E4F - 66 41 0F7F 00         - movdqa [r8],xmm0						<<INJECT/SKIP
;dirtrally2.exe+A21E54 - 66 41 0F7F 48 10      - movdqa [r8+10],xmm1					<< SKIP
;dirtrally2.exe+A21E5A - 45 84 C9              - test r9b,r9b
;dirtrally2.exe+A21E5D - 75 0A                 - jne dirtrally2.exe+A21E69				<<RETURN
;dirtrally2.exe+A21E5F - 41 F6 80 10020000 04  - test byte ptr [r8+00000210],04 { 4 }	
	cmp [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraStructAddress],r8
	je skip
noskip:
	movdqa [r8],xmm0
	movdqa [r8+10h],xmm1
skip:
	test r9b,r9b
   jmp qword ptr [_cameraWrite6Continue]
cameraWrite6 ENDP

cameraWrite7 PROC
;0F 29 03 41 0F 58 CD 0F 29 5B 10 66 41 0F 6F F4
;dirtrally2.exe+A21A8C - 0F28 DA               - movaps xmm3,xmm2
;dirtrally2.exe+A21A8F - 0F55 08               - andnps xmm1,[rax]
;dirtrally2.exe+A21A92 - 0F28 FA               - movaps xmm7,xmm2
;dirtrally2.exe+A21A95 - 0F55 DE               - andnps xmm3,xmm6
;dirtrally2.exe+A21A98 - 0F29 03               - movaps [rbx],xmm0									<<INJECT/SKIP
;dirtrally2.exe+A21A9B - 41 0F58 CD            - addps xmm1,xmm13
;dirtrally2.exe+A21A9F - 0F29 5B 10            - movaps [rbx+10],xmm3								<<SKIP
;dirtrally2.exe+A21AA3 - 66 41 0F6F F4         - movdqa xmm6,xmm12	
;dirtrally2.exe+A21AA8 - 66 0F66 35 C0217D00   - pcmpgtd xmm6,[dirtrally2.exe+11F3C70] { (0) }		<<RETURN
;dirtrally2.exe+A21AB0 - 0F55 F9               - andnps xmm7,xmm1
;dirtrally2.exe+A21AB3 - E8 F8F50000           - call dirtrally2.exe+A310B0
;dirtrally2.exe+A21AB8 - 66 44 0F66 25 AF217D00  - pcmpgtd xmm12,[dirtrally2.exe+11F3C70] { (0) }
;dirtrally2.exe+A21AC1 - 0F55 F7               - andnps xmm6,xmm7
;dirtrally2.exe+A21AC4 - 44 0F28 B4 24 F0000000  - movaps xmm14,[rsp+000000F0]
	cmp [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraStructAddress],rbx
	je skip
noskip:
	movaps [rbx],xmm0
skip:
	addps xmm1,xmm13
	cmp [g_cameraEnabled], 1
	jne noskip2
	cmp [g_cameraStructAddress],rbx
	je skip2
noskip2:
	movaps [rbx+10h],xmm3
skip2:
	movdqa xmm6,xmm12
   jmp qword ptr [_cameraWrite7Continue]
cameraWrite7 ENDP

cameraWrite8 PROC
;0F 28 00 0F 28 CA 0F 58 47 10 0F 55 C8 0F 29 4F 10
;dirtrally2.exe+A21A8C - 0F28 DA               - movaps xmm3,xmm2
;dirtrally2.exe+A21A8F - 0F55 08               - andnps xmm1,[rax]
;dirtrally2.exe+A21A92 - 0F28 FA               - movaps xmm7,xmm2
;dirtrally2.exe+A21A95 - 0F55 DE               - andnps xmm3,xmm6
;dirtrally2.exe+A21A98 - 0F29 03               - movaps [rbx],xmm0									<<INJECT/SKIP
;dirtrally2.exe+A21A9B - 41 0F58 CD            - addps xmm1,xmm13
;dirtrally2.exe+A21A9F - 0F29 5B 10            - movaps [rbx+10],xmm3								<<SKIP
;dirtrally2.exe+A21AA3 - 66 41 0F6F F4         - movdqa xmm6,xmm12	
;dirtrally2.exe+A21AA8 - 66 0F66 35 C0217D00   - pcmpgtd xmm6,[dirtrally2.exe+11F3C70] { (0) }		<<RETURN
;dirtrally2.exe+A21AB0 - 0F55 F9               - andnps xmm7,xmm1
;dirtrally2.exe+A21AB3 - E8 F8F50000           - call dirtrally2.exe+A310B0
;dirtrally2.exe+A21AB8 - 66 44 0F66 25 AF217D00  - pcmpgtd xmm12,[dirtrally2.exe+11F3C70] { (0) }
;dirtrally2.exe+A21AC1 - 0F55 F7               - andnps xmm6,xmm7
;dirtrally2.exe+A21AC4 - 44 0F28 B4 24 F0000000  - movaps xmm14,[rsp+000000F0]
	movaps xmm1,xmm2
	addps xmm0,[rdi+10h]
	andnps xmm1,xmm0
	cmp [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraStructAddress],rdi
	je skip
noskip:
	movaps [rdi+10h],xmm1
skip:
   jmp qword ptr [_cameraWrite8Continue]
cameraWrite8 ENDP

cameraWrite9 PROC
;0F 56 C1 41 0F 28 D1 0F 29 46 10 41 0F 28 C9
;dirtrally2.exe+A20657 - 0F58 D0               - addps xmm2,xmm0
;dirtrally2.exe+A2065A - 41 0F28 C1            - movaps xmm0,xmm9
;dirtrally2.exe+A2065E - 0F54 C1               - andps xmm0,xmm1
;dirtrally2.exe+A20661 - 0F55 CA               - andnps xmm1,xmm2
;dirtrally2.exe+A20664 - 0F29 54 24 30         - movaps [rsp+30],xmm2		<<INJECT
;dirtrally2.exe+A20669 - 0F56 C1               - orps xmm0,xmm1
;dirtrally2.exe+A2066C - 41 0F28 D1            - movaps xmm2,xmm9
;dirtrally2.exe+A20670 - 0F29 46 10            - movaps [rsi+10],xmm0		<<SKIP
;dirtrally2.exe+A20674 - 41 0F28 C9            - movaps xmm1,xmm9			<<RETURN
	movaps [rsp+30h],xmm2
	orps xmm0,xmm1
	movaps xmm2,xmm9
	cmp [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraStructAddress],rsi
	je skip
noskip:
	movaps [rsi+10h],xmm0
skip:
    jmp qword ptr [_cameraWrite9Continue]
cameraWrite9 ENDP

cameraWrite10 PROC
;45 0F 28 53 D0 45 0F 28 5B C0 66 0F 7F 07
;dirtrally2.exe+A23A31 - 4C 8D 9C 24 C0000000  - lea r11,[rsp+000000C0]
;dirtrally2.exe+A23A39 - 49 8B 5B 10           - mov rbx,[r11+10]
;dirtrally2.exe+A23A3D - 41 0F28 73 F0         - movaps xmm6,[r11-10]
;dirtrally2.exe+A23A42 - 0F28 00               - movaps xmm0,[rax]
;dirtrally2.exe+A23A45 - 45 0F28 4B E0         - movaps xmm9,[r11-20]
;dirtrally2.exe+A23A4A - 45 0F28 53 D0         - movaps xmm10,[r11-30]		<<INJECT
;dirtrally2.exe+A23A4F - 45 0F28 5B C0         - movaps xmm11,[r11-40]
;dirtrally2.exe+A23A54 - 66 0F7F 07            - movdqa [rdi],xmm0			<<SKIP
;dirtrally2.exe+A23A58 - 49 8B E3              - mov rsp,r11				<<RETURN
;dirtrally2.exe+A23A5B - 5F                    - pop rdi
	movaps xmm10,[r11-30h]
	movaps xmm11,[r11-40h]
	cmp [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraStructAddress],rdi
	je skip
noskip:
	movdqa [rdi],xmm0
skip:
    jmp qword ptr [_cameraWrite10Continue]
cameraWrite10 ENDP

cameraWrite18 PROC
;dirtrally2.exe+A1F034 - 66 0F66 0D 344C7D00   - pcmpgtd xmm1,[dirtrally2.exe+11F3C70] { (0) }
;dirtrally2.exe+A1F03C - 0F28 D1               - movaps xmm2,xmm1
;dirtrally2.exe+A1F03F - 0F58 30               - addps xmm6,[rax]
;dirtrally2.exe+A1F042 - 0F29 07               - movaps [rdi],xmm0			<INJECT/SKIP
;dirtrally2.exe+A1F045 - F3 0F10 44 24 54      - movss xmm0,[rsp+54]
;dirtrally2.exe+A1F04B - 0F55 D6               - andnps xmm2,xmm6
;dirtrally2.exe+A1F04E - 0F29 57 10            - movaps [rdi+10],xmm2		<<SKIP
;dirtrally2.exe+A1F052 - 0FC6 D2 55            - shufps xmm2,xmm2,55 { 85 }	<<RETURN
;dirtrally2.exe+A1F056 - 0F2F D0               - comiss xmm2,xmm0
;dirtrally2.exe+A1F059 - 73 1B                 - jae dirtrally2.exe+A1F076
	cmp [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraStructAddress],rdi
	je skip
noskip:
	movaps [rdi],xmm0
skip:
	movss xmm0, dword ptr [rsp+54h]
	andnps xmm2,xmm6
	cmp [g_cameraEnabled], 1
	jne noskip2
	cmp [g_cameraStructAddress],rdi
	je skip2
noskip2:
	movaps [rdi+10h],xmm2
skip2:
	jmp qword ptr [_cameraWrite18Continue]
cameraWrite18 ENDP

cameraWrite19 PROC
;dirtrally2.exe+A1F220 - 0F29 44 24 40         - movaps [rsp+40],xmm0
;dirtrally2.exe+A1F225 - 48 8B D7              - mov rdx,rdi
;dirtrally2.exe+A1F228 - 48 8D 4D 80           - lea rcx,[rbp-80]
;dirtrally2.exe+A1F22C - E8 BFABF2FF           - call dirtrally2.exe+949DF0
;dirtrally2.exe+A1F231 - F3 0F10 0D 4FBF8100   - movss xmm1,[dirtrally2.exe+123B188] { (0.00) }
;dirtrally2.exe+A1F239 - 0F28 00               - movaps xmm0,[rax]				<<<INJECT
;dirtrally2.exe+A1F23C - 66 0F7F 07            - movdqa [rdi],xmm0				<<<SKIP
;dirtrally2.exe+A1F240 - 0F2F 8E 3C040000      - comiss xmm1,[rsi+0000043C]		
;dirtrally2.exe+A1F247 - 73 2A                 - jae dirtrally2.exe+A1F273		<<RETURN
;dirtrally2.exe+A1F249 - 44 0FB6 8E 28010000   - movzx r9d,byte ptr [rsi+00000128]
;dirtrally2.exe+A1F251 - 48 8D 87 60010000     - lea rax,[rdi+00000160]
	movaps xmm0,[rax]
	cmp [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraStructAddress],rdi
	je skip
noskip:
	movdqa [rdi],xmm0
skip:
	comiss xmm1, dword ptr [rsi+0000043Ch]
	jmp qword ptr [_cameraWrite19Continue]
cameraWrite19 ENDP

cameraWrite20 PROC
;dirtrally2.exe+A216B1 - 0F55 C8               - andnps xmm1,xmm0			<<INJECT
;dirtrally2.exe+A216B4 - 0F28 C3               - movaps xmm0,xmm3
;dirtrally2.exe+A216B7 - 0F58 4B 10            - addps xmm1,[rbx+10]
;dirtrally2.exe+A216BB - 0F55 C1               - andnps xmm0,xmm1
;dirtrally2.exe+A216BE - 0F29 43 10            - movaps [rbx+10],xmm0		<<SKIP	
;dirtrally2.exe+A216C2 - 48 83 C4 30           - add rsp,30 { 48 }			<<RETURN
;dirtrally2.exe+A216C6 - 5B                    - pop rbx
	andnps xmm1,xmm0
	movaps xmm0,xmm3
	addps xmm1,[rbx+10h]
	andnps xmm0,xmm1
	cmp [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraStructAddress],rbx
	je skip
noskip:
	movaps [rbx+10h],xmm0
skip:
	jmp qword ptr [_cameraWrite20Continue]
cameraWrite20 ENDP

cameraWrite21 PROC
;dirtrally2.exe+A63242 - 66 0F66 1D 260A7900   - pcmpgtd xmm3,[dirtrally2.exe+11F3C70] { (0) }
;dirtrally2.exe+A6324A - 48 8B D7              - mov rdx,rdi
;dirtrally2.exe+A6324D - 0F28 D3               - movaps xmm2,xmm3								<<INJECT
;dirtrally2.exe+A63250 - 0F55 D0               - andnps xmm2,xmm0
;dirtrally2.exe+A63253 - 0F29 57 10            - movaps [rdi+10],xmm2							<<SKIP
;dirtrally2.exe+A63257 - 0F28 86 D0010000      - movaps xmm0,[rsi+000001D0]						
;dirtrally2.exe+A6325E - 66 0F66 0D 0A0A7900   - pcmpgtd xmm1,[dirtrally2.exe+11F3C70] { (0) }  <<RETURN
;dirtrally2.exe+A63266 - 0F59 C6               - mulps xmm0,xmm6
	movaps xmm2,xmm3
	andnps xmm2,xmm0
	cmp [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraStructAddress],rdi
	je skip
noskip:
	movaps [rdi+10h],xmm2
skip:
	movaps xmm0,[rsi+000001D0h]
	jmp qword ptr [_cameraWrite21Continue]
cameraWrite21 ENDP

cameraWrite22 PROC
;dirtrally2.exe+A594D4 - 0F57 C0               - xorps xmm0,xmm0
;dirtrally2.exe+A594D7 - F3 0F58 4B 10         - addss xmm1,[rbx+10]
;dirtrally2.exe+A594DC - F3 0F58 E1            - addss xmm4,xmm1
;dirtrally2.exe+A594E0 - F3 0F10 C4            - movss xmm0,xmm4							<<INJECT
;dirtrally2.exe+A594E4 - F3 0F10 E8            - movss xmm5,xmm0
;dirtrally2.exe+A594E8 - 0FC6 ED E1            - shufps xmm5,xmm5,-1F { 225 }
;dirtrally2.exe+A594EC - 0F29 6E 10            - movaps [rsi+10],xmm5					<<SKIP
;dirtrally2.exe+A594F0 - 8B 4B 18              - mov ecx,[rbx+18]						<<RETURN
;dirtrally2.exe+A594F3 - B8 89888888           - mov eax,88888889 { -2004318071 }
;dirtrally2.exe+A594F8 - FF C1                 - inc ecx
	movss xmm0,xmm4
	movss xmm5,xmm0
	shufps xmm5,xmm5,-1Fh
	cmp [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraStructAddress],rsi
	je skip
noskip:
	movaps [rsi+10h],xmm5
skip:
	jmp qword ptr [_cameraWrite22Continue]
cameraWrite22 ENDP

cameraWrite23 PROC
;dirtrally2.exe+A632A1 - 48 8D 4C 24 50        - lea rcx,[rsp+50]
;dirtrally2.exe+A632A6 - 48 8B D7              - mov rdx,rdi
;dirtrally2.exe+A632A9 - 0F28 CF               - movaps xmm1,xmm7
;dirtrally2.exe+A632AC - 0F28 C7               - movaps xmm0,xmm7
;dirtrally2.exe+A632AF - 0F58 D6               - addps xmm2,xmm6
;dirtrally2.exe+A632B2 - 0F55 CA               - andnps xmm1,xmm2				<<INJECT
;dirtrally2.exe+A632B5 - 0F58 4F 10            - addps xmm1,[rdi+10]
;dirtrally2.exe+A632B9 - 0F55 C1               - andnps xmm0,xmm1
;dirtrally2.exe+A632BC - 0F29 47 10            - movaps [rdi+10],xmm0			<<SKIP
;dirtrally2.exe+A632C0 - E8 2B6BEEFF           - call dirtrally2.exe+949DF0		<<RETURN
;dirtrally2.exe+A632C5 - 44 0F28 84 24 90000000  - movaps xmm8,[rsp+00000090]
;dirtrally2.exe+A632CE - 0F28 BC 24 A0000000   - movaps xmm7,[rsp+000000A0]
;dirtrally2.exe+A632D6 - 0F28 00               - movaps xmm0,[rax]
	andnps xmm1,xmm2
	addps xmm1,[rdi+10h]
	andnps xmm0,xmm1
	cmp [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraStructAddress],rdi
	je skip
noskip:
	movaps [rdi+10h],xmm0
skip:
	jmp qword ptr [_cameraWrite23Continue]
cameraWrite23 ENDP

cameraWrite24 PROC
;dirtrally2.exe+A588D0 - 44 0F28 84 24 F0000000  - movaps xmm8,[rsp+000000F0]
;dirtrally2.exe+A588D9 - 0F28 BC 24 00010000   - movaps xmm7,[rsp+00000100]
;dirtrally2.exe+A588E1 - 0F28 B4 24 10010000   - movaps xmm6,[rsp+00000110]
;dirtrally2.exe+A588E9 - 0F28 00               - movaps xmm0,[rax]				<<INJECT
;dirtrally2.exe+A588EC - 48 8B BC 24 38010000  - mov rdi,[rsp+00000138]
;dirtrally2.exe+A588F4 - 66 41 0F7F 06         - movdqa [r14],xmm0				<<SKIP
;dirtrally2.exe+A588F9 - 48 8B 9C 24 30010000  - mov rbx,[rsp+00000130]			<<RETURN
;dirtrally2.exe+A58901 - 44 0F28 8C 24 E0000000  - movaps xmm9,[rsp+000000E0]
;dirtrally2.exe+A5890A - 4C 8D 9C 24 20010000  - lea r11,[rsp+00000120]
;dirtrally2.exe+A58912 - 49 8B 6B 20           - mov rbp,[r11+20]
;dirtrally2.exe+A58916 - 49 8B 73 28           - mov rsi,[r11+28]
;dirtrally2.exe+A5891A - 49 8B E3              - mov rsp,r11
;dirtrally2.exe+A5891D - 41 5E                 - pop r14
	movaps xmm0,[rax]
	mov rdi,[rsp+00000138h]
	cmp [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraStructAddress],r14
	je skip
noskip:
	movdqa [r14],xmm0
skip:
	jmp qword ptr [_cameraWrite24Continue]
cameraWrite24 ENDP

cameraWrite25 PROC
;dirtrally2.exe+A632BC - 0F29 47 10            - movaps [rdi+10],xmm0
;dirtrally2.exe+A632C0 - E8 2B6BEEFF           - call dirtrally2.exe+949DF0
;dirtrally2.exe+A632C5 - 44 0F28 84 24 90000000  - movaps xmm8,[rsp+00000090]
;dirtrally2.exe+A632CE - 0F28 BC 24 A0000000   - movaps xmm7,[rsp+000000A0]				<<INJECT
;dirtrally2.exe+A632D6 - 0F28 00               - movaps xmm0,[rax]
;dirtrally2.exe+A632D9 - 66 0F7F 07            - movdqa [rdi],xmm0						<<SKIP
;dirtrally2.exe+A632DD - EB 07                 - jmp dirtrally2.exe+A632E6				<<RETURN
;dirtrally2.exe+A632DF - 80 8F 10020000 02     - or byte ptr [rdi+00000210],02 { 2 }
	movaps xmm7,[rsp+000000A0h]
	movaps xmm0,[rax]
	cmp [g_cameraEnabled], 1
	jne noskip
	cmp [g_cameraStructAddress],rdi
	je skip
noskip:
	movdqa [rdi],xmm0
skip:
	jmp qword ptr [_cameraWrite25Continue]
cameraWrite25 ENDP


END