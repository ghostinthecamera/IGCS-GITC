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
PUBLIC dofStruct
PUBLIC timescaleinterceptor
PUBLIC playerpointerinterceptor
PUBLIC entitytimeinterceptor
PUBLIC entityopacityinterceptor
;---------------------------------------------------------------

;---------------------------------------------------------------
; Externs which are used and set by the system. Read / write these
; values in asm to communicate with the system
EXTERN g_cameraEnabled: byte
EXTERN g_playersonly: byte
EXTERN g_hideplayer: byte
EXTERN g_hideNPC: byte
EXTERN g_cameraStructAddress: qword
EXTERN g_dofstructaddress: qword
EXTERN g_timescaleaddress: qword
EXTERN g_playerpointer: qword
EXTERN g_opacitypointer: qword
;---------------------------------------------------------------

;---------------------------------------------------------------
; Own externs, defined in InterceptorHelper.cpp
EXTERN _cameraStructInterceptionContinue: qword
EXTERN _dofInterceptionContinue: qword
EXTERN _timestructinterceptionContinue: qword
EXTERN _playerpointerContinue: qword
EXTERN _entitytimeContinue: qword
EXTERN _entityopacityContinue: qword

.data

.code

timevalue dd 0.0

cameraStructInterceptor PROC
;DarkSoulsIII.exe+E0073D - E8 CE96B8FF           - call DarkSoulsIII.exe+989E10
;DarkSoulsIII.exe+E00742 - 48 8B 5F 38           - mov rbx,[rdi+38]
;DarkSoulsIII.exe+E00746 - 48 8D 55 E7           - lea rdx,[rbp-19]
;DarkSoulsIII.exe+E0074A - 8B 48 50              - mov ecx,[rax+50]
;DarkSoulsIII.exe+E0074D - 89 4B 50              - mov [rbx+50],ecx
;DarkSoulsIII.exe+E00750 - 8B 48 54              - mov ecx,[rax+54]
;DarkSoulsIII.exe+E00753 - 89 4B 54              - mov [rbx+54],ecx
;DarkSoulsIII.exe+E00756 - 8B 48 58              - mov ecx,[rax+58]
;DarkSoulsIII.exe+E00759 - 89 4B 58              - mov [rbx+58],ecx
;DarkSoulsIII.exe+E0075C - 8B 48 5C              - mov ecx,[rax+5C]
;DarkSoulsIII.exe+E0075F - 89 4B 5C              - mov [rbx+5C],ecx
;DarkSoulsIII.exe+E00762 - 48 8B C8              - mov rcx,rax
;DarkSoulsIII.exe+E00765 - E8 868E60FF           - call DarkSoulsIII.exe+4095F0
;DarkSoulsIII.exe+E0076A - 0F28 00               - movaps xmm0,[rax]			<<inject here
;DarkSoulsIII.exe+E0076D - 66 0F7F 43 10         - movdqa [rbx+10],xmm0
;DarkSoulsIII.exe+E00772 - 0F28 48 10            - movaps xmm1,[rax+10]
;DarkSoulsIII.exe+E00776 - 66 0F7F 4B 20         - movdqa [rbx+20],xmm1
;DarkSoulsIII.exe+E0077B - 0F28 40 20            - movaps xmm0,[rax+20]
;DarkSoulsIII.exe+E0077F - 66 0F7F 43 30         - movdqa [rbx+30],xmm0
;DarkSoulsIII.exe+E00784 - 0F28 48 30            - movaps xmm1,[rax+30]
;DarkSoulsIII.exe+E00788 - 66 0F7F 4B 40         - movdqa [rbx+40],xmm1
;DarkSoulsIII.exe+E0078D - 48 8B 4F 28           - mov rcx,[rdi+28]				<<return here
;DarkSoulsIII.exe+E00791 - 48 8B 57 30           - mov rdx,[rdi+30]
	mov [g_cameraStructAddress],rbx
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
	;movdqa [rbx+10h],xmm0
	movaps xmm1,[rax+10h]
	;movdqa [rbx+20h],xmm1
	movaps xmm0,[rax+20h]
	;movdqa [rbx+30h],xmm0
	movaps xmm1,[rax+30h]
	;movdqa [rbx+40h],xmm1
