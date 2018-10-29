;-------------------------------------------------------------------------;
; WinBio database management functions
;-------------------------------------------------------------------------;

KEY_WOW64_64KEY = 0100h
KEY_WOW64_32KEY = 0200h

RRF_RT_REG_SZ    = 02h
RRF_RT_REG_DWORD = 10h

ERROR_PATH_NOT_FOUND = 3
ERROR_ACCESS_DENIED  = 5

RPC_S_SERVER_TOO_BUSY    = 000006BBh
RPC_X_SS_IN_NULL_CONTEXT = 000006EFh

struct WBIOSVC_SENSOR
  guidDatabaseId         db 16 dup (?)
  dwSensorMode           dd ?
  dwSystemSensor         dd ?
  szEngineAdapterBinary  db MAX_PATH dup (?)
  szSensorAdapterBinary  db MAX_PATH dup (?)
  szStorageAdapterBinary db MAX_PATH dup (?)
ends

struct WBIOSVC_DATABASE
  dwAttributes       dd ?
  dwAutoCreate       dd ?
  dwAutoName         dd ?
  dwBiometricType    dd ?
  szConnectionString db WINBIO_MAX_STRING_LEN dup (?)
  szFilePath         db MAX_PATH dup (?)
  guidFormat         db 16 dup (?)
  dwInitialSize      dd ?
ends

; Common prologue/epilogue for the 4 functions below:
;proc ReadWbiosvcSensorKey hKey:DWORD, pSensor:DWORD
;proc ReadWbiosvcDatabaseKey hKey:DWORD, pDatabase:DWORD
;proc SaveWbiosvcSensorKey hKey:DWORD, pSensor:DWORD
;proc SaveWbiosvcDatabaseKey hKey:DWORD, pDatabase:DWORD
ReadSaveWrapper:
.prologue:      pop eax
                push ebx
                mov ebx, [esp+4h+4h+0] ; [arg0]
                push ebp
                mov ebp, Regaux.getDword
                push esi
                mov esi, database.szDatabaseId
                push edi
                mov edi, [esp+10h+4h+4h] ; [arg1]

                call eax

.epilogue:      sbb eax, eax
                inc eax
                pop edi
                pop esi
                pop ebp
                pop ebx
                retn 8h

; pSensor:WBIOSVC_SENSOR
;proc ReadWbiosvcSensorKey hKey:DWORD, pSensor:DWORD
ReadWbiosvcSensorKey:
                call ReadSaveWrapper
                ;mov ebx, [hKey]
                ;mov ebp, Regaux.getDword
                ;mov esi, database.szDatabaseId
                ;mov edi, [pSensor]

                ;add edi, WBIOSVC_SENSOR.guidDatabaseId ; offset 0
                call Regaux.getGuid

                ;mov esi, database.szSensorMode
                call ebp

                ;mov esi, database.szSystemSensor
                call ebp

                add ebp, Regaux.getSz - Regaux.getDword
                ;mov esi, database.szEngineAdapterBinary
                mov ecx, MAX_PATH
                call ebp

                ;mov esi, database.szSensorAdapterBinary
                call ebp

                ;mov esi, database.szStorageAdapterBinary
                call ebp

                retn

; pDatabase:WBIOSVC_DATABASE
;proc ReadWbiosvcDatabaseKey hKey:DWORD, pDatabase:DWORD
ReadWbiosvcDatabaseKey:
                call ReadSaveWrapper

                ;mov ebx, [hKey]
                ;mov ebp, Regaux.getDword
                ;mov esi, database.szAttributes
                add esi, database.szAttributes - database.szDatabaseId
                ;mov edi, [pDatabase]

                ;add edi, WBIOSVC_DATABASE.dwAttributes ; offset 0
                call ebp

                ;mov esi, database.szAutoCreate
                call ebp

                ;mov esi, database.szAutoName
                call ebp

                ;mov esi, database.szBiometricType
                call ebp

                ;mov esi, database.szConnectionString
                mov ecx, WINBIO_MAX_STRING_LEN
                add ebp, Regaux.getSz - Regaux.getDword
                call ebp

                ;mov esi, database.szFilePath
                ;mov ecx, WINBIO_MAX_STRING_LEN
                call ebp

                ;mov esi, database.szFormat
                call Regaux.getGuid

                ;mov esi, database.szInitialSize
                call Regaux.getDword

                retn

