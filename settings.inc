;-------------------------------------------------------------------------
; Functions to save & load settings.ini
;-------------------------------------------------------------------------

INVALID_FILE_ATTRIBUTES = -1
ERROR_FILE_NOT_FOUND = 2

; szSettingsPath[MAX_PATH]:BYTE
proc GetAppDataSettingsPath szSettingsPath:DWORD
                push esi
                push edi
                mov esi, [szSettingsPath]
                xor edi, edi

                push esi
                push edi
                push edi
                push CSIDL_LOCAL_APPDATA
                push edi
                call [SHGetFolderPathA]
                test eax, eax
                jnz .fail

                push esi
                call [lstrlenA]
                cmp eax, MAX_PATH - (settings.RelativeSettingsPathLen+1)
                xchg eax, edi
                ja .fail ; Malicious %appdata% it seems

                lea ecx, [eax + settings.szDirectory.strlen]
                add edi, esi
                mov esi, settings.szDirectory
                mov al, '\'
                stosb
                rep movsb

                mov cl, settings.szFilename.size
                mov al, '\'
                stosb
                rep movsb

                ; Return path length
                lea eax, [edi-1]
                sub eax, [szSettingsPath]

.fail:          pop edi
                pop esi
                ret
endp

; pSettings:SETTINGS
proc ReadAppDataSettings pSettings:DWORD
                local szPath[MAX_PATH]:BYTE
                push ebx
                push esi
                push edi

                ; Get settings.ini path
                lea esi, [szPath]
                push esi
                call GetAppDataSettingsPath
                test eax, eax
                jz .fail

                ; Create settings directory if missing
                push '\'
                lea edi, [esi+eax]
                pop ecx
                xchg eax, ecx
                std
                repnz scasb
                cld
                inc edi
                mov [edi], ah

                push esi
                call [GetFileAttributesA]
                cmp eax, FILE_ATTRIBUTE_DIRECTORY
                jz .dir_ok
                cmp eax, INVALID_FILE_ATTRIBUTES
                jnz .fail
                call [GetLastError]
                cmp eax, ERROR_FILE_NOT_FOUND
                jnz .fail
                push NULL
                push esi
                call [CreateDirectoryA]
                test eax, eax
                jz .fail
.dir_ok:        mov byte [edi], '\'

.read_ini:      ; memzero(pSettings)
                xor eax, eax
                lea ecx, [eax + sizeof.SETTINGS]
                mov edi, [pSettings]
                push edi
                rep stosb
                pop edi

                ; Calculate default x & y coordinates
                call [GetDpiForSystem]
                mov ebx, eax

                push ebx
                push SM_CXSCREEN
                call [GetSystemMetricsForDpi]
                shr eax, 1
                mov [edi + SETTINGS.GUI.x], eax
                mov [edi + SETTINGS.GUI.cx], eax

                push 96*2
                push ebx
                push WIN_WIDTH
                call [MulDiv]
                sub [edi + SETTINGS.GUI.x], eax
                add [edi + SETTINGS.GUI.cx], eax

                push ebx
                push SM_CYSCREEN
                call [GetSystemMetricsForDpi]
                shr eax, 1
                mov [edi + SETTINGS.GUI.y], eax
                mov [edi + SETTINGS.GUI.cy], eax

                push 96*2
                push ebx
                push WIN_HEIGHT
                call [MulDiv]
                sub [edi + SETTINGS.GUI.y], eax
                add [edi + SETTINGS.GUI.cy], eax

                ; [GUI]
                mov ecx, .get_int
                mov ebx, settings.szSectionGUI
                lea esi, [ebx + (settings.szKeyX - settings.szSectionGUI)]

                mov edx, [edi] ;mov edx, [edi + SETTINGS.GUI.x]
                call ecx
                stosd
                mov edx, [edi] ;mov edx, [edi + SETTINGS.GUI.y]
                call ecx
                stosd
                mov edx, [edi] ;mov edx, [edi + SETTINGS.GUI.cx]
                call ecx
                cmp eax, [edi-2*4 + SETTINGS.GUI.x]
                stosd
                jle .corrupt
                mov edx, [edi] ;mov edx, [edi + SETTINGS.GUI.cy]
                call ecx
                cmp eax, [edi-3*4 + SETTINGS.GUI.y]
                stosd
                jle .corrupt

                ; [Devices]
                add ebx, settings.szSectionDevice - settings.szSectionGUI
                lea eax, [szPath]
                push eax
                push WINBIO_MAX_STRING_LEN
                push edi
                push NULL
                push esi
                push ebx
                call [GetPrivateProfileStringA]
                mov ecx, WINBIO_MAX_STRING_LEN
                add esi, settings.szInstanceId.size

                ; convert InstanceId to wsz
                push esi
                lea esi, [edi + ecx-1]
                lea edi, [edi + ecx*2]
                push edi
                std
                dec edi
                dec edi
