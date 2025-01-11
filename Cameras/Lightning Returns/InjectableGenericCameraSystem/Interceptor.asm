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
.model flat,C
.stack 4096
;---------------------------------------------------------------
; Public definitions so the linker knows which names are present in this file
PUBLIC cameraStructInterceptor
PUBLIC cameraWrite1Interceptor
PUBLIC timestopInterceptor
PUBLIC resolutionInterceptor
PUBLIC aspectratioInterceptor
PUBLIC battleARInterceptor
PUBLIC cutsceneARInterceptor

;---------------------------------------------------------------

;---------------------------------------------------------------
; Externs which are used and set by the system. Read / write these
; values in asm to communicate with the system
EXTERN g_cameraEnabled: byte
EXTERN g_cameraStructAddress: dword
EXTERN g_timescaleaddress: dword
EXTERN g_resolutionAddress: dword
EXTERN g_aspectratioAddress: dword
EXTERN g_battleARAddress: dword
EXTERN g_cutsceneARAddress: dword


;---------------------------------------------------------------

;---------------------------------------------------------------
; Own externs, defined in InterceptorHelper.cpp
EXTERN _cameraStructInterceptionContinue: dword
EXTERN _cameraWrite1InterceptionContinue: dword
EXTERN _timestopInterceptionContinue: dword
EXTERN _resolutionInterceptionContinue: dword
EXTERN _aspectratioInterceptionContinue: dword
EXTERN _battleARInterceptionContinue: dword
EXTERN _cutsceneARInterceptionContinue: dword


.data

.code

cutsceneARInterceptor PROC
;LRFF13.exe+30B152 - F3 0F7E 40 30         - movq xmm0,[eax+30]
;LRFF13.exe+30B157 - 66 0FD6 45 E8         - movq [ebp-18],xmm0
;LRFF13.exe+30B15C - F3 0F7E 40 38         - movq xmm0,[eax+38]
;LRFF13.exe+30B161 - 6A 01                 - push 01 { 1 }
;LRFF13.exe+30B163 - 66 0FD6 45 F0         - movq [ebp-10],xmm0		<<inject here
;LRFF13.exe+30B168 - F3 0F7E 43 18         - movq xmm0,[ebx+18]
;LRFF13.exe+30B16D - 66 0FD6 86 70020000   - movq [esi+00000270],xmm0	<<esi+270+40
;LRFF13.exe+30B175 - F3 0F7E 43 20         - movq xmm0,[ebx+20]		<< return here
;LRFF13.exe+30B17A - 6A 00                 - push 00 { 0 }
;LRFF13.exe+30B17C - 8D 55 B8              - lea edx,[ebp-48]
;LRFF13.exe+30B17F - 52                    - push edx
;LRFF13.exe+30B180 - 8B CE                 - mov ecx,esi
;LRFF13.exe+30B182 - 66 0FD6 86 78020000   - movq [esi+00000278],xmm0
	movq qword ptr [ebp-10h],xmm0
	movq xmm0,qword ptr [ebx+18h]
	movq qword ptr [esi+00000270h],xmm0
	mov [g_cutsceneARAddress],esi
	jmp dword ptr [_cutsceneARInterceptionContinue]
cutsceneARInterceptor ENDP