; pSensor:WBIOSVC_SENSOR
;proc SaveWbiosvcSensorKey hKey:DWORD, pSensor:DWORD
SaveWbiosvcSensorKey:
                call ReadSaveWrapper

                ;mov ebx, [hKey]
                ;mov ebp, Regaux.setDword
                add ebp, Regaux.setDword - Regaux.getDword
                ;mov esi, database.szDatabaseId
                ;mov edi, [pSensor]

                call Regaux.setGuid

                ;mov esi, database.szSensorMode
                call ebp

                ;mov esi, database.szSystemSensor
                call ebp

                add ebp, Regaux.setSz - Regaux.setDword
                ;mov esi, database.szEngineAdapterBinary
                mov ecx, MAX_PATH
                call ebp

                ;mov esi, database.szSensorAdapterBinary
                call ebp

                ;mov esi, database.szStorageAdapterBinary
                call ebp

                retn

; pDatabase:WBIOSVC_DATABASE
;proc SaveWbiosvcDatabaseKey hKey:DWORD, pDatabase:DWORD
SaveWbiosvcDatabaseKey:
                call ReadSaveWrapper
                ;mov ebx, [hKey]
                ;mov ebp, Regaux.setDword
                add ebp, Regaux.setDword - Regaux.getDword
                ;mov esi, database.szAttributes
                add esi, database.szAttributes - database.szDatabaseId
                ;mov edi, [pDatabase]

                ;add edi, WBIOSVC_DATABASE.dwAttributes ; offset 0
                call ebp

                ;mov esi, database.szAutoCreate
                call ebp

                ;mov esi, database.szAutoName
                call ebp

                ;mov esi, database.szBiometricType
                call ebp

                ;mov esi, database.szConnectionString
                mov ecx, WINBIO_MAX_STRING_LEN
                call Regaux.setSz

                ;mov esi, database.szFilePath
                mov ecx, MAX_PATH
                call Regaux.setSz

                ;mov esi, database.szFormat
                call Regaux.setGuid

                ;mov esi, database.szInitialSize
                call ebp

                retn

;-------------------------------------------------------------------------;
; Iterate WinBio sensor configuration keys for (all) device instances.
;
;   HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Enum\<DeviceInstanceId>
;                     \Device Parameters\WinBio\Configurations\<Id>
;
; wszDeviceInstanceId[WINBIO_MAX_STRING_LEN]:WORD (iter all if NULL)
; fnFilter = proc hKey:DWORD, wszDeviceInstanceId:DWORD, dwConfigId:DWORD, pUser:DWORD
;
; If fnFilter returns ...
; * eax = -2, then delete hKey and stop iteration and return NÚLL.
; * eax = -1, then delete hKey and iterate to the next key.
; * eax =  0, then close hKey and iterate to the next key.
; * eax =  1, then stop iteration and return hKey.
;-------------------------------------------------------------------------;
proc IterSensorConfigurations pwszDeviceInstanceId:DWORD, fnFilter:DWORD, pUser:DWORD
                local szKey[65+WINBIO_MAX_STRING_LEN+1+255+1 + 2]:BYTE
                local szConfigId[32]:BYTE
                local wszDeviceInstanceId[WINBIO_MAX_STRING_LEN+2]:WORD
                local dwConfigId:DWORD
                local hConfigurationsKey:DWORD
                local dwConfigurationsKeyWritable:DWORD
                local dwConfigurationIdx:DWORD
                local hConfigurationKey:DWORD
                local dwInstanceLen:DWORD
                local dwInstanceIdx:DWORD
                local hInstanceKey:DWORD
                local hDeviceKey:DWORD
                local dwDeviceLen:DWORD
                local dwDeviceIdx:DWORD
                local hDevicesKey:DWORD
                local dwEnumLen:DWORD
                local dwEnumIdx:DWORD
                local hEnumKey:DWORD

                xor eax, eax
                push esi
                push edi
                push eax ; return value

                cmp [pwszDeviceInstanceId], eax
                jz .iter_all

                mov esi, [pwszDeviceInstanceId]
                push esi
                call [lstrlenW]
                test eax, eax
                lea ecx, [eax+1]
                jz .exit
                cmp eax, WINBIO_MAX_STRING_LEN
                ja .exit
                lea edi, [wszDeviceInstanceId]
                lea eax, [szKey]
                push edi
                push database.szSensorKeyFmt
                push eax
                rep movsw
                call [wsprintfA]
                add esp, 0ch

                lea eax, [hConfigurationsKey]
                push eax
                push KEY_READ + KEY_WOW64_32KEY + KEY_SET_VALUE
                push 0
                lea eax, [szKey]
                push eax
                push HKEY_LOCAL_MACHINE
                call [RegOpenKeyExA]
                test eax, eax
                jnz .exit
                jmp .configs_loop