@@:             lodsb
                stosw
                loop @b
                cld
                pop edi
                pop esi

                ; [Commands]
                mov eax, edi
                mov ecx, MAX_PATH
                mov edx, .get_str
                add ebx, settings.szSectionCommand - settings.szSectionDevice

                ; lea edi, [pSettings + SETTINGS.cmd.RThumb]
                ; mov esi, settings.szRThumb
                call edx
                ; lea edi, [pSettings + SETTINGS.cmd.RIndex]
                ; mov esi, settings.szRIndex
                call edx
                ; lea edi, [pSettings + SETTINGS.cmd.RMiddle]
                ; mov esi, settings.RMiddle
                call edx
                ; lea edi, [pSettings + SETTINGS.cmd.RRing]
                ; mov esi, settings.RRing
                call edx
                ; lea edi, [pSettings + SETTINGS.cmd.RLittle]
                ; mov esi, settings.RLittle
                call edx
                ; lea edi, [pSettings + SETTINGS.cmd.LRThumb]
                ; mov esi, settings.szLThumb
                call edx
                ; lea edi, [pSettings + SETTINGS.cmd.LIndex]
                ; mov esi, settings.szLIndex
                call edx
                ; lea edi, [pSettings + SETTINGS.cmd.LMiddle]
                ; mov esi, settings.LMiddle
                call edx
                ; lea edi, [pSettings + SETTINGS.cmd.RRing]
                ; mov esi, settings.RRing
                call edx
                ; lea edi, [pSettings + SETTINGS.cmd.LLittle]
                ; mov esi, settings.LLittle
                call edx

                push 1
                pop eax
                jmp .ok
.fail:          xor eax, eax
.ok:            pop edi
                pop esi
                pop ebx
                ret

.corrupt:       lea eax, [szPath]
                push eax
                call [DeleteFileA]
                jmp .read_ini

.get_int:       ; Input: edx = default, ebx = section, esi = key, [szPath]
                ; Output: eax = int, esi = key + strlen(esi)+1
                push ecx
                push edx

                lea eax, [szPath]
                push eax
                push edx
                push esi
                push ebx

@@:             lodsb
                test al, al
                jnz @b

                call [GetPrivateProfileIntA]

                pop edx
                pop ecx
                retn

.get_str:       ; ecx = size, ebx = section, esi = key, [szPath]
                ; Output: edu = str, edu = edi + ecx, esi = key + strlen(esi)+1
                push ecx
                push edx

                lea eax, [szPath]
                push eax
                push ecx
                push edi
                push NULL
                push esi
                push ebx

@@:             lodsb
                test al, al
                jnz @b

                call [GetPrivateProfileStringA]

                pop edx
                pop ecx
                add edi, ecx
                retn
endp