battleARInterceptor PROC
;LRFF13.exe+30AD1E - F3 0F11 45 D4         - movss [ebp-2C],xmm0
;LRFF13.exe+30AD23 - F3 0F11 45 E4         - movss [ebp-1C],xmm0
;LRFF13.exe+30AD28 - F3 0F10 05 30065802   - movss xmm0,[LRFF13.exe+1D80630] { (1.00) }
;LRFF13.exe+30AD30 - 6A 01                 - push 01 { 1 }
;LRFF13.exe+30AD32 - F3 0F11 45 F4         - movss [ebp-0C],xmm0	<<intercept here
;LRFF13.exe+30AD37 - F3 0F7E 43 18         - movq xmm0,[ebx+18]
;LRFF13.exe+30AD3C - 66 0FD6 86 70020000   - movq [esi+00000270],xmm0	<<esi+270+40 is AR
;LRFF13.exe+30AD44 - F3 0F7E 43 20         - movq xmm0,[ebx+20]		<<return here
;LRFF13.exe+30AD49 - 6A 00                 - push 00 { 0 }
;LRFF13.exe+30AD4B - 8D 45 B8              - lea eax,[ebp-48]
;LRFF13.exe+30AD4E - 50                    - push eax
;LRFF13.exe+30AD4F - 8B CE                 - mov ecx,esi
;LRFF13.exe+30AD51 - 66 0FD6 86 78020000   - movq [esi+00000278],xmm0
	movss dword ptr [ebp-0Ch],xmm0
	movq xmm0,qword ptr [ebx+18h]
	movq qword ptr[esi+00000270h],xmm0
	mov [g_battleARAddress],esi
	jmp dword ptr [_battleARInterceptionContinue]
battleARInterceptor ENDP



aspectratioInterceptor PROC
;LRFF13.exe+210CAA - 8B 8D FCFEFFFF        - mov ecx,[ebp-00000104]
;LRFF13.exe+210CB0 - 66 0FD6 86 0C010000   - movq [esi+0000010C],xmm0
;LRFF13.exe+210CB8 - F3 0F7E 45 F4         - movq xmm0,[ebp-0C]
;LRFF13.exe+210CBD - 66 0FD6 86 14010000   - movq [esi+00000114],xmm0			<<INJECT
;LRFF13.exe+210CC5 - F3 0F10 87 540A0000   - movss xmm0,[edi+00000A54]			EDI
;LRFF13.exe+210CCD - 6A 01                 - push 01 { 1 }						<<RETURN
;LRFF13.exe+210CCF - 83 EC 10              - sub esp,10 { 16 }
;LRFF13.exe+210CD2 - F3 0F11 44 24 0C      - movss [esp+0C],xmm0
;LRFF13.exe+210CD8 - F3 0F10 85 DCFEFFFF   - movss xmm0,[ebp-00000124]
	movq qword ptr [esi+00000114h],xmm0
	mov [g_aspectratioAddress],edi
	movss xmm0,dword ptr [edi+00000A54h]
	jmp dword ptr [_aspectratioInterceptionContinue]
aspectratioInterceptor ENDP

resolutionInterceptor PROC
;LRFF13.exe+695D50 - 55                    - push ebp
;LRFF13.exe+695D51 - 8B EC                 - mov ebp,esp
;LRFF13.exe+695D53 - A1 60E90205           - mov eax,[LRFF13.exe+4D0E960] { (DE0A6180) }
;LRFF13.exe+695D58 - 8B 4D 08              - mov ecx,[ebp+08]				<<INJECT
;LRFF13.exe+695D5B - 8B 55 0C              - mov edx,[ebp+0C]
;LRFF13.exe+695D5E - 39 88 6C010000        - cmp [eax+0000016C],ecx
;LRFF13.exe+695D64 - 75 08                 - jne LRFF13.exe+695D6E
;LRFF13.exe+695D66 - 39 90 70010000        - cmp [eax+00000170],edx
;LRFF13.exe+695D6C - 74 13                 - je LRFF13.exe+695D81
;LRFF13.exe+695D6E - 89 88 6C010000        - mov [eax+0000016C],ecx
;LRFF13.exe+695D74 - 89 90 70010000        - mov [eax+00000170],edx
;LRFF13.exe+695D7A - C6 80 6A010000 00     - mov byte ptr [eax+0000016A],00 { 0 }
;LRFF13.exe+695D81 - 5D                    - pop ebp
;LRFF13.exe+695D82 - C3                    - ret							<<RETURN
	mov [g_resolutionAddress],esi
	mov ecx,[ebp+08h]
	mov edx,[ebp+0Ch]
	cmp [eax+0000016Ch],ecx
	jne jump1
	cmp [eax+00000170h],edx
	je jump2
jump1:
	mov [eax+0000016Ch],ecx
	mov [eax+00000170h],edx
	mov byte ptr [eax+0000016Ah],00h
