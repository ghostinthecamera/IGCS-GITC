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
PUBLIC cameraWrite3Interceptor
PUBLIC cameraWrite4Interceptor
PUBLIC cameraWrite5Interceptor
;PUBLIC timestopReadInterceptor
;PUBLIC resolutionScaleReadInterceptor
;PUBLIC displayTypeInterceptor
;PUBLIC dofSelectorWriteInterceptor

;---------------------------------------------------------------

;---------------------------------------------------------------
; Externs which are used and set by the system. Read / write these
; values in asm to communicate with the system
EXTERN g_cameraEnabled: byte
EXTERN g_cameraStructAddress: qword
;EXTERN g_resolutionScaleAddress: qword
;EXTERN g_timestopStructAddress: qword
;EXTERN g_displayTypeStructAddress: qword
;EXTERN g_dofStructAddress: qword

;---------------------------------------------------------------

;---------------------------------------------------------------
; Own externs, defined in InterceptorHelper.cpp
EXTERN _cameraStructInterceptionContinue: qword
EXTERN _cameraWrite1InterceptionContinue: qword
EXTERN _cameraWrite2InterceptionContinue: qword
EXTERN _cameraWrite3InterceptionContinue: qword
EXTERN _cameraWrite4InterceptionContinue: qword
EXTERN _cameraWrite5InterceptionContinue: qword
;EXTERN _timestopReadInterceptionContinue: qword
;EXTERN _resolutionScaleReadInterceptionContinue: qword
;EXTERN _displayTypeInterceptionContinue: qword
;EXTERN _dofSelectorWriteInterceptionContinue: qword

.data

.code