proc WriteAppDataSettings pSettings:DWORD
                local szItoaBuf[16]:BYTE
                local szPath[MAX_PATH]:BYTE

                lea eax, [szPath]
                push eax
                call GetAppDataSettingsPath
                test eax, eax
                jz .ret

                push ebx
                push esi
                push edi

                mov esi, [pSettings]

                ; [GUI]
                mov ebx, settings.szSectionGUI
                mov edx, .write_int

                lodsd ;mov eax, [pSettings + SETTINGS.GUI.x]
                lea edi, [ebx + (settings.szKeyX - settings.szSectionGUI)]
                call edx

                lodsd ;mov eax, [pSettings + SETTINGS.GUI.y]
                ;mov edi, settings.szKeyY
                call edx

                lodsd ;mov eax, [pSettings + SETTINGS.GUI.cx]
                ;mov edi, settings.szKeyCX
                call edx

                lodsd ;mov eax, [pSettings + SETTINGS.GUI.cy]
                ;mov edi, settings.szKeyCY
                call edx

                ; [Devices]
                add ebx, settings.szSectionDevice - settings.szSectionGUI
                lea eax, [szPath]
                push eax
                push esi
                push edi
                push ebx

                mov ecx, WINBIO_MAX_STRING_LEN
                add edi, settings.szInstanceId.size
                push edi
                mov edi, esi
                push ecx
@@:             lodsw
                stosb
                loop @b
                mov eax, ecx
                pop ecx
                rep stosb
                pop edi

                call [WritePrivateProfileStringA]

                ; [Commands]
                mov eax, esi
                mov ecx, MAX_PATH
                mov edx, .write_str
                add ebx, settings.szSectionCommand - settings.szSectionDevice

                ; lea eax, [pSettings + SETTINGS.cmd.RThumb]
                ; mov edi, settings.szRThumb
                call edx
                ; lea eax, [pSettings + SETTINGS.cmd.RIndex]
                ; mov edi, settings.szRIndex
                call edx
                ; lea eax, [pSettings + SETTINGS.cmd.RMiddle]
                ; mov edi, settings.RMiddle
                call edx
                ; lea eax, [pSettings + SETTINGS.cmd.RRing]
                ; mov edi, settings.RRing
                call edx
                ; lea eax, [pSettings + SETTINGS.cmd.RLittle]
                ; mov edi, settings.RLittle
                call edx
                ; lea eax, [pSettings + SETTINGS.cmd.LRThumb]
                ; mov edi, settings.szLThumb
                call edx
                ; lea eax, [pSettings + SETTINGS.cmd.LIndex]
                ; mov edi, settings.szLIndex
                call edx
                ; lea eax, [pSettings + SETTINGS.cmd.LMiddle]
                ; mov edi, settings.LMiddle
                call edx
                ; lea eax, [pSettings + SETTINGS.cmd.RRing]
                ; mov edi, settings.RRing
                call edx
                ; lea eax, [pSettings + SETTINGS.cmd.LLittle]
                ; mov edi, settings.LLittle
                call edx

.fail:          pop edi
                pop esi
                pop ebx
.ret:           ret

                ; Input: eax = int, ebx = section, edi = key, [szPath]
                ; Output: edi = edi + strlen(edi)+1
.write_int:     push eax
                push ecx
                push edx

                lea ecx, [szPath]
                lea edx, [ecx + (szItoaBuf-szPath)]

                push ecx
                push edx
                push edi
                push ebx

                push eax
                push settings.szFormatD
                push edx
                call [wsprintfA]
                add esp, 0ch

                call [WritePrivateProfileStringA]

                xor eax, eax
                repnz scasb

                pop edx
                pop ecx
                pop eax
                retn

                ; Input: eax = str, ebx = section, edi = key, [szPath]
                ; Output: eax = eax + ecx, edi = edi + strlen(edi)+1
.write_str:     push eax
                push ecx
                push edx

                lea ecx, [szPath]
                push ecx
                push eax
                push edi
                push ebx
                call [WritePrivateProfileStringA]

                xor eax, eax
                repnz scasb

                pop edx
                pop ecx
                pop eax
                add eax, ecx
                retn
endp

DeleteAppDataSettings:
                push ebp
                mov ebp, esp
                sub esp, MAX_PATH

                push esi
                push edi

                ; Delete settings.ini
                lea esi, [esp+8];lea esi, [szSettingsPath]
                push esi
                call GetAppDataSettingsPath
                push eax
                push esi
                call [DeleteFileA]
                pop ecx

                ; Then try to remove the containing folder
                mov al, '\'
                dec ecx
                lea edi, [esi+ecx]
                std
                repnz scasb
                cld
                inc edi
                mov al, 0
                stosb

                push esi
                call [RemoveDirectoryA]

                pop edi
                pop esi
                leave
                retn