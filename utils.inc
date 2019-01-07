;-------------------------------------------------------------------------;
; Miscellaneous utilities
;-------------------------------------------------------------------------;

SERVICE_ALL_ACCESS   = 0f01ffh
SERVICE_CONTROL_STOP = 1
SERVICE_STOPPED      = 1
SERVICE_RUNNING      = 4

struct SERVICE_STATUS
  dwServiceType             dd ?
  dwCurrentState            dd ?
  dwControlsAccepted        dd ?
  dwWin32ExitCode           dd ?
  dwServiceSpecificExitCode dd ?
  dwCheckPoint              dd ?
  dwWaitHint                dd ?
ends

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

;proc ValidateGuidString szGUID:DWORD
ValidateGuidString:
                push ebx
                push edi
                mov ebx, utils.asciiDigitValueTable
                mov edi, dword [esp + 2*4 + 4] ; [szGUID]

                push edi
                call [lstrlenA]

                cmp eax, 38
                ja .invalid

                cmp eax, 36
                jz .no_braces
                cmp eax, 38
                jnz .invalid
                mov al, '{'
                scasb
                jnz .invalid
                cmp byte [edi+36-1], '}'
                jnz .invalid
.no_braces:
                xor ecx, ecx
                push ecx
                push 12
                push 4
                push 4
                push 4
                mov cl, 8
.loop:          xchg esi, edi
@@:             lodsb
                xlatb
                test al, al
                js .invalid
                loop @b
                pop ecx
                xchg esi, edi
                jecxz .valid
                mov al, '-'
                scasb
                jz .loop

.invalid:       xor eax, eax
                jmp .exit
.valid:         mov al, 1
.exit:          pop edi
                pop ebx
                retn 4h

; guid[16]:BYTE
;proc StringToGuid szIn:DWORD, guid:DWORD
StringToGuid:
                push dword [esp+4]
                call ValidateGuidString
                test eax, eax
                jnz @f
                retn 8h
@@:
                push ebp
                push ebx
                push esi
                push edi
                virtual at esp
                  .regs dd 4 dup (?)    ; saved ebp ebx esi edi
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
                retn 8h

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
                pop eax
                xchg [esp], eax
                sub eax, WINBIO_E_MIN
                cmp eax, WINBIO_E_MAX - WINBIO_E_MIN
                ja .no_winbio_err
                mov eax, [utils.pszErrorTextTable + eax*4]
                retn
.no_winbio_err: xor eax, eax
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

                push 5000
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
; WbioSrvc restart needed, for example, after registering a new database-
;--------------------------------------------------------------------------;

WbioSrvcRestart:
                virtual at ebp
                  .status SERVICE_STATUS ?
                  .counter dd ?
                  .allocaSize = $ - $$
                end virtual
                push ebp
                sub esp, .allocaSize
                mov ebp, esp
                xor eax, eax
                push eax ; [.rc]

                push GENERIC_EXECUTE
                push utils.szSERVICES_ACTIVE_DATABASE
                push eax
                call [OpenSCManagerA]
                test eax, eax
                jz .fail_manager

                push eax ; For CloseServiceHandle

                push SERVICE_ALL_ACCESS
                push utils.szWbioSrvc
                push eax
                call [OpenServiceA]
                test eax, eax
                jz .fail_service

                push ebx
                mov ebx, eax

                ; Stop the service
                push -1
                pop [.counter]
.stop_service:  push ebp
                push ebx
                call [QueryServiceStatus]
                test eax, eax
                jz .fail
                cmp [ebp + SERVICE_STATUS.dwCurrentState], SERVICE_STOPPED
                jz .restart

                inc [.counter]
                jz .request_stop
                cmp [.counter], 10
                jae .fail

                push 100
                call [Sleep]
                jmp .stop_service

.request_stop:  push ebp
                push SERVICE_CONTROL_STOP
                push ebx
                call [ControlService]
                test eax, eax
                jz .fail
                jmp .stop_service

.restart:       ; Restart the service
                xor eax, eax
                mov [.counter], eax

                push eax ; NULL
                push eax ; 0
                push ebx
                call [StartServiceA]
                test eax, eax
                jz .fail

.query_restart: push ebp
                push ebx
                call [QueryServiceStatus]
                test eax, eax
                jz .fail
                cmp [ebp + SERVICE_STATUS.dwCurrentState], SERVICE_RUNNING
                jz .restarted

                inc [.counter]
                jz .request_stop
                cmp [.counter], 10
                jae .fail

                push 100
                call [Sleep]
                jmp .query_restart
.restarted:

                mov dword [esp+8], 1 ; mov [.rc], 1
.fail:          xchg [esp], ebx
                call [CloseServiceHandle]
.fail_service:  call [CloseServiceHandle]
.fail_manager:  pop eax ; mov eax, [.rc]
                add esp, .allocaSize
                pop ebp
                retn
