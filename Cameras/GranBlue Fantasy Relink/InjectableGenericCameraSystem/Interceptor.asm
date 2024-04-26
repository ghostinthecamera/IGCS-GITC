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
PUBLIC cameraWrite1Interceptor
PUBLIC cameraWrite2Interceptor
PUBLIC cameraWrite3Interceptor
PUBLIC resolutionInterceptor
PUBLIC aspectratioInterceptor

;---------------------------------------------------------------

;---------------------------------------------------------------
; Externs which are used and set by the system. Read / write these
; values in asm to communicate with the system
EXTERN g_cameraEnabled: byte
EXTERN g_cameraStructAddress: qword
EXTERN g_resolutionAddress: qword
EXTERN g_aspectratiobaseAddress: qword


;---------------------------------------------------------------

;---------------------------------------------------------------
; Own externs, defined in InterceptorHelper.cpp
EXTERN _cameraStructInterceptionContinue: qword
EXTERN _cameraWrite1InterceptionContinue: qword
EXTERN _cameraWrite2InterceptionContinue: qword
EXTERN _cameraWrite3InterceptionContinue: qword
EXTERN _resolutionInterceptionContinue: qword
EXTERN _aspectratioInterceptionContinue: qword


.data

.code

cameraStructInterceptor PROC
;granblue_fantasy_relink.exe+5B5C75 - C6 86 040D0000 01     - mov byte ptr [rsi+00000D04],01 { 1 }
;granblue_fantasy_relink.exe+5B5C7C - C6 86 0C0D0000 00     - mov byte ptr [rsi+00000D0C],00 { 0 }
;granblue_fantasy_relink.exe+5B5C83 - 8B 05 F7767305        - mov eax,[granblue_fantasy_relink.exe+5CED380] { (0) }
;granblue_fantasy_relink.exe+5B5C89 - 48 8D 3D 90B01304     - lea rdi,[granblue_fantasy_relink.exe+46F0D20] { (240) }
;granblue_fantasy_relink.exe+5B5C90 - 48 8B 04 C7           - mov rax,[rdi+rax*8]							<<INJECT/read rax
;granblue_fantasy_relink.exe+5B5C94 - 48 8B 40 60           - mov rax,[rax+60]
;granblue_fantasy_relink.exe+5B5C98 - 48 8B 80 48090000     - mov rax,[rax+00000948]
;granblue_fantasy_relink.exe+5B5C9F - 80 78 74 00           - cmp byte ptr [rax+74],00 { 0 }				<<return 0xF
;granblue_fantasy_relink.exe+5B5CA3 - 0F84 64090000         - je granblue_fantasy_relink.exe+5B660D
;granblue_fantasy_relink.exe+5B5CA9 - C4C34121C610          - vinsertps xmm0,xmm7,xmm14,10
;granblue_fantasy_relink.exe+5B5CAF - C4C11816CD            - vmovlhps xmm1,xmm12,xmm13
;granblue_fantasy_relink.exe+5B5CB4 - C4C11915D5            - vunpckhpd xmm2,xmm12,xmm13
;granblue_fantasy_relink.exe+5B5CB9 - C579D6DB              - vmovq xmm3,xmm0,xmm11
;granblue_fantasy_relink.exe+5B5CBD - C5D857E4              - vxorps xmm4,xmm4,xmm4
;granblue_fantasy_relink.exe+5B5CC1 - C5A115EC              - vunpckhpd xmm5,xmm11,xmm4
;granblue_fantasy_relink.exe+5B5CC5 - C5E8C6D5 88           - vshufps xmm2,xmm2,xmm5,-78 { 136 }
	mov rax,[rdi+rax*8h]
	mov [g_cameraStructAddress],rax
	mov rax,[rax+60h]
	mov rax,[rax+00000948h]