exit:
	jmp qword ptr [_cameraStructInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraStructInterceptor ENDP

timescaleinterceptor PROC
;DarkSoulsIII.exe+EEDDF0 - F3 0F5E D8            - divss xmm3,xmm0
;DarkSoulsIII.exe+EEDDF4 - F3 0F11 5D 87         - movss [rbp-79],xmm3		<<<inject here
;DarkSoulsIII.exe+EEDDF9 - F3 0F11 5D 67         - movss [rbp+67],xmm3
;DarkSoulsIII.exe+EEDDFE - 48 89 54 24 28        - mov [rsp+28],rdx
;DarkSoulsIII.exe+EEDE03 - 4C 89 6C 24 28        - mov [rsp+28],r13			<<return here
;DarkSoulsIII.exe+EEDE08 - 48 8D 45 67           - lea rax,[rbp+67]
;DarkSoulsIII.exe+EEDE0C - 48 8D 4D 6F           - lea rcx,[rbp+6F]
;DarkSoulsIII.exe+EEDE10 - 44 0F2F EB            - comiss xmm13,xmm3		
;DarkSoulsIII.exe+EEDE14 - 48 0F46 C1            - cmovbe rax,rcx
;DarkSoulsIII.exe+EEDE18 - 8B 00                 - mov eax,[rax]
;DarkSoulsIII.exe+EEDE1A - 89 83 64020000        - mov [rbx+00000264],eax	
;DarkSoulsIII.exe+EEDE20 - 80 BB 78020000 00     - cmp byte ptr [rbx+00000278],00 { 0 }
;DarkSoulsIII.exe+EEDE27 - 74 09                 - je DarkSoulsIII.exe+EEDE32	
;DarkSoulsIII.exe+EEDE29 - 45 3B FE              - cmp r15d,r14d
;DarkSoulsIII.exe+EEDE2C - 41 0F93 C0            - setae r8b
;DarkSoulsIII.exe+EEDE30 - EB 07                 - jmp DarkSoulsIII.exe+EEDE39
;DarkSoulsIII.exe+EEDE32 - 45 85 F6              - test r14d,r14d
;DarkSoulsIII.exe+EEDE35 - 41 0F94 C0            - sete r8b
;DarkSoulsIII.exe+EEDE39 - F3 0F10 53 18         - movss xmm2,[rbx+18]
	mov [g_timescaleaddress], rbx
	movss dword ptr [rbp-79h],xmm3
	movss dword ptr [rbp+67h],xmm3
	mov [rsp+28h],rdx
exit:
	jmp qword ptr [_timestructinterceptionContinue]
timescaleinterceptor ENDP


playerpointerinterceptor PROC
;DarkSoulsIII.exe+9AB0D4 - 41 0FB6 F8            - movzx edi,r8l
;DarkSoulsIII.exe+9AB0D8 - 48 8B F2              - mov rsi,rdx
;DarkSoulsIII.exe+9AB0DB - 48 8B D9              - mov rbx,rcx
;DarkSoulsIII.exe+9AB0DE - 74 35                 - je DarkSoulsIII.exe+9AB115
;DarkSoulsIII.exe+9AB0E0 - 48 8B 49 08           - mov rcx,[rcx+08]
;DarkSoulsIII.exe+9AB0E4 - E8 A719EEFF           - call DarkSoulsIII.exe+88CA90
;DarkSoulsIII.exe+9AB0E9 - 84 C0                 - test al,al
;DarkSoulsIII.exe+9AB0EB - 74 28                 - je DarkSoulsIII.exe+9AB115
;DarkSoulsIII.exe+9AB0ED - 40 84 FF              - test dil,dil
;DarkSoulsIII.exe+9AB0F0 - 75 18                 - jne DarkSoulsIII.exe+9AB10A
;DarkSoulsIII.exe+9AB0F2 - 48 8B 43 08           - mov rax,[rbx+08]					<<Inject here
;DarkSoulsIII.exe+9AB0F6 - 48 8B 88 901F0000     - mov rcx,[rax+00001F90]
;DarkSoulsIII.exe+9AB0FD - 48 8B 49 28           - mov rcx,[rcx+28]
;DarkSoulsIII.exe+9AB101 - E8 FA72FFFF           - call DarkSoulsIII.exe+9A2400		<<return here
;DarkSoulsIII.exe+9AB106 - 84 C0                 - test al,al
;DarkSoulsIII.exe+9AB108 - 74 0B                 - je DarkSoulsIII.exe+9AB115
;DarkSoulsIII.exe+9AB10A - 48 8B D6              - mov rdx,rsi
;DarkSoulsIII.exe+9AB10D - 48 8B CB              - mov rcx,rbx
;DarkSoulsIII.exe+9AB110 - E8 EB060000           - call DarkSoulsIII.exe+9AB800
;DarkSoulsIII.exe+9AB115 - 48 8B 5C 24 30        - mov rbx,[rsp+30]
;DarkSoulsIII.exe+9AB11A - 48 8B 74 24 38        - mov rsi,[rsp+38]
;DarkSoulsIII.exe+9AB11F - 48 83 C4 20           - add rsp,20 { 32 }
;DarkSoulsIII.exe+9AB123 - 5F                    - pop rdi
	mov rax,[rbx+08h]
	mov rcx,[rax+00001F90h]
	mov rcx,[rcx+28h]
	mov [g_playerpointer],rcx
	mov [g_opacitypointer],rax
exit:
	jmp qword ptr [_playerpointerContinue]
playerpointerinterceptor ENDP

dofStruct PROC
;DarkSoulsIII.exe+EF6766 - 48 8B 42 08           - mov rax,[rdx+08]
;DarkSoulsIII.exe+EF676A - 48 89 41 08           - mov [rcx+08],rax
;DarkSoulsIII.exe+EF676E - 8B 42 10              - mov eax,[rdx+10]
;DarkSoulsIII.exe+EF6771 - 89 41 10              - mov [rcx+10],eax
;DarkSoulsIII.exe+EF6774 - 8B 42 14              - mov eax,[rdx+14]
;DarkSoulsIII.exe+EF6777 - 89 41 14              - mov [rcx+14],eax
;DarkSoulsIII.exe+EF677A - 8B 42 18              - mov eax,[rdx+18]
;DarkSoulsIII.exe+EF677D - 89 41 18              - mov [rcx+18],eax
;DarkSoulsIII.exe+EF6780 - 8B 42 1C              - mov eax,[rdx+1C]
;DarkSoulsIII.exe+EF6783 - 89 41 1C              - mov [rcx+1C],eax
;DarkSoulsIII.exe+EF6786 - 8B 42 20              - mov eax,[rdx+20]
;DarkSoulsIII.exe+EF6789 - 89 41 20              - mov [rcx+20],eax
;DarkSoulsIII.exe+EF678C - 8B 42 24              - mov eax,[rdx+24]
;DarkSoulsIII.exe+EF678F - 89 41 24              - mov [rcx+24],eax
;DarkSoulsIII.exe+EF6792 - 8B 42 28              - mov eax,[rdx+28]			<< inject/rdx has base, +x28 isdof, set to 0 to remove
;DarkSoulsIII.exe+EF6795 - 89 41 28              - mov [rcx+28],eax
;DarkSoulsIII.exe+EF6798 - 8B 42 2C              - mov eax,[rdx+2C]
;DarkSoulsIII.exe+EF679B - 89 41 2C              - mov [rcx+2C],eax
;DarkSoulsIII.exe+EF679E - 8B 42 30              - mov eax,[rdx+30]
;DarkSoulsIII.exe+EF67A1 - 89 41 30              - mov [rcx+30],eax
;DarkSoulsIII.exe+EF67A4 - 8B 42 34              - mov eax,[rdx+34]			<<return
;DarkSoulsIII.exe+EF67A7 - 89 41 34              - mov [rcx+34],eax
;DarkSoulsIII.exe+EF67AA - 8B 42 38              - mov eax,[rdx+38]
;DarkSoulsIII.exe+EF67AD - 89 41 38              - mov [rcx+38],eax
;DarkSoulsIII.exe+EF67B0 - 8B 42 3C              - mov eax,[rdx+3C]
;DarkSoulsIII.exe+EF67B3 - 89 41 3C              - mov [rcx+3C],eax
;DarkSoulsIII.exe+EF67B6 - 8B 42 40              - mov eax,[rdx+40]
;DarkSoulsIII.exe+EF67B9 - 89 41 40              - mov [rcx+40],eax
;DarkSoulsIII.exe+EF67BC - 8B 42 44              - mov eax,[rdx+44]
;DarkSoulsIII.exe+EF67BF - 89 41 44              - mov [rcx+44],eax
	mov eax,[rdx+28h]
	mov [g_dofstructaddress], rdx
	mov [rcx+28h],eax
	mov eax,[rdx+2Ch]
	mov [rcx+2Ch],eax
	mov eax,[rdx+30h]
exit:
	jmp qword ptr [_dofInterceptionContinue]
dofStruct ENDP


entitytimeinterceptor PROC
;DarkSoulsIII.exe+9A3D9A - E8 61350000           - call DarkSoulsIII.exe+9A7300
;DarkSoulsIII.exe+9A3D9F - 48 8B 43 08           - mov rax,[rbx+08]
;DarkSoulsIII.exe+9A3DA3 - 48 8B 88 901F0000     - mov rcx,[rax+00001F90]
;DarkSoulsIII.exe+9A3DAA - 48 8B 89 B8000000     - mov rcx,[rcx+000000B8]
;DarkSoulsIII.exe+9A3DB1 - 0F57 F6               - xorps xmm6,xmm6
;DarkSoulsIII.exe+9A3DB4 - 48 85 C9              - test rcx,rcx
;DarkSoulsIII.exe+9A3DB7 - 0F84 BF000000         - je DarkSoulsIII.exe+9A3E7C
;DarkSoulsIII.exe+9A3DBD - E8 9E9BFFFF           - call DarkSoulsIII.exe+99D960
;DarkSoulsIII.exe+9A3DC2 - 0F28 D0               - movaps xmm2,xmm0
;DarkSoulsIII.exe+9A3DC5 - F3 0F10 83 580A0000   - movss xmm0,[rbx+00000A58]		<<inject here/check player pointer against rbx
;DarkSoulsIII.exe+9A3DCD - F3 0F59 83 60090000   - mulss xmm0,[rbx+00000960]		<<we will use our own value to multiply against
;DarkSoulsIII.exe+9A3DD5 - F3 0F59 D0            - mulss xmm2,xmm0					<< return here
;DarkSoulsIII.exe+9A3DD9 - 80 BB 610A0000 00     - cmp byte ptr [rbx+00000A61],00 { 0 }
;DarkSoulsIII.exe+9A3DE0 - 74 03                 - je DarkSoulsIII.exe+9A3DE5
;DarkSoulsIII.exe+9A3DE2 - 0F28 D6               - movaps xmm2,xmm6
;DarkSoulsIII.exe+9A3DE5 - 80 BB 670A0000 00     - cmp byte ptr [rbx+00000A67],00 { 0 }
;DarkSoulsIII.exe+9A3DEC - 74 37                 - je DarkSoulsIII.exe+9A3E25
;DarkSoulsIII.exe+9A3DEE - 48 8D 8B 30090000     - lea rcx,[rbx+00000930]
;DarkSoulsIII.exe+9A3DF5 - 48 8D 54 24 20        - lea rdx,[rsp+20]
;DarkSoulsIII.exe+9A3DFA - E8 81879AFF           - call DarkSoulsIII.exe+34C580
;DarkSoulsIII.exe+9A3DFF - 90                    - nop 
	cmp byte ptr [g_playersonly],0
	je originalcode
	cmp rbx,qword ptr [g_playerpointer]
	je originalcode
	movss xmm0,dword ptr [timevalue]
	mulss xmm0,dword ptr [rbx+00000960h]
	jmp exit
originalcode:
	movss xmm0,dword ptr [rbx+00000A58h]
	mulss xmm0,dword ptr [rbx+00000960h]
exit:
	jmp qword ptr [_entitytimeContinue]
entitytimeinterceptor ENDP


entityopacityinterceptor PROC
;DarkSoulsIII.exe+5B14B1 - 48 83 C4 30           - add rsp,30 { 48 }
;DarkSoulsIII.exe+5B14B5 - 5F                    - pop rdi
;DarkSoulsIII.exe+5B14B6 - C3                    - ret 
;DarkSoulsIII.exe+5B14B7 - 83 45 60 01           - add dword ptr [rbp+60],01 { 1 }
;DarkSoulsIII.exe+5B14BB - E9 0E44C504           - jmp DarkSoulsIII.exe+52058CE
;DarkSoulsIII.exe+5B14C0 - 40 53                 - push rbx
;DarkSoulsIII.exe+5B14C2 - 48 83 EC 20           - sub rsp,20 { 32 }
;DarkSoulsIII.exe+5B14C6 - 48 8B 01              - mov rax,[rcx]
;DarkSoulsIII.exe+5B14C9 - 48 8B D9              - mov rbx,rcx
;DarkSoulsIII.exe+5B14CC - FF 90 A0030000        - call qword ptr [rax+000003A0]
;DarkSoulsIII.exe+5B14D2 - F3 0F10 8B 441A0000   - movss xmm1,[rbx+00001A44]
;DarkSoulsIII.exe+5B14DA - F3 0F59 8B 401A0000   - mulss xmm1,[rbx+00001A40]
;DarkSoulsIII.exe+5B14E2 - F3 0F59 8B 4C1A0000   - mulss xmm1,[rbx+00001A4C]
;DarkSoulsIII.exe+5B14EA - F3 0F59 8B 501A0000   - mulss xmm1,[rbx+00001A50]
;DarkSoulsIII.exe+5B14F2 - F3 0F59 C1            - mulss xmm0,xmm1
;DarkSoulsIII.exe+5B14F6 - 48 83 C4 20           - add rsp,20 { 32 }
;DarkSoulsIII.exe+5B14FA - 5B                    - pop rbx
;DarkSoulsIII.exe+5B14FB - C3                    - ret 
;DarkSoulsIII.exe+5B14FC - 90                    - nop 
;DarkSoulsIII.exe+5B14FD - 48 8B C4              - mov rax,rsp
;DarkSoulsIII.exe+5B1500 - 32 C0                 - xor al,al
	cmp rbx, qword ptr [g_opacitypointer]
	je playerhidecode
	jne npchidecode
	;jmp originalcode
playerhidecode:
	cmp byte ptr [g_hideplayer],0
	je originalcode
	movss xmm1,dword ptr [timevalue]
	mulss xmm1,dword ptr [rbx+00001A40h]
	jmp exit
npchidecode:
	cmp byte ptr [g_hideNPC],0
	je originalcode
	movss xmm1,dword ptr [timevalue]
	mulss xmm1,dword ptr [rbx+00001A40h]
	jmp exit
originalcode:
	movss xmm1,dword ptr [rbx+00001A44h]
	mulss xmm1,dword ptr [rbx+00001A40h]
exit:
	jmp qword ptr [_entityopacityContinue]
entityopacityinterceptor ENDP



END