jump2:
	pop ebp
	jmp dword ptr [_resolutionInterceptionContinue]
resolutionInterceptor ENDP

cameraStructInterceptor PROC
;8B 8E C8 00 00 00 8B 86 C4 00 00 00 3B C1
;LRFF13.exe+26FFA3 - 75 05                 - jne LRFF13.exe+26FFAA
;LRFF13.exe+26FFA5 - A3 00000000           - mov [00000000],eax { 0 }
;LRFF13.exe+26FFAA - 8B 8E C8000000        - mov ecx,[esi+000000C8]		<<INJECT
;LRFF13.exe+26FFB0 - 8B 86 C4000000        - mov eax,[esi+000000C4]
;LRFF13.exe+26FFB6 - 3B C1                 - cmp eax,ecx				<<EDI has cam
;LRFF13.exe+26FFB8 - 7D 1F                 - jnl LRFF13.exe+26FFD9		<<RETURN
;LRFF13.exe+26FFBA - 03 46 4C              - add eax,[esi+4C]
;LRFF13.exe+26FFBD - 89 86 C4000000        - mov [esi+000000C4],eax
	mov ecx,[esi+000000C8h]
	mov eax,[esi+000000C4h]
	cmp eax,ecx
	mov [g_cameraStructAddress],edi
	jmp dword ptr [_cameraStructInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraStructInterceptor ENDP



cameraWrite1Interceptor PROC
;LRFF13.exe+30C1D0 - 55                    - push ebp
;LRFF13.exe+30C1D1 - 8B EC                 - mov ebp,esp
;LRFF13.exe+30C1D3 - 8B C1                 - mov eax,ecx
;LRFF13.exe+30C1D5 - 8B 4D 08              - mov ecx,[ebp+08]
;LRFF13.exe+30C1D8 - F3 0F7E 01            - movq xmm0,[ecx]			<<INJECT
;LRFF13.exe+30C1DC - 66 0FD6 00            - movq [eax],xmm0
;LRFF13.exe+30C1E0 - F3 0F7E 41 08         - movq xmm0,[ecx+08]
;LRFF13.exe+30C1E5 - 66 0FD6 40 08         - movq [eax+08],xmm0
;LRFF13.exe+30C1EA - F3 0F7E 41 10         - movq xmm0,[ecx+10]
;LRFF13.exe+30C1EF - 66 0FD6 40 10         - movq [eax+10],xmm0
;LRFF13.exe+30C1F4 - F3 0F7E 41 18         - movq xmm0,[ecx+18]
;LRFF13.exe+30C1F9 - 66 0FD6 40 18         - movq [eax+18],xmm0
;LRFF13.exe+30C1FE - F3 0F7E 41 20         - movq xmm0,[ecx+20]
;LRFF13.exe+30C203 - 66 0FD6 40 20         - movq [eax+20],xmm0
;LRFF13.exe+30C208 - F3 0F7E 41 28         - movq xmm0,[ecx+28]
;LRFF13.exe+30C20D - 66 0FD6 40 28         - movq [eax+28],xmm0
;LRFF13.exe+30C212 - F3 0F7E 41 30         - movq xmm0,[ecx+30]
;LRFF13.exe+30C217 - 66 0FD6 40 30         - movq [eax+30],xmm0
;LRFF13.exe+30C21C - F3 0F7E 41 38         - movq xmm0,[ecx+38]
;LRFF13.exe+30C221 - 66 0FD6 40 38         - movq [eax+38],xmm0
;LRFF13.exe+30C226 - F3 0F7E 41 40         - movq xmm0,[ecx+40]			<<RETURN
;LRFF13.exe+30C22B - 66 0FD6 40 40         - movq [eax+40],xmm0
;LRFF13.exe+30C230 - F3 0F7E 41 48         - movq xmm0,[ecx+48]
;LRFF13.exe+30C235 - 66 0FD6 40 48         - movq [eax+48],xmm0
;LRFF13.exe+30C23A - F3 0F7E 41 50         - movq xmm0,[ecx+50]
;LRFF13.exe+30C23F - 66 0FD6 40 50         - movq [eax+50],xmm0
;LRFF13.exe+30C244 - F3 0F7E 41 58         - movq xmm0,[ecx+58]
;LRFF13.exe+30C249 - 66 0FD6 40 58         - movq [eax+58],xmm0
;LRFF13.exe+30C24E - F3 0F7E 41 60         - movq xmm0,[ecx+60]
;LRFF13.exe+30C253 - 66 0FD6 40 60         - movq [eax+60],xmm0

	cmp [g_cameraEnabled],1
	jne originalcode

	push ebx
	mov ebx, [g_cameraStructAddress]
	add ebx,078h
	cmp ebx,eax
	pop ebx
	je skipwrite


originalcode:
	movq xmm0,qword ptr [ecx]
	movq qword ptr [eax],xmm0
	movq xmm0,qword ptr [ecx+08h]
	movq qword ptr [eax+08h],xmm0
	movq xmm0,qword ptr [ecx+10h]
	movq qword ptr [eax+10h],xmm0
	movq xmm0,qword ptr [ecx+18h]
	movq qword ptr [eax+18h],xmm0
	movq xmm0,qword ptr [ecx+20h]
	movq qword ptr [eax+20h],xmm0
	movq xmm0,qword ptr [ecx+28h]
	movq qword ptr [eax+28h],xmm0
	movq xmm0,qword ptr [ecx+30h]
	movq qword ptr [eax+30h],xmm0
	movq xmm0,qword ptr [ecx+38h]
	movq qword ptr [eax+38h],xmm0
	jmp exit
skipwrite:
	movq xmm0,qword ptr [ecx]
	;movq qword ptr [eax],xmm0
	movq xmm0,qword ptr [ecx+08h]
	;movq qword ptr [eax+08h],xmm0
	movq xmm0,qword ptr [ecx+10h]
	;movq qword ptr [eax+10h],xmm0
	movq xmm0,qword ptr [ecx+18h]
	;movq qword ptr [eax+18h],xmm0
	movq xmm0,qword ptr [ecx+20h]
	;movq qword ptr [eax+20h],xmm0
	movq xmm0,qword ptr [ecx+28h]
	;movq qword ptr [eax+28h],xmm0
	movq xmm0,qword ptr [ecx+30h]
	;movq qword ptr [eax+30h],xmm0
	movq xmm0,qword ptr [ecx+38h]
	;movq qword ptr [eax+38h],xmm0
exit:
	jmp dword ptr [_cameraWrite1InterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraWrite1Interceptor ENDP



timestopInterceptor PROC
;LRFF13.exe+6B96B1 - 8B 10                 - mov edx,[eax]
;LRFF13.exe+6B96B3 - 8B C8                 - mov ecx,eax
;LRFF13.exe+6B96B5 - 8B 42 3C              - mov eax,[edx+3C]	
;LRFF13.exe+6B96B8 - FF D0                 - call eax						<<call to load gamespeed
;LRFF13.exe+6B96BA - D9 5D E8              - fstp dword ptr [ebp-18]		<<ecx/INJECT
;LRFF13.exe+6B96BD - 66 0F6E 45 F0         - movd xmm0,[ebp-10]
;LRFF13.exe+6B96C2 - F3 0F10 4D E8         - movss xmm1,[ebp-18]
;LRFF13.exe+6B96C7 - 8B 16                 - mov edx,[esi]					
;LRFF13.exe+6B96C9 - 8B 42 10              - mov eax,[edx+10]				<<return
;LRFF13.exe+6B96CC - 0F5B C0               - cvtdq2ps xmm0,xmm0

	mov [g_timescaleaddress],ecx
	fstp dword ptr [ebp-18h]
	movd xmm0,dword ptr [ebp-10h]
	movss xmm1,dword ptr [ebp-18h]
	mov edx,[esi]

	jmp dword ptr [_timestopInterceptionContinue]
timestopInterceptor ENDP

END