.iter_all:      lea eax, [hEnumKey]
                push eax
                push KEY_READ + KEY_WOW64_32KEY
                push 0
                push database.szEnumKey
                push HKEY_LOCAL_MACHINE
                call [RegOpenKeyExA]
                test eax, eax
                jnz .enum_end

                mov [dwEnumIdx], 0
.enum_iter:     xor eax, eax
                push eax
                push eax
                push eax
                push eax
                lea eax, [dwEnumLen]
                mov dword [eax], WINBIO_MAX_STRING_LEN
                push eax
                lea eax, [wszDeviceInstanceId]
                push eax
                push [dwEnumIdx]
                inc [dwEnumIdx]
                push [hEnumKey]
                call [RegEnumKeyExW]
                test eax, eax
                jnz .enum_end

                lea eax, [hDevicesKey]
                push eax
                push KEY_READ + KEY_WOW64_32KEY
                push 0
                lea eax, [wszDeviceInstanceId]
                push eax
                push [hEnumKey]
                call [RegOpenKeyExW]
                test eax, eax
                jnz .enum_iter

                mov [dwDeviceIdx], 0
.devices_iter:  xor eax, eax
                push eax
                push eax
                push eax
                push eax
                lea eax, [dwDeviceLen]
                mov ecx, [dwEnumLen]
                mov dword [eax], WINBIO_MAX_STRING_LEN
                sub dword [eax], ecx
                push eax
                mov ax, '\'
                lea edi, [wszDeviceInstanceId + ecx*2]
                stosw
                push edi
                push [dwDeviceIdx]
                inc [dwDeviceIdx]
                push [hDevicesKey]
                call [RegEnumKeyExW]
                test eax, eax
                jnz .devices_end
                lea eax, [hDeviceKey]
                push eax
                push KEY_READ + KEY_WOW64_32KEY
                push 0
                push edi
                push [hDevicesKey]
                call [RegOpenKeyExW]
                test eax, eax
                jnz .devices_iter

                mov [dwInstanceIdx], eax
.insts_iter:    xor eax, eax
                push eax
                push eax
                push eax
                push eax
                lea eax, [dwInstanceLen]
                mov ecx, [dwEnumLen]
                add ecx, [dwDeviceLen]
                inc ecx
                mov dword [eax], WINBIO_MAX_STRING_LEN
                sub dword [eax], ecx
                push eax
                mov ax, '\'
                lea edi, [wszDeviceInstanceId + ecx*2]
                stosw
                push edi
                push [dwInstanceIdx]
                inc [dwInstanceIdx]
                push [hDeviceKey]
                call [RegEnumKeyExW]
                test eax, eax
                jnz .insts_end
                lea eax, [hInstanceKey]
                push eax
                push KEY_READ + KEY_WOW64_32KEY
                push 0
                push edi
                push [hDeviceKey]
                call [RegOpenKeyExW]
                test eax, eax
                jnz .devices_next

                lea eax, [hConfigurationsKey]
                push eax
                push KEY_READ + KEY_WOW64_32KEY
                push 0
                push database.szConfigsKey
                push [hInstanceKey]
                call [RegOpenKeyExA]
                test eax, eax
                jnz .insts_next

                ; Check configurations of device instance
.configs_loop:  mov [dwConfigurationsKeyWritable], eax
                mov [dwConfigurationIdx], eax
.configs_iter:  xor eax, eax
                push 32
                mov ecx, esp
                push eax
                push eax
                push eax
                push eax
                push ecx
                lea esi, [szConfigId]
                push esi
                push [dwConfigurationIdx]
                inc [dwConfigurationIdx]
                push [hConfigurationsKey]
                call [RegEnumKeyExA]
                test eax, eax
                pop ecx
                jnz .configs_end
                mov byte [esi+ecx], 0

                ; Is numeric?
                xor edx, edx