exit:
	jmp qword ptr [_cameraStructInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraStructInterceptor ENDP

cameraWrite1Interceptor PROC
;granblue_fantasy_relink.exe+5B01A5 - C7 86 AC010000 0000803F - mov [rsi+000001AC],3F800000 { 1.00 }
;granblue_fantasy_relink.exe+5B01AF - C5F82886 90 010000    - vmovaps xmm0,[rsi+00000190]
;granblue_fantasy_relink.exe+5B01B7 - C5F8588E 10 010000    - vaddps xmm1,xmm0,[rsi+00000110]
;granblue_fantasy_relink.exe+5B01BF - C5F85886 00 010000    - vaddps xmm0,xmm0,[rsi+00000100]
;granblue_fantasy_relink.exe+5B01C7 - C5F82946 10           - vmovaps [rsi+10],xmm0					<<INJECT/filter and skip
;granblue_fantasy_relink.exe+5B01CC - C5F8294E 20           - vmovaps [rsi+20],xmm1
;granblue_fantasy_relink.exe+5B01D1 - C5F82886 20 010000    - vmovaps xmm0,[rsi+00000120]
;granblue_fantasy_relink.exe+5B01D9 - C5F82946 30           - vmovaps [rsi+30],xmm0					
;granblue_fantasy_relink.exe+5B01DE - 48 89 F1              - mov rcx,rsi							<<RETURN
;granblue_fantasy_relink.exe+5B01E1 - E8 3A690B00           - call granblue_fantasy_relink.exe+666B20
;granblue_fantasy_relink.exe+5B01E6 - 48 8B 05 7BD29D05     - mov rax,[granblue_fantasy_relink.exe+5F8D468] { (3CE7FF6F000) }
;granblue_fantasy_relink.exe+5B01ED - 48 85 C0              - test rax,rax
;granblue_fantasy_relink.exe+5B01F0 - 74 09                 - je granblue_fantasy_relink.exe+5B01FB
	cmp byte ptr [g_cameraEnabled], 1
	jne originalCode
	cmp rsi,[g_cameraStructAddress]
	je skip
originalCode:
	vmovaps [rsi+10h],xmm0
	vmovaps [rsi+20h],xmm1
	vmovaps xmm0,[rsi+00000120h]
	vmovaps [rsi+30h],xmm0	
	jmp exit
skip:
    vmovaps xmm0,[rsi+00000120h]
exit:
	jmp qword ptr [_cameraWrite1InterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraWrite1Interceptor ENDP

cameraWrite2Interceptor PROC
;granblue_fantasy_relink.exe+867518 - C5F8284E 70           - vmovaps xmm1,[rsi+70]
;granblue_fantasy_relink.exe+86751D - C5F82896 80 000000    - vmovaps xmm2,[rsi+00000080]
;granblue_fantasy_relink.exe+867525 - C5F8289E 90 000000    - vmovaps xmm3,[rsi+00000090]
;granblue_fantasy_relink.exe+86752D - C5F8294E 10           - vmovaps [rsi+10],xmm1		<<INJECT
;granblue_fantasy_relink.exe+867532 - C5F82956 20           - vmovaps [rsi+20],xmm2		<<
;granblue_fantasy_relink.exe+867537 - C5F8295E 30           - vmovaps [rsi+30],xmm3		<<
;granblue_fantasy_relink.exe+86753C - C5FA100D 48 5B3803    - vmovss xmm1,[granblue_fantasy_relink.exe+3BED08C] { (0.00) }		<<RETURN
;granblue_fantasy_relink.exe+867544 - C5F2C2C8 02           - vcmpless xmm1,xmm1,xmm0 { 2 }
;granblue_fantasy_relink.exe+867549 - C5FAC215 6A 3BE70302  - vcmpless xmm2,xmm0,dqword ptr [granblue_fantasy_relink.exe+46DB0BC] { 2,(0.00) }
;granblue_fantasy_relink.exe+867552 - C5F054C8              - vandps xmm1,xmm1,xmm0
;granblue_fantasy_relink.exe+867556 - C4E3714AC020          - vblendvps xmm0,xmm1,xmm0,xmm2
;granblue_fantasy_relink.exe+86755C - C5FA1146 54           - vmovss [rsi+54],xmm0
	cmp byte ptr [g_cameraEnabled], 1
	jne originalCode
	cmp rsi,[g_cameraStructAddress]
	je exit
originalCode:
	vmovaps [rsi+10h],xmm1
	vmovaps [rsi+20h],xmm2
	vmovaps [rsi+30h],xmm3
exit:
	jmp qword ptr [_cameraWrite2InterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraWrite2Interceptor ENDP

cameraWrite3Interceptor PROC
;granblue_fantasy_relink.exe+85355F - C5F8298E 10 010000    - vmovaps [rsi+00000110],xmm1
;granblue_fantasy_relink.exe+853567 - C5F82896 60 010000    - vmovaps xmm2,[rsi+00000160]
;granblue_fantasy_relink.exe+85356F - C5F82996 20 010000    - vmovaps [rsi+00000120],xmm2
;granblue_fantasy_relink.exe+853577 - 48 8B 86 70010000     - mov rax,[rsi+00000170]
;granblue_fantasy_relink.exe+85357E - 48 89 86 30010000     - mov [rsi+00000130],rax
;granblue_fantasy_relink.exe+853585 - 8B 86 78010000        - mov eax,[rsi+00000178]
;granblue_fantasy_relink.exe+85358B - 89 86 38010000        - mov [rsi+00000138],eax
;granblue_fantasy_relink.exe+853591 - C5F8294E 20           - vmovaps [rsi+20],xmm1			<<INJECT
;granblue_fantasy_relink.exe+853596 - C5F82946 10           - vmovaps [rsi+10],xmm0			<<
;granblue_fantasy_relink.exe+85359B - C5F82956 30           - vmovaps [rsi+30],xmm2			<<
;granblue_fantasy_relink.exe+8535A0 - C5FA1086 74 010000    - vmovss xmm0,[rsi+00000174]	<<RETURN
;granblue_fantasy_relink.exe+8535A8 - C5FA1146 50           - vmovss [rsi+50],xmm0
;granblue_fantasy_relink.exe+8535AD - C5FA1086 78 010000    - vmovss xmm0,[rsi+00000178]
;granblue_fantasy_relink.exe+8535B5 - 48 8B 46 60           - mov rax,[rsi+60]
;granblue_fantasy_relink.exe+8535B9 - C5FA1180 D4 090000    - vmovss [rax+000009D4],xmm0
;granblue_fantasy_relink.exe+8535C1 - C6 80 DE090000 01     - mov byte ptr [rax+000009DE],01 { 1 }
	cmp byte ptr [g_cameraEnabled], 1
	jne originalCode
	cmp rsi,[g_cameraStructAddress]
	je exit
originalCode:
	vmovaps [rsi+20h],xmm1
	vmovaps [rsi+10h],xmm0
	vmovaps [rsi+30h],xmm2
exit:
	jmp qword ptr [_cameraWrite3InterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraWrite3Interceptor ENDP

resolutionInterceptor PROC
;granblue_fantasy_relink.exe+5C9FDE - 0F83 85000000         - jae granblue_fantasy_relink.exe+5CA069
;granblue_fantasy_relink.exe+5C9FE4 - 48 6B D0 4C           - imul rdx,rax,4C
;granblue_fantasy_relink.exe+5C9FE8 - 48 8D 0D D14E3805     - lea rcx,[granblue_fantasy_relink.exe+594EEC0] { (0) }
;granblue_fantasy_relink.exe+5C9FEF - 8B 44 0A 24           - mov eax,[rdx+rcx+24]			<<inject, lea
;granblue_fantasy_relink.exe+5C9FF3 - 44 8B 54 0A 28        - mov r10d,[rdx+rcx+28]
;granblue_fantasy_relink.exe+5C9FF8 - 44 8B 4C 0A 2C        - mov r9d,[rdx+rcx+2C]
;granblue_fantasy_relink.exe+5C9FFD - 44 8B 44 0A 30        - mov r8d,[rdx+rcx+30]			<<<return
;granblue_fantasy_relink.exe+5CA002 - 89 05 A84B3805        - mov [granblue_fantasy_relink.exe+594EBB0],eax { (1280) }
;granblue_fantasy_relink.exe+5CA008 - 44 89 15 A54B3805     - mov [granblue_fantasy_relink.exe+594EBB4],r10d { (720) }
;granblue_fantasy_relink.exe+5CA00F - 44 89 0D A24B3805     - mov [granblue_fantasy_relink.exe+594EBB8],r9d { (1280) }
;granblue_fantasy_relink.exe+5CA016 - 44 89 05 9F4B3805     - mov [granblue_fantasy_relink.exe+594EBBC],r8d { (720) }
;granblue_fantasy_relink.exe+5CA01D - 80 3D F00E7305 01     - cmp byte ptr [granblue_fantasy_relink.exe+5CFAF14],01 { (0),1 }
;granblue_fantasy_relink.exe+5CA024 - 75 3E                 - jne granblue_fantasy_relink.exe+5CA064
;granblue_fantasy_relink.exe+5CA026 - 3B 05 405F3705        - cmp eax,[granblue_fantasy_relink.exe+593FF6C] { (1280) }
;
;v1.1
;granblue_fantasy_relink.exe+5DD23E - 0F83 A4000000         - jae granblue_fantasy_relink.exe+5DD2E8
;granblue_fantasy_relink.exe+5DD244 - 48 6B C8 4C           - imul rcx,rax,4C
;granblue_fantasy_relink.exe+5DD248 - 4C 8D 0D 41B14105     - lea r9,[granblue_fantasy_relink.exe+59F8390] { (0) }
;granblue_fantasy_relink.exe+5DD24F - 42 8B 54 09 24        - mov edx,[rcx+r9+24]		<<inject, lea
;granblue_fantasy_relink.exe+5DD254 - 42 8B 44 09 28        - mov eax,[rcx+r9+28]
;granblue_fantasy_relink.exe+5DD259 - 46 8B 44 09 2C        - mov r8d,[rcx+r9+2C]
;granblue_fantasy_relink.exe+5DD25E - 42 8B 4C 09 30        - mov ecx,[rcx+r9+30]		<<return
;granblue_fantasy_relink.exe+5DD263 - 87 15 1FAE4105        - xchg [granblue_fantasy_relink.exe+59F8088],edx { (1280) }
;granblue_fantasy_relink.exe+5DD269 - 87 05 1DAE4105        - xchg [granblue_fantasy_relink.exe+59F808C],eax { (720) }
;granblue_fantasy_relink.exe+5DD26F - 44 87 05 1AAE4105     - xchg [granblue_fantasy_relink.exe+59F8090],r8d { (1280) }
;granblue_fantasy_relink.exe+5DD276 - 87 0D 18AE4105        - xchg [granblue_fantasy_relink.exe+59F8094],ecx { (720) }
;granblue_fantasy_relink.exe+5DD27C - 80 3D F1D87C05 01     - cmp byte ptr [granblue_fantasy_relink.exe+5DAAB74],01 { (0),1 }
;granblue_fantasy_relink.exe+5DD283 - 75 5E                 - jne granblue_fantasy_relink.exe+5DD2E3
;granblue_fantasy_relink.exe+5DD285 - 8B 0D E5D87C05        - mov ecx,[granblue_fantasy_relink.exe+5DAAB70] { (4) }
;granblue_fantasy_relink.exe+5DD28B - 48 83 F9 05           - cmp rcx,05 { 5 }

	push rdx
	lea rdx,[rcx+r9]
	mov [g_resolutionAddress],rdx
	pop rdx
	mov edx,[rcx+r9+24h]
	mov eax,[rcx+r9+28h]
	mov r8d,[rcx+r9+2Ch]
	jmp qword ptr [_resolutionInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
resolutionInterceptor ENDP

aspectratioInterceptor PROC
;granblue_fantasy_relink.exe+1A10FFD - 8B 05 7DC32D04        - mov eax,[granblue_fantasy_relink.exe+5CED380] { (0) }
;granblue_fantasy_relink.exe+1A11003 - 48 8D 0D 16FDCD02     - lea rcx,[granblue_fantasy_relink.exe+46F0D20] { (7FF6149E56F0) }
;granblue_fantasy_relink.exe+1A1100A - 48 8B 04 C1           - mov rax,[rcx+rax*8]
;granblue_fantasy_relink.exe+1A1100E - 48 8B 40 60           - mov rax,[rax+60]
;granblue_fantasy_relink.exe+1A11012 - C5C25908              - vmulss xmm1,xmm7,[rax]				<<INJECT
;granblue_fantasy_relink.exe+1A11016 - C4E279B94810          - vfmadd231ss xmm1,xmm0,[rax+10]
;granblue_fantasy_relink.exe+1A1101C - C4E249B94820          - vfmadd231ss xmm1,xmm6,[rax+20]
;granblue_fantasy_relink.exe+1A11022 - C5FA118E 88 000000    - vmovss [rsi+00000088],xmm1			<<RETURN
;granblue_fantasy_relink.exe+1A1102A - C5C25948 04           - vmulss xmm1,xmm7,[rax+04]
;granblue_fantasy_relink.exe+1A1102F - C4E279B94814          - vfmadd231ss xmm1,xmm0,[rax+14]
;granblue_fantasy_relink.exe+1A11035 - C4E249B94824          - vfmadd231ss xmm1,xmm6,[rax+24]
;granblue_fantasy_relink.exe+1A1103B - C5FA118E 8C 000000    - vmovss [rsi+0000008C],xmm1
;
;v1.2
;granblue_fantasy_relink.exe+1AAEB8D - 8B 05 3DC33804        - mov eax,[granblue_fantasy_relink.exe+5E3AED0] { (0) }
;granblue_fantasy_relink.exe+1AAEB93 - 48 8D 0D 46D6D302     - lea rcx,[granblue_fantasy_relink.exe+47EC1E0] { (7FF78C744810) }
;granblue_fantasy_relink.exe+1AAEB9A - 48 8B 04 C1           - mov rax,[rcx+rax*8]
;granblue_fantasy_relink.exe+1AAEB9E - 48 8B 40 60           - mov rax,[rax+60]
;granblue_fantasy_relink.exe+1AAEBA2 - C5C25908              - vmulss xmm1,xmm7,[rax]			<<INJECT
;granblue_fantasy_relink.exe+1AAEBA6 - C5FA5950 10           - vmulss xmm2,xmm0,[rax+10]
;granblue_fantasy_relink.exe+1AAEBAB - C5EA58C9              - vaddss xmm1,xmm2,xmm1
;granblue_fantasy_relink.exe+1AAEBAF - C5CA5950 20           - vmulss xmm2,xmm6,[rax+20]
;granblue_fantasy_relink.exe+1AAEBB4 - C5F258CA              - vaddss xmm1,xmm1,xmm2
;granblue_fantasy_relink.exe+1AAEBB8 - C5FA118E 88 000000    - vmovss [rsi+00000088],xmm1		<<RETURN
;granblue_fantasy_relink.exe+1AAEBC0 - C5C25948 04           - vmulss xmm1,xmm7,[rax+04]
;granblue_fantasy_relink.exe+1AAEBC5 - C5FA5950 14           - vmulss xmm2,xmm0,[rax+14]
;granblue_fantasy_relink.exe+1AAEBCA - C5EA58C9              - vaddss xmm1,xmm2,xmm1
;granblue_fantasy_relink.exe+1AAEBCE - C5CA5950 24           - vmulss xmm2,xmm6,[rax+24]
;granblue_fantasy_relink.exe+1AAEBD3 - C5F258CA              - vaddss xmm1,xmm1,xmm2
	mov [g_aspectratiobaseAddress],rax
	vmulss xmm1,xmm7,dword ptr [rax]
	vmulss xmm2,xmm0,dword ptr [rax+10h]
	vaddss xmm1,xmm2,xmm1
	vmulss xmm2,xmm6,dword ptr [rax+20h]
	vaddss xmm1,xmm1,xmm2
	jmp qword ptr [_aspectratioInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
aspectratioInterceptor ENDP

END