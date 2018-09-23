;-------------------------------------------------------------------------;
; Command-line interface (very rudimentary)
;-------------------------------------------------------------------------;

proc CommandLineInterface
                local argc:DWORD
                local argv:DWORD

                ; Debugging ...
                ;call @f
                ;du '"C:\Users\Hitler\Desktop\HotFinger\hotfinger.exe" install',0
                ;du '"C:\Users\Hitler\Desktop\HotFinger\hotfinger.exe" install USB\VID_1488&PID_1488\222222222222',0
                ;du '"C:\Users\Hitler\Desktop\HotFinger\hotfinger.exe" uninstall',0
                ;@@: pop eax
                ;jmp @f

                ; Read command-line and skip argv[0]
                call [GetCommandLineW]
@@:             lea ecx, [argc]
                push ecx
                push eax
                call [CommandLineToArgvW]
                test eax, eax
                jnz @f
                ret
@@:             movzx ecx, byte [argc]
                mov [argv], eax

                cmp cl, 2
                jnz .three_args
                push eax
                push dword [eax + 4]
                push clidata.wszInstall
                call [lstrcmpW]
                test eax, eax
                pop eax
                jz .install
                push dword [eax + 4]
                push clidata.wszUninstall
                call [lstrcmpW]
                test eax, eax
                jnz .exit
                push NULL
                push guidDatabase
                call UninstallDatabase
                jmp .exit_process

.install:       push guidDatabase
                call InstallDatabase
                jmp .exit_process

.three_args:    cmp cl, 3
                jnz .exit
                push dword [eax + 8]
                push dword [eax + 4]
                push clidata.wszInstall
                call [lstrcmpW]
                test eax, eax
                pop eax
                jnz .exit
.install_for:   push eax
                push guidDatabase
                call InstallDatabaseForSensor
                jmp .exit_process

.exit:          push [argv]
                call [LocalFree]
                ret
.exit_process:  push eax
                push [argv]
                call [LocalFree]
                call [ExitProcess]
endp