@@:             cmp byte [esi+edx], '0'
                jb .configs_iter
                cmp byte [esi+edx], '9'
                ja .configs_iter
                inc edx
                loop @b
                call atoi
                mov [dwConfigId], eax

                lea eax, [hConfigurationKey]
                push eax
                push KEY_READ + KEY_WOW64_32KEY
                push 0
                push esi
                push [hConfigurationsKey]
                call [RegOpenKeyExA]
                test eax, eax
                jnz .configs_iter

                ; Finally, call the callback
                push [pUser]
                push [dwConfigId]
                lea eax, [wszDeviceInstanceId]
                push eax
                push [hConfigurationKey]
                call [fnFilter]

                test eax, eax
                jns .no_delete
.delete:        pushf
                push [hConfigurationKey]
                call [RegCloseKey]
                call .delete_hkey
                popf
                jnp .cleanup
                test eax, eax
                jnz .configs_iter
                jmp .cleanup
.no_delete:     jz .configs_next
                mov eax, [hConfigurationKey]
                mov dword [esp], eax
                jmp .cleanup

.configs_next:  push [hConfigurationKey]
                call [RegCloseKey]
                jmp .configs_iter

.configs_end:   push [hConfigurationsKey]
                call [RegCloseKey]
                cmp [pwszDeviceInstanceId], 0
                jnz .exit

.insts_next:    push [hInstanceKey]
                call [RegCloseKey]
                jmp .insts_iter
.insts_end:
.devices_next:  push [hDeviceKey]
                call [RegCloseKey]
                jmp .devices_iter
.devices_end:   push [hDevicesKey]
                call [RegCloseKey]
                jmp .enum_iter

.cleanup:       push [hConfigurationsKey]
                call [RegCloseKey]
                cmp [pwszDeviceInstanceId], 0
                jnz .exit

                push [hInstanceKey]
                call [RegCloseKey]
                push [hDeviceKey]
                call [RegCloseKey]

                push [hDevicesKey]
                call [RegCloseKey]

.enum_end:      push [hEnumKey]
                call [RegCloseKey]

.exit:          pop eax
                pop edi
                pop esi
                ret

.delete_hkey:   cmp [dwConfigurationsKeyWritable], 0
                jnz .writable_hkey

                lea eax, [wszDeviceInstanceId]
                push eax
                push database.szSensorKeyFmt
                lea eax, [szKey]
                push eax
                call [wsprintfA]
                add esp, 0ch

                lea eax, [hConfigurationsKey]
                push eax
                push dword [eax]
                call [RegCloseKey]
                push KEY_READ + KEY_WOW64_32KEY + KEY_SET_VALUE
                push 0
                lea eax, [szKey]
                push eax
                push HKEY_LOCAL_MACHINE
                call [RegOpenKeyExA]
                test eax, eax
                jz .writable_hkey
                xor eax, eax
                retn

.writable_hkey: lea eax, [szConfigId]
                push eax
                push [hConfigurationsKey]
                call [RegDeleteKeyA]
                xor eax, eax
                inc eax
                mov [dwConfigurationsKeyWritable], eax
                retn
endp

proc InstallDatabaseForSensor guid:DWORD, wszDeviceInstanceId:DWORD
                local dwConfigId:DWORD
                local dwFound:DWORD
                local hSensorKey:DWORD
                local hDatabaseKey:DWORD
                local sensor:WBIOSVC_SENSOR
                local database:WBIOSVC_DATABASE
                local szSensorKey[65+WINBIO_MAX_STRING_LEN+1+255+1 + 2]:BYTE
                local szDatabaseKey[91 + 1 + 2]:BYTE

                push [guid]
                call IsDatabaseInstalled
                test eax, eax
                jz @f
                mov eax, WINBIO_E_DATABASE_ALREADY_EXISTS
                ret

@@:             mov [dwConfigId], eax
                mov [dwFound], eax

                ; Closures in x86 assembly, deal with it 8)
                push ebp ; pUser
                call .filter_end
;-------------------------------------------------------------------------;
                virtual at esp
                  .hDbKey  dd ?
                  .szDbKey db 91 + 1 + 2 dup (?)
                  .allocaSize = $ - $$
                  .edi     dd ?
                  .ebp     dd ?
                  .retAddr dd ?
                  .hKey    dd ?
                  .device  dd ?
                  .config  dd ?
                  .user    dd ?
                end virtual

                push ebp
                push edi
                sub esp, .allocaSize
                mov ebp, [.user]

                ; Detemine configuration key (max + 1)
                mov eax, [.config]
                cmp eax, [dwConfigId]
                jb @f
                inc eax
                mov [dwConfigId], eax
