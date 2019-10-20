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

;---------------------------------------------------------------

;---------------------------------------------------------------
; Externs which are used and set by the system. Read / write these
; values in asm to communicate with the system
EXTERN g_cameraEnabled: byte
EXTERN g_cameraStructAddress: qword

;---------------------------------------------------------------

;---------------------------------------------------------------
; Own externs, defined in InterceptorHelper.cpp
EXTERN _cameraStructInterceptionContinue: qword

.data


.code


cameraStructInterceptor PROC
;NinoKuni_WotWW_Remastered.exe+5B5CD6 - 0F58 E0               - addps xmm4,xmm0
;NinoKuni_WotWW_Remastered.exe+5B5CD9 - 0F58 E1               - addps xmm4,xmm1
;NinoKuni_WotWW_Remastered.exe+5B5CDC - 66 0F6F CF            - movdqa xmm1,xmm7
;NinoKuni_WotWW_Remastered.exe+5B5CE0 - 66 0FDF FE            - pandn xmm7,xmm6
;NinoKuni_WotWW_Remastered.exe+5B5CE4 - 66 0FDF CD            - pandn xmm1,xmm5
;NinoKuni_WotWW_Remastered.exe+5B5CE8 - 0F28 74 24 30         - movaps xmm6,[rsp+30]
;NinoKuni_WotWW_Remastered.exe+5B5CED - 0F28 C4               - movaps xmm0,xmm4
;NinoKuni_WotWW_Remastered.exe+5B5CF0 - 0FC6 05 D84B7B00 FA   - shufps xmm0,[NinoKuni_WotWW_Remastered.exe+D6A8D0],-06 { 250,(1.00) }
;NinoKuni_WotWW_Remastered.exe+5B5CF8 - 66 0F7F 12            - movdqa [rdx],xmm2			<<intercept here
;NinoKuni_WotWW_Remastered.exe+5B5CFC - 66 0F7F 4A 10         - movdqa [rdx+10],xmm1
;NinoKuni_WotWW_Remastered.exe+5B5D01 - 66 0F7F 7A 20         - movdqa [rdx+20],xmm7
;NinoKuni_WotWW_Remastered.exe+5B5D06 - 0F28 7C 24 20         - movaps xmm7,[rsp+20]
;NinoKuni_WotWW_Remastered.exe+5B5D0B - 0FC6 E0 C4            - shufps xmm4,xmm0,-3C { 196 }
;NinoKuni_WotWW_Remastered.exe+5B5D0F - 66 0F7F 62 30         - movdqa [rdx+30],xmm4
;NinoKuni_WotWW_Remastered.exe+5B5D14 - 48 83 C4 48           - add rsp,48 { 72 }			<<<return here
	mov [g_cameraStructAddress],rdx
	cmp byte ptr [g_cameraEnabled],1
	je skipwrites
	movdqa [rdx],xmm2
	movdqa [rdx+10h],xmm1
	movdqa [rdx+20h],xmm7
	movaps xmm7,[rsp+20h]
	shufps xmm4,xmm0,-3Ch
	movdqa [rdx+30h],xmm4
	jmp exit
skipwrites:
	;movdqa [rdx],xmm2
	;movdqa [rdx+10h],xmm1
	;movdqa [rdx+20h],xmm7
	movaps xmm7, [rsp+20h]
	shufps xmm4,xmm0,-3Ch
	;movdqa [rdx+30h],xmm4
exit:
	jmp qword ptr [_cameraStructInterceptionContinue]	; jmp back into the original game code, which is the location after the original statements above.
cameraStructInterceptor ENDP


END