;-------------------------------------------------------------------------;
; Miscellaneous utilities
;-------------------------------------------------------------------------;

;--------------------------------------------------------------------------;
; GUID strings
;--------------------------------------------------------------------------;

; szOut[39]:BYTE
proc GuidToString guid:DWORD, szOut:DWORD
                push esi
                xor eax, eax
                push 15
                push 8
                pop ecx
                pop esi
                add esi, [guid]
                std
@@:             lodsb
                push eax
                loop @b
                dec esi
                lodsw
                push eax
                lodsw
                push eax
                dec esi
                dec esi
                lodsd
                lea esi, [szOut]
                push eax
                cld
                push utils.szGuidFormat
                push dword [esi]
                call [wsprintfA]
                add esp, 8+11*4
                lodsd ; return szOut
                pop esi
                ret
endp

; guid[16]:BYTE
;proc StringToGuid szIn:DWORD, guid:DWORD
StringToGuid:
                push ebp
                push ebx
                push esi
                push edi
                virtual at esp
                  .regs dd 4 dup (?)    ; saved ebp esi edi
                  .addr dd ?            ; return address
                  .szIn dd ?            ; 1st param
                  .guid dd ?            ; 2nd param
                end virtual

                xor ecx, ecx
                mov esi, [.szIn]
                mov edi, [.guid]
                mov ebx, utils.asciiDigitValueTable
                mov ebp, .conv
                push edi                ; return value

                ; Skip brace (if any)
                cmp byte [esi], '{'     ; {
                jnz @f
                inc esi
@@:
                mov cl, 8
                call ebp                ; %08X
                inc esi                 ; -
                stosd
                mov cl, 4
                call ebp                ; %04X
                inc esi                 ; -
                stosw
                mov cl, 4
                call ebp                ; %04X
                inc esi                 ; -
                stosw
                mov cl, 2
                call ebp                ; %02X
                stosb
                mov cl, 2
                call ebp                ; %02X
                inc esi                 ; -
                stosb

                push 6
@@:             mov cl, 2
                call ebp                ; %02X%02X%02X%02X%02X%02X
                stosb
                dec dword [esp]
                jnz @b
                pop eax

                pop eax
                pop edi
                pop esi
                pop ebx
                pop ebp
                retn 8

                ; convert ASCII hexdigits to integer value
                ; esi = input, ecx = count, ebx = digit-value table
.conv:          xor edx, edx
@@:             lodsb
                xlatb
                shl edx, 4
                or dl, al
                loop @b
                mov eax, edx
                retn
;endp
                ; convert ASCII digits to integer value
                ; esi = input string (\0-terminated)
atoi:           push ebx
                push esi
                xor eax, eax
                xor edx, edx
                push 10
                mov ebx, utils.asciiDigitValueTable
                pop ecx
@@:             lodsb
                test al, al
                jz .done
                xlatb
                lea edx, [edx+edx*4]
                shl edx, 1
                add edx, eax
                loop @b
.done:          mov eax, edx
                pop esi
                pop ebx
                retn

;--------------------------------------------------------------------------;
; Run command as admin with ShellExecuteW
;--------------------------------------------------------------------------;

proc RunAsAdmin wszParameters:DWORD, hwndParent:DWORD
                local wszArgv0[MAX_PATH+2]:WORD
                local shinfo:SHELLEXECUTEINFO

                push edi

                xor eax, eax
                mov ecx, sizeof.SHELLEXECUTEINFO
                lea edi, [shinfo]
                push ecx
                push edi
                rep stosb
                pop edi
                pop [edi + SHELLEXECUTEINFO.cbSize]
                mov [edi + SHELLEXECUTEINFO.fMask], SEE_MASK_NOCLOSEPROCESS
                push [hwndParent]
                pop [edi + SHELLEXECUTEINFO.hwnd]
                mov [edi + SHELLEXECUTEINFO.lpVerb], utils.wszRunAs
                push [wszParameters]
                pop [edi + SHELLEXECUTEINFO.lpParameters]
                mov [edi + SHELLEXECUTEINFO.nShow], SW_SHOW

                push MAX_PATH+1
                lea eax, [wszArgv0]
                mov [edi + SHELLEXECUTEINFO.lpFile], eax
                push eax
                push NULL
                call [GetModuleFileNameW]
                test eax, eax
                js .fail

                push edi
                call [ShellExecuteExW]
                test eax, eax
                jz .fail

                push 50000
                push [edi + SHELLEXECUTEINFO.hProcess]
                call [WaitForSingleObject]
                test eax, eax
                jnz .close_and_fail
                push eax
                push esp
                push [edi + SHELLEXECUTEINFO.hProcess]
                call [GetExitCodeProcess]
                push [edi + SHELLEXECUTEINFO.hProcess]
                call [CloseHandle]
                pop eax

                pop edi
                ret

.close_and_fail:
                push [edi + SHELLEXECUTEINFO.hProcess]
                call [CloseHandle]
.fail:          or eax, -1
                pop edi
                ret
endp

;--------------------------------------------------------------------------;
; Subfactor text functions
;--------------------------------------------------------------------------;

proc SubfactorText dwSubfactor:DWORD
                mov eax, [dwSubfactor]
                cmp eax, 10
                jbe @f
                xor eax, eax
@@:             mov eax, [utils.pszSubfactorTable + eax*4]
                ret
endp

proc SubfactorFullText dwSubfactor:DWORD
                mov eax, [dwSubfactor]
                cmp eax, 10
                jbe @f
                xor eax, eax
@@:             mov eax, [utils.pszSubfactorFullTable + eax*4]
                ret
endp

proc SubfactorSimpleText dwSubfactor:DWORD
                push [dwSubfactor]
                call SubfactorText
                cmp [eax+1], ' '
                jnz @f
                add eax, 2
@@:             ret
endp

;--------------------------------------------------------------------------;
; WinBio error strings
;--------------------------------------------------------------------------;

;proc WinBioErrorText dwErrorCode:DWORD
WinBioErrorText:
                mov eax, [esp + 4]
                sub eax, WINBIO_E_MIN
                jz .no_winbio_err
                cmp eax, WINBIO_E_MAX - WINBIO_E_MIN
                ja .no_winbio_err
                mov eax, [utils.pszErrorTextTable + eax*4]
                retn 4
.no_winbio_err: xor eax, eax
                retn 4

;--------------------------------------------------------------------------;
; Auxiliary fastcall functions for manipulating Windows registry keys.
; ebx = hKey, esi = szValueName, edi = buffer, ecx = len (if applicable).
; Upon return, edi = edi + ecx and CF=1 indicates an error.
;--------------------------------------------------------------------------;
Regaux:
.getGuid:       virtual at esp
                  .szGuid db 39 dup (?)
                  .pad0   db ?
                  .allocaSize = $ - $$
                end virtual
                sub esp, .allocaSize
                lea eax, [.szGuid]
                push 39
                push esp
                push eax
                push 0
                push RRF_RT_REG_SZ
                push esi
                push NULL
                push ebx
                call [RegGetValueA]
                test eax, eax
                pop eax
                jnz .getGuid.error
                cmp eax, 37
                jz @f
                cmp eax, 39
                jnz .getGuid.error
@@:             lea eax, [.szGuid]
                push edi
                push eax
                call StringToGuid
                clc
.getGuid.done:  lea edi, [edi + 16]
                lea esp, [esp + .allocaSize]
                retn
.getGuid.error: stc
                jmp .getGuid.done

.getSz:         push ecx
                push ecx
                push esp
                push edi
                add edi, ecx
                push 0
                push RRF_RT_REG_SZ
                push esi
                push NULL
                push ebx
                call [RegGetValueA]
                test eax, eax
                pop eax
                clc
                jz @f
                stc
@@:             pop ecx
                retn

.getDword:      push 4
                push esp
                push edi
                push 0
                push RRF_RT_REG_DWORD
                push esi
                push NULL
                push ebx
                call [RegGetValueA]
                test eax, eax
                pop eax
                clc
                jz @f
                stc
@@:             lea edi, [edi + 4]
                retn

;--------------------------------------------------------------------------;

.setGuid39:     push 0  ; without braces
                jmp @f
.setGuid:       push 1  ; with braces
@@:             ;virtual at esp
                ;  .szGuid db 39 dup (?)
                ;  .pad0   db ?
                ;  .allocaSize = $ - $$
                ;end virtual
                sub esp, .allocaSize

                push esp
                push edi
                call GuidToString

                mov edx, esp
                add edx, [esp + .allocaSize]
                sub eax, edx
                lea ecx, [39+2*eax]
                mov byte [esp+ecx], 0

                push ecx
                push edx
                push REG_SZ
                push 0
                push esi
                push ebx
                call [RegSetValueExA]
                test eax, eax
                clc
                jz @f
                stc
@@:
                lea edi, [edi + 16]
                lea esp, [esp + .allocaSize + 4]
                retn

.setSz:         push ecx
                mov eax, ecx            ; store max length to eax
                xor ecx, ecx
@@:             cmp byte [edi+ecx], 0   ; calculate real length
                lea ecx, [ecx+1]
                jnz @b

                push ecx
                push edi
                add edi, eax
                push REG_SZ
                push 0
                push esi
                push ebx
                call [RegSetValueExA]
                test eax, eax
                clc
                jz @f
                stc
@@:             pop ecx
                retn

.setDword:      push 4
                push edi
                push REG_DWORD
                push 0
                push esi
                push ebx
                call [RegSetValueExA]
                add edi, 4
                test eax, eax
                clc
                jz @f
                stc
@@:             retn