@@:
                ; Skip if suitable system sensor has been already found
                cmp [dwFound], 0
                jnz .exit

                ; We are interested only in system sensors ...
                mov ecx, [.hKey]
                push NULL
                mov eax, esp
                push 4
                push esp
                push eax
                push 0
                push RRF_RT_REG_DWORD
                push database.szSystemSensor
                push NULL
                push ecx
                call [RegGetValueA]
                test eax, eax
                pop ecx
                pop eax
                jnz .exit
                cmp ecx, 4
                jnz .exit
                test eax, eax
                jz .exit

                ; ... that have fingerprint database
                mov ecx, database.szDatabasesKey.strlen
                mov esi, database.szDatabasesKey
                lea edi, [.szDbKey]
                rep movsb
                mov ax, '\{'
                stosw
                lea eax, [edi-1]

                mov ecx, [.hKey]
                push 36+1
                push esp
                push edi
                push 0
                push RRF_RT_REG_SZ
                push database.szDatabaseId
                push NULL
                push ecx
                call [RegGetValueA]
                test eax, eax
                pop ecx
                jnz .exit
                cmp ecx, 36+1
                jnz .exit
                lea edi, [edi+ecx-1]
                mov al, '}'
                stosw

                lea eax, [.hDbKey]
                lea edi, [.szDbKey]
                push eax
                push KEY_READ + KEY_WOW64_32KEY
                push 0
                push edi
                push HKEY_LOCAL_MACHINE
                call [RegOpenKeyExA]
                test eax, eax
                jnz .exit

                mov ecx, [.hDbKey]
                push 0
                mov eax, esp
                push 4
                push esp
                push eax
                push 0
                push RRF_RT_REG_DWORD
                push database.szBiometricType
                push NULL
                push ecx
                call [RegGetValueA]
                test eax, eax
                pop ecx
                pop eax
                jnz .exit_cleanly
                cmp ecx, 4
                jnz .exit_cleanly
                test eax, WINBIO_TYPE_FINGERPRINT
                jz .exit_cleanly

                ; System fingerprint sensor found.
                ; Read sensor & database configuration keys.
                lea eax, [sensor]
                push eax
                push [.hKey + 4]
                call ReadWbiosvcSensorKey
                test eax, eax
                jz .exit_cleanly
                lea eax, [database]
                push eax
                push [.hDbKey + 4]
                call ReadWbiosvcDatabaseKey
                test eax, eax
                jz .exit_cleanly

                ; Done
                mov [dwFound], 1

.exit_cleanly:  push [.hDbKey]
                call [RegCloseKey]
.exit:          xor eax, eax
                add esp, .allocaSize
                pop edi
                pop ebp
                retn 10h
;-------------------------------------------------------------------------;
.filter_end:    push [wszDeviceInstanceId]
                call IterSensorConfigurations

                cmp [dwFound], 0
                jnz .system_sensor_found
                mov eax, WINBIO_E_INVALID_DEVICE_STATE
                ret

.system_sensor_found:
                ; Adjust database & sensor configs for our private datbaase
                xor eax, eax
                push ebx
                push esi
                push edi
                lea ebx, [database]
                lea edi, [sensor]
                mov [ebx + WBIOSVC_DATABASE.szConnectionString], al
                mov [ebx + WBIOSVC_DATABASE.szFilePath], al
                mov [edi + WBIOSVC_SENSOR.dwSystemSensor], eax
                inc eax
                mov [ebx + WBIOSVC_DATABASE.dwAutoCreate], eax
                mov [ebx + WBIOSVC_DATABASE.dwAutoName], eax
                mov al, 32
                mov [ebx + WBIOSVC_DATABASE.dwInitialSize], eax

                push 16/4
                pop ecx
                mov esi, [guid]
                lea edi, [edi + WBIOSVC_SENSOR.guidDatabaseId]
                rep movsd

