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
PUBLIC timestopInterceptor

;---------------------------------------------------------------

;---------------------------------------------------------------
; Externs which are used and set by the system. Read / write these
; values in asm to communicate with the system
EXTERN g_cameraEnabled: byte
EXTERN g_cameraStructAddress: qword
EXTERN g_timestopAddress: qword



;---------------------------------------------------------------

;---------------------------------------------------------------
; Own externs, defined in InterceptorHelper.cpp
EXTERN _cameraStructInterceptionContinue: qword
EXTERN _cameraWrite1InterceptionContinue: qword
EXTERN _timestopInterceptionContinue: qword


.data

.code

cameraStructInterceptor PROC
;v0.5
;armoredcore6.exe+151951D - 48 8B CE              - mov rcx,rsi
;armoredcore6.exe+1519520 - E8 2BEFFFFF           - call armoredcore6.exe+1518450
;armoredcore6.exe+1519525 - 48 8B 56 18           - mov rdx,[rsi+18]
;armoredcore6.exe+1519529 - 48 8B 4E 20           - mov rcx,[rsi+20]
;armoredcore6.exe+151952D - E8 FE8FDEFF           - call armoredcore6.exe+1302530
;armoredcore6.exe+1519532 - 48 8B 4E 28           - mov rcx,[rsi+28]	<<INJECT
;armoredcore6.exe+1519536 - 48 8B 56 18           - mov rdx,[rsi+18]
;armoredcore6.exe+151953A - 48 8B 01              - mov rax,[rcx]		<<RCX has cam
;armoredcore6.exe+151953D - FF 50 18              - call qword ptr [rax+18]	
;armoredcore6.exe+1519540 - 48 8D 4D F7           - lea rcx,[rbp-09]	<<<<RETURN
;armoredcore6.exe+1519544 - E8 5708EA00           - call armoredcore6.exe+23B9DA0
;armoredcore6.exe+1519549 - 48 8B 56 18           - mov rdx,[rsi+18]
;armoredcore6.exe+151954D - 4C 8D 45 F7           - lea r8,[rbp-09]
;armoredcore6.exe+1519551 - F3 0F10 05 4BDA5402   - movss xmm0,[armoredcore6.exe+3A66FA4] { (43.00) }
;armoredcore6.exe+1519559 - 48 8B 4E 10           - mov rcx,[rsi+10]
	;mov rcx,[rsi+28h]
	;mov rdx,[rsi+18h]
	;mov rax,[rcx]
	;mov [g_cameraStructAddress],rcx
	;call qword ptr [rax+18h]