cameraStructInterceptor PROC
; camera address interceptor is also a the FoV write blocker.
;dirtrally2.exe+9F1FA5 - 57                    - push rdi
;dirtrally2.exe+9F1FA6 - 48 83 EC 50           - sub rsp,50 { 80 }
;dirtrally2.exe+9F1FAA - 48 8B 01              - mov rax,[rcx]
;dirtrally2.exe+9F1FAD - 48 8B DA              - mov rbx,rdx
;dirtrally2.exe+9F1FB0 - FF 90 C0000000        - call qword ptr [rax+000000C0]  <<inject here
;dirtrally2.exe+9F1FB6 - 48 8B D0              - mov rdx,rax
;dirtrally2.exe+9F1FB9 - 48 8D 4C 24 20        - lea rcx,[rsp+20]
;dirtrally2.exe+9F1FBE - 48 8B F8              - mov rdi,rax
;dirtrally2.exe+9F1FC1 - E8 AA5E6BFF           - call dirtrally2.exe+A7E70
;dirtrally2.exe+9F1FC6 - 0F28 00               - movaps xmm0,[rax]
;dirtrally2.exe+9F1FC9 - 66 0F7F 43 20         - movdqa [rbx+20],xmm0
	call qword ptr [rax+000000C0h]
	mov [g_cameraStructAddress], rax
	mov rdx,rax
	lea rcx,[rsp+20h]
	jmp qword ptr [_cameraStructInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraStructInterceptor ENDP

cameraWrite1Interceptor PROC
;AOB: 41 0F 28 88 80 01 00 00 66 41 0F 7F 00 66 41 0F 7F 48 10
;dirtrally2.exe+9EB7B7 - 74 26                 - je dirtrally2.exe+9EB7DF
;dirtrally2.exe+9EB7B9 - 41 8B 80 54010000     - mov eax,[r8+00000154]
;dirtrally2.exe+9EB7C0 - 41 89 40 70           - mov [r8+70],eax
;dirtrally2.exe+9EB7C4 - 41 0F28 80 30010000   - movaps xmm0,[r8+00000130]
;dirtrally2.exe+9EB7CC - 66 41 0F7F 00         - movdqa [r8],xmm0
;dirtrally2.exe+9EB7D1 - 41 0F28 88 40010000   - movaps xmm1,[r8+00000140]
;dirtrally2.exe+9EB7D9 - 66 41 0F7F 48 10      - movdqa [r8+10],xmm1
;dirtrally2.exe+9EB7DF - 41 0F28 80 70010000   - movaps xmm0,[r8+00000170]
;dirtrally2.exe+9EB7E7 - 41 0F28 88 80010000   - movaps xmm1,[r8+00000180]	<<< inject here
;dirtrally2.exe+9EB7EF - 66 41 0F7F 00         - movdqa [r8],xmm0
;dirtrally2.exe+9EB7F4 - 66 41 0F7F 48 10      - movdqa [r8+10],xmm1		<< chase cam write 1
;dirtrally2.exe+9EB7FA - 45 84 C9              - test r9l,r9l				<< return here
;dirtrally2.exe+9EB7FD - 75 0A                 - jne dirtrally2.exe+9EB809
;dirtrally2.exe+9EB7FF - 41 F6 80 10020000 04  - test byte ptr [r8+00000210],04 { 00000004 }
;dirtrally2.exe+9EB807 - 74 09                 - je dirtrally2.exe+9EB812
;dirtrally2.exe+9EB809 - 33 C0                 - xor eax,eax
;dirtrally2.exe+9EB80B - 49 89 80 D8010000     - mov [r8+000001D8],rax
;dirtrally2.exe+9EB812 - 48 8B B9 B0000000     - mov rdi,[rcx+000000B0]
	movaps xmm1,[r8+00000180h]
	movdqa [r8],xmm0
	cmp [g_cameraEnabled], 1
	je exit
	movdqa [r8+10h],xmm1
exit:
	jmp qword ptr [_cameraWrite1InterceptionContinue] 
cameraWrite1Interceptor ENDP


cameraWrite2Interceptor PROC
;AOB: 0F 29 5C 24 40 0F 56 D0 41 0F 28 C1 0F 29 56 10
;dirtrally2.exe+9E9FE7 - 0F28 5C 24 40         - movaps xmm3,[rsp+40]
;dirtrally2.exe+9E9FEC - 0F28 D1               - movaps xmm2,xmm1
;dirtrally2.exe+9E9FEF - 0FC6 C0 00            - shufps xmm0,xmm0,00 { 0 }
;dirtrally2.exe+9E9FF3 - 0F59 44 24 30         - mulps xmm0,[rsp+30]
;dirtrally2.exe+9E9FF8 - 0F58 D8               - addps xmm3,xmm0
;dirtrally2.exe+9E9FFB - 41 0F28 C1            - movaps xmm0,xmm9
;dirtrally2.exe+9E9FFF - 0F54 C1               - andps xmm0,xmm1
;dirtrally2.exe+9EA002 - 0F55 D3               - andnps xmm2,xmm3
;dirtrally2.exe+9EA005 - 0F29 5C 24 40         - movaps [rsp+40],xmm3		<<inject here
;dirtrally2.exe+9EA00A - 0F56 D0               - orps xmm2,xmm0
;dirtrally2.exe+9EA00D - 41 0F28 C1            - movaps xmm0,xmm9	
;dirtrally2.exe+9EA011 - 0F29 56 10            - movaps [rsi+10],xmm2		<<chase cam write 2
;dirtrally2.exe+9EA015 - 66 44 0F66 1D B2FC5D00  - pcmpgtd xmm11,[dirtrally2.exe+FC9CD0] { (0) }  <<return here
;dirtrally2.exe+9EA01E - 41 0F54 C3            - andps xmm0,xmm11
;dirtrally2.exe+9EA022 - 41 0F28 D3            - movaps xmm2,xmm11
;dirtrally2.exe+9EA026 - 0F55 54 24 60         - andnps xmm2,[rsp+60]
;dirtrally2.exe+9EA02B - 41 0F28 CB            - movaps xmm1,xmm11
;dirtrally2.exe+9EA02F - 0F55 4C 24 40         - andnps xmm1,[rsp+40]
;dirtrally2.exe+9EA034 - 0F56 D0               - orps xmm2,xmm0
	movaps [rsp+40h],xmm3
	orps xmm2,xmm0
	movaps xmm0,xmm9
	cmp [g_cameraEnabled], 1
	je exit
	movaps [rsi+10h],xmm2
exit:
	jmp qword ptr [_cameraWrite2InterceptionContinue]
cameraWrite2Interceptor ENDP

cameraWrite3Interceptor PROC
;AOB: 45 0A 88 D0 00 00 00 66 41 0F 7F 40 10 66 41 0F 7F 08
;dirtrally2.exe+9EB99E - 41 80 B8 00010000 00  - cmp byte ptr [r8+00000100],00 { 0 }
;dirtrally2.exe+9EB9A6 - 74 26                 - je dirtrally2.exe+9EB9CE
;dirtrally2.exe+9EB9A8 - 41 8B 80 04010000     - mov eax,[r8+00000104]
;dirtrally2.exe+9EB9AF - 41 89 40 70           - mov [r8+70],eax
;dirtrally2.exe+9EB9B3 - 41 0F28 80 E0000000   - movaps xmm0,[r8+000000E0]
;dirtrally2.exe+9EB9BB - 66 41 0F7F 00         - movdqa [r8],xmm0
;dirtrally2.exe+9EB9C0 - 41 0F28 88 F0000000   - movaps xmm1,[r8+000000F0]
;dirtrally2.exe+9EB9C8 - 66 41 0F7F 48 10      - movdqa [r8+10],xmm1
;dirtrally2.exe+9EB9CE - 41 0F28 80 50010000   - movaps xmm0,[r8+00000150]
;dirtrally2.exe+9EB9D6 - 48 81 C1 90070000     - add rcx,00000790 { 1936 }
;dirtrally2.exe+9EB9DD - 41 0F28 88 40010000   - movaps xmm1,[r8+00000140]
;dirtrally2.exe+9EB9E5 - 49 8B D2              - mov rdx,r10
;dirtrally2.exe+9EB9E8 - 45 0FB6 88 00010000   - movzx r9d,byte ptr [r8+00000100]
;dirtrally2.exe+9EB9F0 - 45 0A 88 D0 00 00 00  - or r9l,[r8+000000D0]			<<< inject here
;dirtrally2.exe+9EB9F7 - 66 41 0F7F 40 10      - movdqa [r8+10],xmm0				<<<bonnet/bumper cam write
;dirtrally2.exe+9EB9FD - 66 41 0F7F 08         - movdqa [r8],xmm1
;dirtrally2.exe+9EBA02 - 49 81 C0 18010000     - add r8,00000118 { 280 }			<< return for this op
;dirtrally2.exe+9EBA09 - 48 83 C4 58           - add rsp,58 { 88 }
;dirtrally2.exe+9EBA0D - E9 3EFBFFFF           - jmp dirtrally2.exe+9EB550
	or r9d,[r8+000000D0h]
	cmp [g_cameraEnabled], 1
	je writeskip
	movdqa [r8+10h],xmm0
writeskip:
	movdqa [r8],xmm1
	jmp qword ptr [_cameraWrite3InterceptionContinue]
cameraWrite3Interceptor ENDP

cameraWrite4Interceptor PROC
;AOB: 66 41 0F 7F 40 10 66 41 0F 7F 80 70 01 00 00
;dirtrally2.exe+9EB8B3 - 48 8B CB              - mov rcx,rbx
;dirtrally2.exe+9EB8B6 - 41 0F28 88 C0000000   - movaps xmm1,[r8+000000C0]
;dirtrally2.exe+9EB8BE - 66 41 0F7F 00         - movdqa [r8],xmm0
;dirtrally2.exe+9EB8C3 - 41 0F28 80 E0000000   - movaps xmm0,[r8+000000E0]
;dirtrally2.exe+9EB8CB - 66 41 0F7F 40 10      - movdqa [r8+10],xmm0				<<inject here/interior cam write 1
;dirtrally2.exe+9EB8D1 - 66 41 0F7F 80 70010000  - movdqa [r8+00000170],xmm0		<< return here
;dirtrally2.exe+9EB8DA - 66 41 0F7F 88 80010000  - movdqa [r8+00000180],xmm1
	cmp [g_cameraEnabled], 1
	je writeskip
	movdqa [r8+10h],xmm0
writeskip:
	movdqa [r8+00000170h],xmm0
	jmp qword ptr [_cameraWrite4InterceptionContinue]
cameraWrite4Interceptor ENDP

cameraWrite5Interceptor PROC
;AOB: 66 0F 7F 03 66 0F 7F 4B 10 48 8B 5C 24 60
;dirtrally2.exe+9EB901 - 0F28 83 80010000      - movaps xmm0,[rbx+00000180]
;dirtrally2.exe+9EB908 - 0F28 8B 70010000      - movaps xmm1,[rbx+00000170]
;dirtrally2.exe+9EB90F - 66 0F7F 03            - movdqa [rbx],xmm0					<<<inject here/interior cam write 2
;dirtrally2.exe+9EB913 - 66 0F7F 4B 10         - movdqa [rbx+10],xmm1
;dirtrally2.exe+9EB918 - 48 8B 5C 24 60        - mov rbx,[rsp+60]					<<<return here
;dirtrally2.exe+9EB91D - 48 83 C4 50           - add rsp,50 { 80 }
;dirtrally2.exe+9EB921 - 5F                    - pop rdi
	movdqa [rbx],xmm0
	cmp [g_cameraEnabled], 1
	je writeskip
	movdqa [rbx+10h],xmm1
writeskip:
	mov rbx,[rsp+60h]
	jmp qword ptr [_cameraWrite5InterceptionContinue]
cameraWrite5Interceptor ENDP

END