.adjusted:      ; Write adjusted configs to the registry
                mov ecx, database.szDatabasesKey.strlen
                mov esi, database.szDatabasesKey
                lea edi, [szDatabaseKey]
                rep movsb
                mov al, '\'
                stosb
                push edi
                push [guid]
                call GuidToString

                lea edi, [szSensorKey]
                push [wszDeviceInstanceId]
                push database.szSensorKeyFmt
                push edi
                call [wsprintfA]
                add edi, eax
                mov al, '\'
                stosb
                push [dwConfigId]
                push database.szFormatD
                push edi
                call [wsprintfA]
                add esp, 0ch + 0ch

                push NULL
                lea eax, [hDatabaseKey]
                push eax
                push NULL
                push KEY_WRITE
                push 0
                push NULL
                push 0
                lea eax, [szDatabaseKey]
                push eax
                push HKEY_LOCAL_MACHINE
                call [RegCreateKeyExA]
                test eax, eax
                jnz .finish

                push NULL
                lea eax, [hSensorKey]
                push eax
                push NULL
                push KEY_WRITE
                push 0
                push NULL
                push 0
                lea eax, [szSensorKey]
                push eax
                push HKEY_LOCAL_MACHINE
                call [RegCreateKeyExA]
                test eax, eax
                jz @f
                push eax
                push [hDatabaseKey]
                call [RegCloseKey]
                lea eax, [szDatabaseKey]
                push eax
                push HKEY_LOCAL_MACHINE
                call [RegDeleteKeyA]
                pop eax
                jmp .finish
@@:
                lea eax, [database]
                push eax
                push [hDatabaseKey]
                call SaveWbiosvcDatabaseKey

                lea eax, [sensor]
                push eax
                push [hSensorKey]
                call SaveWbiosvcSensorKey

                push [hDatabaseKey]
                push [hSensorKey]
                call [RegCloseKey]
                call [RegCloseKey]

                ; WinBio service restart required to use the registered database
                call WbioSrvcRestart

                xor eax, eax
.finish:        pop edi
                pop esi
                pop ebx
                ret
endp

proc GetDatabaseSensorDeviceInstanceId guid:DWORD, wszDeviceInstanceId:DWORD
                mov eax, [wszDeviceInstanceId]
                mov word [eax], 0
                push ebp ; pUser
                call .filter_end
;-------------------------------------------------------------------------;
                virtual at esp
                  .guid    db 16 dup (?)
                  .szGuid  db 36+1 + 3 dup (?)
                  .allocaSize = $ - $$
                  .edi     dd ?
                  .esi     dd ?
                  .ebp     dd ?
                  .retAddr dd ?
                  .hKey    dd ?
                  .device  dd ?
                  .config  dd ?
                  .user    dd ?
                end virtual
                push ebp
                push esi
                push edi
                sub esp, .allocaSize
                mov ebp, [.user]

                lea eax, [.szGuid]
                mov ecx, [.hKey]
                push 36+1
                push esp
                push eax
                push 0
                push RRF_RT_REG_SZ
                push database.szDatabaseId
                push NULL
                push ecx
                call [RegGetValueA]
                test eax, eax
                pop ecx
                jnz .mismatch
                cmp ecx, 36+1
                jnz .mismatch

                lea ecx, [.szGuid]
                push esp ;.guid
                push ecx
                call StringToGuid
                test eax, eax
                jz .mismatch

                mov ecx, 16
                mov esi, esp ;lea esi, [.guid]
                mov edi, [guid]
                repz cmpsb
                jnz .mismatch
                mov esi, [.device]
                mov edi, [wszDeviceInstanceId]
@@:             lodsw
                stosw
                test ax, ax
                jnz @b

.match:         xor eax, eax
                inc eax
                jmp @f
.mismatch:      xor eax, eax
@@:             add esp, .allocaSize
                pop edi
                pop esi
                pop ebp
                retn 10h
;-------------------------------------------------------------------------;
.filter_end:    push NULL
                call IterSensorConfigurations
                test eax, eax
                jz .exit
                push eax
                call [RegCloseKey]
                push 1
                pop eax
.exit:          ret
endp

proc IsDatabaseInstalled guid:DWORD
                local wszDeviceInstanceId[WINBIO_MAX_STRING_LEN]:WORD
                lea eax, [wszDeviceInstanceId]
                push eax
                push [guid]
                call GetDatabaseSensorDeviceInstanceId
                ret
endp

proc InstallDatabase guid:DWORD
                local pUnits:DWORD
                local dwCount:DWORD

                push ebx
                push esi
                push edi

                push [guid]
                call IsDatabaseInstalled
                test eax, eax
                mov eax, WINBIO_E_DATABASE_ALREADY_EXISTS
                jnz .fail