;exit:
;new
;armoredcore6.exe+151A5F7 - 89 46 50              - mov [rsi+50],eax
;armoredcore6.exe+151A5FA - 48 8B 0D 77109103     - mov rcx,[armoredcore6.exe+4E2B678] { (7FF3FB643DB0) }
;armoredcore6.exe+151A601 - 48 85 C9              - test rcx,rcx
;armoredcore6.exe+151A604 - 74 2D                 - je armoredcore6.exe+151A633
;armoredcore6.exe+151A606 - 48 8B 46 28           - mov rax,[rsi+28]		<<inject
;armoredcore6.exe+151A60A - 48 8D 55 B7           - lea rdx,[rbp-49]
;armoredcore6.exe+151A60E - 0F28 40 10            - movaps xmm0,[rax+10]
;armoredcore6.exe+151A612 - 0F29 45 B7            - movaps [rbp-49],xmm0
;armoredcore6.exe+151A616 - 0F28 48 20            - movaps xmm1,[rax+20]		
;armoredcore6.exe+151A61A - 0F29 4D C7            - movaps [rbp-39],xmm1		<<return
;armoredcore6.exe+151A61E - 0F28 40 30            - movaps xmm0,[rax+30]
;armoredcore6.exe+151A622 - 0F29 45 D7            - movaps [rbp-29],xmm0
;armoredcore6.exe+151A626 - 0F28 48 40            - movaps xmm1,[rax+40]
;armoredcore6.exe+151A62A - 0F29 4D E7            - movaps [rbp-19],xmm1
;armoredcore6.exe+151A62E - E8 0D8C1000           - call armoredcore6.exe+1623240
;armoredcore6.exe+151A633 - 90                    - nop 
	mov rax,[rsi+28h]
	mov [g_cameraStructAddress],rax
	lea rdx,[rbp-49h]
	movaps xmm0,[rax+10h]
	movaps [rbp-49h],xmm0
	jmp qword ptr [_cameraStructInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraStructInterceptor ENDP

cameraWrite1Interceptor PROC
;armoredcore6.exe.text+6BEFC6 - E8 A5040300           - call armoredcore6.exe.text+6EF470
;armoredcore6.exe.text+6BEFCB - 48 8D 8B 04010000     - lea rcx,[rbx+00000104]
;armoredcore6.exe.text+6BEFD2 - E8 69040300           - call armoredcore6.exe.text+6EF440
;armoredcore6.exe.text+6BEFD7 - 0F28 C8               - movaps xmm1,xmm0
;armoredcore6.exe.text+6BEFDA - 48 8D 8F 04010000     - lea rcx,[rdi+00000104]
;armoredcore6.exe.text+6BEFE1 - E8 8A040300           - call armoredcore6.exe.text+6EF470
;armoredcore6.exe.text+6BEFE6 - 0F28 43 10            - movaps xmm0,[rbx+10]    <<inject here
;armoredcore6.exe.text+6BEFEA - 0F29 47 10            - movaps [rdi+10],xmm0	<<r1
;armoredcore6.exe.text+6BEFEE - 0F28 4B 20            - movaps xmm1,[rbx+20]		
;armoredcore6.exe.text+6BEFF2 - 0F29 4F 20            - movaps [rdi+20],xmm1 <<r2
;armoredcore6.exe.text+6BEFF6 - 0F28 43 30            - movaps xmm0,[rbx+30]
;armoredcore6.exe.text+6BEFFA - 0F29 47 30            - movaps [rdi+30],xmm0  <<r3
;armoredcore6.exe.text+6BEFFE - 0F28 4B 40            - movaps xmm1,[rbx+40]
;armoredcore6.exe.text+6BF002 - 48 8B 5C 24 30        - mov rbx,[rsp+30]
;armoredcore6.exe.text+6BF007 - 0F29 4F 40            - movaps [rdi+40],xmm1  <<position
;armoredcore6.exe.text+6BF00B - 48 83 C4 20           - add rsp,20 { 32 }	   <<return here
;armoredcore6.exe.text+6BF00F - 5F                    - pop rdi
;armoredcore6.exe.text+6BF010 - C3                    - ret  
	cmp byte ptr [g_cameraEnabled], 1
	jne originalCode
	cmp rdi,[g_cameraStructAddress]
	je exit
originalCode:
	movaps xmm0,[rbx+10h]
	movaps [rdi+10h],xmm0
	movaps xmm1,[rbx+20h]		
	movaps [rdi+20h],xmm1 
	movaps xmm0,[rbx+30h]
	movaps [rdi+30h],xmm0  
	movaps xmm1,[rbx+40h]
	mov rbx,[rsp+30h]
	movaps [rdi+40h],xmm1 
	jmp ogexit
exit:
    mov rbx,[rsp+30h]
ogexit:
	jmp qword ptr [_cameraWrite1InterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraWrite1Interceptor ENDP

timestopInterceptor PROC
;v0.5
;armoredcore6.exe+1677260 - E8 ABCB2A01           - call armoredcore6.exe+2923E10
;armoredcore6.exe+1677265 - 48 8B 05 ECAA8B03     - mov rax,[armoredcore6.exe+4F31D58] { (7FF46EAC29F0) }
;armoredcore6.exe+167726C - F3 0F10 88 74020000   - movss xmm1,[rax+00000274]									<<INJECT HERE, READ RAX, 274 is timescale offset
;armoredcore6.exe+1677274 - F3 0F59 88 68020000   - mulss xmm1,[rax+00000268]
;armoredcore6.exe+167727C - F3 0F59 88 88020000   - mulss xmm1,[rax+00000288]
;armoredcore6.exe+1677284 - 48 8D 1D CDF0BB02     - lea rbx,[armoredcore6.exe+4236358] { (140248540) }			<<RETURN HERE
;armoredcore6.exe+167728B - 48 89 5C 24 28        - mov [rsp+28],rbx
;armoredcore6.exe+1677290 - 0F57 C0               - xorps xmm0,xmm0
;armoredcore6.exe+1677293 - F3 0F11 44 24 30      - movss [rsp+30],xmm0
;armoredcore6.exe+1677299 - 48 8D 3D B0F0BB02     - lea rdi,[armoredcore6.exe+4236350] { (140248510) }
	;movss xmm1,dword ptr [rax+00000274h]
	;mov [g_timestopAddress],rax
	;mulss xmm1,dword ptr [rax+00000268h]
	;mulss xmm1,dword ptr [rax+00000288h]
;v0.6
;armoredcore6.exe+1679432 - 48 8D 0D F7600A02     - lea rcx,[armoredcore6.exe+371F530] { ("w:\fnr\source\library\fd4\dist_win64_vc2015\include\Core/Single") }
;armoredcore6.exe+1679439 - E8 52BE2A01           - call armoredcore6.exe+2925290
;armoredcore6.exe+167943E - 48 8B 05 13998B03     - mov rax,[armoredcore6.exe+4F32D58] { (7FF46EAC29F0) }
;armoredcore6.exe+1679445 - F3 0F10 88 74020000   - movss xmm1,[rax+00000274]		<INJECT/rax
;armoredcore6.exe+167944D - F3 0F59 88 68020000   - mulss xmm1,[rax+00000268]
;armoredcore6.exe+1679455 - F3 0F59 88 88020000   - mulss xmm1,[rax+00000288]		<<RETURN
;armoredcore6.exe+167945D - 48 8D 35 F4DEBB02     - lea rsi,[armoredcore6.exe+4237358] { (140248540) }
;armoredcore6.exe+1679464 - 48 89 74 24 30        - mov [rsp+30],rsi
;armoredcore6.exe+1679469 - 0F57 C0               - xorps xmm0,xmm0
	movss xmm1,dword ptr [rax+00000274h]
	mov [g_timestopAddress],rax
	mulss xmm1,dword ptr [rax+00000268h]
	mulss xmm1,dword ptr [rax+00000288h]
	jmp qword ptr [_timestopInterceptionContinue]
timestopInterceptor ENDP

END