.retry:         lea eax, [dwCount]
                push eax
                lea eax, [pUnits]
                push eax
                push WINBIO_TYPE_FINGERPRINT
                call [WinBioEnumBiometricUnits]
                cmp eax, RPC_S_SERVER_TOO_BUSY
                jz .retry
                test eax, eax
                jnz .fail

                push [dwCount]
                push [pUnits]
                call SensorSelectDialog
                test eax, eax
                jnz .got_sensor
                mov eax, WINBIO_E_CANCELED
                jmp .fail_and_free

.got_sensor:    push [eax + WINBIO_UNIT_SCHEMA.wszDeviceInstanceId]
                push [guid]
                call InstallDatabaseForSensor
                test eax, eax
                jnz .fail_and_free
.success:       xor eax, eax
.fail_and_free: push eax
                push [pUnits]
                call [WinBioFree]
                pop eax

.fail:          pop edi
                pop esi
                pop ebx
                ret
endp

proc UninstallDatabase guid:DWORD
                local szFilePath[MAX_PATH+4]:BYTE
                local szSubKey[40]:BYTE
                local szGUID[39+1]:BYTE
                local hKey:DWORD
                local hDatabasesKey:DWORD

                push esi
                push edi

.purge_sensor:  push [guid] ; user data
                call .filter_end
;-------------------------------------------------------------------------;
                virtual at esp
                  .guid    db 16 dup (?)
                  .szGuid  db 39+1 dup(?)
                  .allocaSize = $ - $$
                  .edi     dd ?
                  .esi     dd ?
                  .rc      dd ?
                  .retAddr dd ?
                  .hKey    dd ?
                  .device  dd ?
                  .config  dd ?
                  .user    dd ?
                end virtual

                xor eax, eax
                push eax
                push esi
                push edi
                sub esp, .allocaSize
                mov ecx, [.hKey]
                lea esi, [.szGuid]
                mov edi, [.user]

                push 40
                push esp
                push esi
                push eax
                push RRF_RT_REG_SZ
                push database.szDatabaseId
                push eax
                push ecx
                call [RegGetValueA]
                test eax, eax
                pop eax
                jnz .skip
                push esp;[.guid]
                push esi
                call StringToGuid
                test eax, eax
                mov esi, esp;lea edi, [.guid]
                jz .skip

                push 16
                pop ecx
                repz cmpsb
                jnz .skip
                dec [.rc]

.skip:          add esp, .allocaSize
                pop edi
                pop esi
                pop eax
                retn 10h
.filter_end:
;-------------------------------------------------------------------------;
                push NULL ; Iterate all device configurations
                call IterSensorConfigurations

.sensor_keys_purged:
                ; Delete from WbioSvc databases key
                lea eax, [hDatabasesKey]
                push eax
                push KEY_READ + KEY_WOW64_32KEY + KEY_SET_VALUE
                push 0
                push database.szDatabasesKey
                push HKEY_LOCAL_MACHINE
                call [RegOpenKeyExA]
                test eax, eax
                jnz .databases_key_purged

                lea esi, [szSubKey]
                push esi
                push [guid]
                call GuidToString

                lea eax, [hKey]
                push eax
                push KEY_READ + KEY_WOW64_32KEY
                push 0
                push esi
                push [hDatabasesKey]
                call [RegOpenKeyExA]
                test eax, eax
                jnz .close_databases_key

                push MAX_PATH+1
                push esp
                lea edi, [szFilePath]
                push edi
                push NULL
                push RRF_RT_REG_SZ
                push database.szFilePath
                push NULL
                push [hKey]
                call [RegGetValueA]
                test eax, eax
                pop eax
                jnz @f
                test eax, eax
                jz @f
                mov byte [edi+eax-1], 0

                push edi
                call [DeleteFileA]
                test eax, eax
                jnz @f

                call [GetLastError]
                sub eax, ERROR_PATH_NOT_FOUND
                jnz @f

                ; In 32-bit processes on 64-bit Windows, access to System32 is
                ; redirected to SysWOW64, which causes DeleteFileA() on WinBio
                ; databases to fail (the databases are contained in System32).
                ; Therefore, disable Wow64FsRedirection and retry the deletion.
                push eax
                push esp
                call [Wow64DisableWow64FsRedirection]

                push edi
                call [DeleteFileA]

                call [Wow64RevertWow64FsRedirection]

@@:             push [hKey]
                call [RegCloseKey]

                push esi
                push [hDatabasesKey]
                call [RegDeleteKeyA]

.close_databases_key:
                push [hDatabasesKey]
                call [RegCloseKey]

.databases_key_purged:
                call DeleteAppDataSettings

                xor eax, eax
                jmp @f
.fail:          xor eax, eax
                inc eax
@@:             pop edi
                pop esi
                ret
endp

;--------------------------------------------------------------------------;
; Auxiliary fastcall functions for manipulating Windows registry keys.
; ebx = hKey, esi = szValueName, edi = buffer, ecx = len (if applicable).
; Upon return, edi = edi + ecx, esi = esi + strlen(esi)+1 and CF=1 indicates
; an error.
;--------------------------------------------------------------------------;
Regaux:
.getGuid:       virtual at esp
                  .szGuid db 39 dup (?)
                  .pad0   db ?
                  .allocaSize = $ - $$
                  .ecx    dd ?
                end virtual
                push ecx
                sub esp, .allocaSize
                mov eax, esp;lea eax, [.szGuid]
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
                clc
                jnz .getGuid.error
@@:             mov eax, esp ;lea eax, [.szGuid]
                push edi
                add edi, 16
                push eax
                call StringToGuid

                test eax, eax
                jz .getGuid.error
@@:             lodsb
                test al, al
                jnz @b
                stc
.getGuid.error: cmc
.getGuid.done:  lea esp, [esp + .allocaSize]
                jnc @f
                pop dword [esp]
@@:             pop ecx
                retn

;--------------------------------------------------------------------------;

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
                pop ecx

                test eax, eax
                jnz .getSz.error
@@:             lodsb
                test al, al
                jnz @b
                stc
.getSz.error:   cmc
.getSz.done:    jnc @f
                pop dword [esp]
@@:             pop ecx
                retn

;--------------------------------------------------------------------------;

.getDword:      push ecx

                push 4
                push esp
                push edi
                add edi, 4
                push 0
                push RRF_RT_REG_DWORD
                push esi
                push NULL
                push ebx
                call [RegGetValueA]
                pop ecx

                test eax, eax
                jnz .getDword.err
@@:             lodsb
                test al, al
                jnz @b
                stc
.getDword.err:  cmc
.getDword.done: jnc @f
                pop dword [esp]
@@:             pop ecx
                retn

;--------------------------------------------------------------------------;

.setGuid39:     push 1  ; with braces
                jmp @f
.setGuid:       push 0  ; without braces
@@:             virtual at esp
                  .szGUID db 39 dup (?)
                  .pad1   db ?
                  .allocaSize = $ - $$
                  .braces dd ?
                  .esp    dd ?
                end virtual
                push dword [esp]
                mov [esp+4], ecx
                sub esp, .allocaSize

                push esp
                push edi
                add edi, 16
                call GuidToString

                mov eax, [.braces]
                lea ecx, [36+1+2*eax]
                xor eax, 1
                lea edx, [esp+eax] ;lea edx, [.szGUID]
                mov byte [edx+ecx-1], 0

                push ecx
                push edx
                push REG_SZ
                push 0
                push esi
                push ebx
                call [RegSetValueExA]

                test eax, eax
                jnz .setGuid.error
@@:             lodsb
                test al, al
                jnz @b
                stc
.setGuid.error: cmc
.setGuid.done:  lea esp, [esp + .allocaSize + 4]
                jnc @f
                pop dword [esp]
@@:             pop ecx
                retn

;--------------------------------------------------------------------------;

.setSz:         push ecx
                xor eax, eax
                xchg eax, ecx           ; store max length to eax
@@:             cmp byte [edi+ecx], 0   ; calculate real length
                lea ecx, [ecx+1]
                jnz @b

                push ecx
                push ecx
                push edi
                add edi, eax
                push REG_SZ
                push 0
                push esi
                push ebx
                call [RegSetValueExA]
                pop ecx

                test eax, eax
                jnz .setSz.error
@@:             lodsb
                test al, al
                jnz @b
                stc
.setSz.error:   cmc
.setSz.done:    jnc @f
                pop dword [esp]
@@:             pop ecx
                retn

;--------------------------------------------------------------------------;

.setDword:      push ecx
                push 4
                push edi
                add edi, 4
                push REG_DWORD
                push 0
                push esi
                push ebx
                call [RegSetValueExA]

                test eax, eax
                jnz .setDword.err
@@:             lodsb
                test al, al
                jnz @b
                stc
.setDword.err:  cmc
.setDword.done: jnc @f
                pop dword [esp]
@@:             pop ecx
                retn
