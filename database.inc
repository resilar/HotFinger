;-------------------------------------------------------------------------;
; WinBio database management functions
;
; The code is shitty. The management of WinBio sensor configurations and
; fingerprint databases should be decoupled properly. Moreover, the code
; currently enumerates biometric units & databases using WinBio API, which
; is incorrect in some situations (e.g., uninstalling database of detached
; sensor does not clear the sensor configuration in registry). Enumeration
; using registry instead of WinBio API would be less inane.
;-------------------------------------------------------------------------;

KEY_WOW64_64KEY = 0100h
KEY_WOW64_32KEY = 0200h

RRF_RT_REG_SZ    = 02h
RRF_RT_REG_DWORD = 10h

ERROR_ACCESS_DENIED = 5

RPC_S_SERVER_TOO_BUSY = 000006BBh
RPC_X_SS_IN_NULL_CONTEXT = 000006EFh

struct WBIOSVC_SENSOR
  guidDatabaseId         db 16 dup (?)
  dwSensorMode           dd ?
  dwSystemSensor         dd ?
  szEngineAdapterBinary  db MAX_PATH dup (?)
  szSensorAdapterBinary  db MAX_PATH dup (?)
  szStorageAdapterBinary db MAX_PATH dup (?)

  ; non-registry fields
  wszDeviceInstanceId    du WINBIO_MAX_STRING_LEN dup (?)
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

  ; non-registry fields
  guidDatabaseId    db 16 dup (?)
ends

; pSensor:WBIOSVC_SENSOR
proc ReadWbiosvcSensorKey hKey:DWORD, pSensor:DWORD
                push ebx
                push esi
                push edi

                mov ebx, [hKey]
                mov edi, [pSensor]
                ;add edi, WBIOSVC_SENSOR.guidDatabaseId ; offset 0
                mov esi, database.szDatabaseId
                call Regaux.getGuid
                jc .error

                mov esi, database.szSensorMode
                call Regaux.getDword
                jc .error

                mov esi, database.szSystemSensor
                call Regaux.getDword
                jc .error

                mov esi, database.szEngineAdapterBinary
                mov ecx, MAX_PATH
                call Regaux.getSz
                jc .error

                mov esi, database.szSensorAdapterBinary
                call Regaux.getSz
                jc .error

                mov esi, database.szStorageAdapterBinary
                call Regaux.getSz
                jc .error

                xor eax, eax
                inc eax
.finish:        pop edi
                pop esi
                pop ebx
                ret
.error:         xor eax, eax
                jmp .finish
endp

; pDatabase:WBIOSVC_DATABASE
proc ReadWbiosvcDatabaseKey hKey:DWORD, pDatabase:DWORD
                push ebx
                push esi
                push edi

                mov ebx, [hKey]
                mov edi, [pDatabase]
                ;add edi, WBIOSVC_DATABASE.dwAttributes ; offset 0
                mov esi, database.szAttributes
                call Regaux.getDword
                jc .error

                mov esi, database.szAutoCreate
                call Regaux.getDword
                jc .error

                mov esi, database.szAutoName
                call Regaux.getDword
                jc .error

                mov esi, database.szBiometricType
                call Regaux.getDword
                jc .error

                mov esi, database.szConnectionString
                mov ecx, WINBIO_MAX_STRING_LEN
                call Regaux.getSz
                jc .error

                mov esi, database.szFilePath
                mov ecx, MAX_PATH
                call Regaux.getSz
                jc .error

                mov esi, database.szFormat
                call Regaux.getGuid
                jc .error

                mov esi, database.szInitialSize
                call Regaux.getDword
                jc .error

                xor eax, eax
                inc eax
.finish:        pop edi
                pop esi
                pop ebx
                ret
.error:         xor eax, eax
                jmp .finish
endp

; guidDatabaseId[16]:BYTE
; fnFilter = proc hKey:DWORD, guidDatabaseId:DWORD, pUser:DWORD
proc IterSensorKeys wszDeviceInstanceId:DWORD, \
                    fnFilter:DWORD, pUser:DWORD, \
                    bCreate:DWORD, bDelete:DWORD
                local szKey[65+WINBIO_MAX_STRING_LEN+1+255+1]:BYTE
                local pad0[2]:BYTE
                local buffer[39]:BYTE
                local pad1[1]:BYTE
                local dwNewMax:DWORD
                local hParentKey:DWORD
                local hKey:DWORD

                push ebx
                push esi
                push edi
                xor eax, eax
                mov [hKey], eax
                mov [dwNewMax], eax

                lea esi, [szKey]
                lea edi, [buffer]
                push [wszDeviceInstanceId]
                push database.szSensorKeyFmt
                push esi
                call [wsprintfA]
                add esp, 0ch

                lea edx, [hParentKey]
                push edx
                push KEY_READ + KEY_WOW64_32KEY
                cmp [bDelete], 0
                jz @f
                or dword [esp], KEY_SET_VALUE
@@:             push 0
                push esi
                add esi, eax
                push HKEY_LOCAL_MACHINE
                call [RegOpenKeyExA]
                test eax, eax
                jnz .exit

                mov byte [esi], '\'
                inc esi
                xor ebx, ebx    ; subkey index
.enumerate:     xor eax, eax
                push 256
                mov edx, esp
                push eax
                push eax
                push eax
                push eax
                push edx
                push esi
                push ebx
                push [hParentKey]
                call [RegEnumKeyExA]
                inc ebx
                pop ecx
                test eax, eax
                jnz .not_found

                ; Is numeric?
                xor edx, edx
@@:             cmp byte [esi+edx], '0'
                jb .enumerate
                cmp byte [esi+edx], '9'
                ja .enumerate
                inc edx
                loop @b
.numeric:
                call atoi
                cmp eax, [dwNewMax]
                jb @f
                inc eax
                mov [dwNewMax], eax
@@:
                lea edx, [hKey]
                push edx
                push KEY_READ + KEY_WOW64_32KEY
                push 0
                push esi
                push [hParentKey]
                call [RegOpenKeyExA]
                test eax, eax
                jnz .enumerate

                push 39
                push esp
                push edi
                push NULL
                push RRF_RT_REG_SZ
                push database.szDatabaseId
                push NULL
                push [hKey]
                call [RegGetValueA]
                test eax, eax
                pop eax
                jnz .bad_key
                cmp al, 37
                jz @f
                cmp al, 39
                jnz .bad_key
@@:
                ; Save database GUID to pDatabase if given
                push edi
                push edi
                call StringToGuid

                pushad
                push [pUser]
                push edi
                push [hKey]
                mov eax, [fnFilter]
                call eax
                test eax, eax
                popad
                jnz .match

.bad_key:       push [hKey]
                call [RegCloseKey]
                jmp .enumerate

.match:         cmp [bDelete], 0
                jz @f

                push [hKey]
                call [RegCloseKey]
                push esi
                push [hParentKey]
                call [RegDeleteKeyA]
                jmp .enumerate

@@:             cmp [bCreate], 0
                jz .close
.collision:     push 0
                push [hKey]
                call [RegCloseKey]
                pop [hKey]
                jmp .close

.not_found:     xor eax, eax
                mov [hKey], eax
                cmp [bCreate], eax
                jz .close

                lea edi, [szKey]
                push [dwNewMax]
                push database.szFormatD
                push edi
                call [wsprintfA]
                add esp, 0ch

                mov eax, [hParentKey]
                push NULL
                lea ecx, [hKey]
                push ecx
                push NULL
                push KEY_WRITE
                push 0
                push NULL
                push 0
                lea edi, [szKey]
                push edi
                push eax
                call [RegCreateKeyExA]

.close:         push [hParentKey]
                call [RegCloseKey]
.exit:          mov eax, [hKey]
                pop edi
                pop esi
                pop ebx
                ret
endp

;-------------------------------------------------------------------------;
; Read *system* sensor registry settings of the specified device instance.
; Output: pSensor:WBIOSVC_SENSOR, pDatabase:WBIOSVC_DATABASE
;-------------------------------------------------------------------------;
proc ReadWbiosvcSettings wszDeviceInstanceId:DWORD, pSensor:DWORD, pDatabase:DWORD
                local szKey[65+WINBIO_MAX_STRING_LEN+1+255+1]:BYTE
                local pad0[2]:BYTE
                local buffer[39]:BYTE
                local pad1[1]:BYTE
                local pStorages:DWORD
                local dwCount:DWORD
                local dwIndex:DWORD
                local hParentKey:DWORD
                local hKey:DWORD

                push ebx
                push esi
                push edi
                push 1 ; return value

.retry:         lea ecx, [dwCount]
                lea eax, [pStorages]
                push ecx
                push eax
                push WINBIO_TYPE_FINGERPRINT
                call [WinBioEnumDatabases]
                cmp eax, RPC_S_SERVER_TOO_BUSY
                jz .retry
                test eax, eax
                jnz .exit

                ; Closures in x86 assembly, deal with it 8)
                push FALSE
                push FALSE
                push ebp ; user data
                call .filter_end
;-------------------------------------------------------------------------;
                virtual at esp
                  .ebp     dd ?
                  .retAddr dd ?
                  .hKey    dd ?
                  .guidDB  dd ?
                  .user    dd ?
                end virtual
                push ebp
                mov ebp, [.user]

                ; We need a system sensor ...
                lea edi, [buffer]
                mov eax, [.hKey]
                push 4
                push esp
                push edi
                push 0
                push RRF_RT_REG_DWORD
                push database.szSystemSensor
                push NULL
                push eax
                call [RegGetValueA]
                pop ecx
                test eax, eax
                jnz .bad_key
                scasd
                jz .bad_key

                ; ... that supports fingerprint storage
                mov [dwIndex], eax
                mov eax, [pStorages]
                mov edx, [dwCount]
.next_storage:  cmp [dwIndex], edx
                jz .bad_key

                mov cl, 16
                lea esi, [eax + WINBIO_STORAGE_SCHEMA.guidDatabaseId]
                mov edi, [.guidDB]
                repz cmpsb
                jz .match
                lea eax, [eax + sizeof.WINBIO_STORAGE_SCHEMA]
                inc [dwIndex]
                jmp .next_storage

.bad_key:       xor eax, eax
.match:         pop ebp
                retn 0ch
.filter_end:
;-------------------------------------------------------------------------;
                push [wszDeviceInstanceId]
                call IterSensorKeys
                test eax, eax
                jz .fail
                mov [hKey], eax
                imul edi, [dwIndex], sizeof.WINBIO_STORAGE_SCHEMA
                add edi, [pStorages]

                ; Read wbiosvc sensor device configuration
                xor ebx, ebx
                mov esi, [pSensor]
                test esi, esi
                push eax ; for RegCloseKey (either branch)
                jnz .read_sensor
                call [RegCloseKey]
                jmp .read_database
.read_sensor:   push esi
                push eax
                call ReadWbiosvcSensorKey
                mov ebx, eax
                call [RegCloseKey]
                dec ebx
                jnz .finish
                push edi
                lea edi, [esi + WBIOSVC_SENSOR.wszDeviceInstanceId]
                mov esi, [wszDeviceInstanceId]
@@:             cmp [esi], bx
                movsw
                jnz @b
                pop edi

                ; Read wbiosvc database configuration
.read_database: add ebx, [pDatabase]
                jz .finish

                lea eax, [szKey]
                lea esi, [edi + WINBIO_STORAGE_SCHEMA.guidDatabaseId]
                lea edi, [ebx + WBIOSVC_DATABASE.guidDatabaseId]
                push eax
                push edi
                movsd ; copy
                movsd ; a
                movsd ; 16-byte
                movsd ; GUID
                call DatabaseKey

                lea esi, [hKey]
                push esi
                push KEY_READ + KEY_WOW64_32KEY
                push 0
                push eax
                push HKEY_LOCAL_MACHINE
                call [RegOpenKeyExA]
                test eax, eax
                jnz .finish

                push ebx
                push dword [esi]
                call ReadWbiosvcDatabaseKey
                dec eax
                pushfd
                push dword [esi]
                call [RegCloseKey]
                popfd

.finish:        ; Adjust the return value based on ZF
                pop eax
                setnz al
                push eax

.fail:          push [pStorages]
                call [WinBioFree]
.exit:          pop eax
                pop edi
                pop esi
                pop ebx
                ret
endp

;-------------------------------------------------------------------------;
; Find non-system sensor for given device and database.
; Output: wszDeviceInstanceId[256]:WORD guidDatabaseId[16]:BYTE
; Output: hKey (eax)
;-------------------------------------------------------------------------;
proc FindPrivateSensorKeyForDatabase wszDeviceInstanceId:DWORD, guidDatabaseId:DWORD
                local dwBuffer:DWORD
                local pStorages:DWORD
                local dwCount:DWORD

                push esi
                push edi

.retry:         lea ecx, [dwCount]
                lea eax, [pStorages]
                push ecx
                push eax
                push WINBIO_TYPE_FINGERPRINT
                call [WinBioEnumDatabases]
                cmp eax, RPC_S_SERVER_TOO_BUSY
                jz .retry
                test eax, eax
                xor eax, eax
                jnz .exit

                push FALSE
                push FALSE
                push ebp ; user data
                call .filter_end
;-------------------------------------------------------------------------;
                virtual at esp
                  .ebp     dd ?
                  .retAddr dd ?
                  .hKey    dd ?
                  .guidDB  dd ?
                  .user    dd ?
                end virtual
                push ebp
                mov ebp, [.user]

                ; We need a non-system sensor ...
                lea edi, [dwBuffer]
                mov eax, [.hKey]
                push 4
                push esp
                push edi
                push 0
                push RRF_RT_REG_DWORD
                push database.szSystemSensor
                push NULL
                push eax
                call [RegGetValueA]
                test eax, eax
                pop ecx
                jnz .bad_key
                scasd
                jnz .bad_key

                ; ... for our database
                mov eax, [pStorages]
                mov edx, [dwCount]
.next_storage:  dec edx
                js .bad_key

                mov cl, 16
                lea esi, [eax + WINBIO_STORAGE_SCHEMA.guidDatabaseId]
                mov edi, [.guidDB]
                push edi
                push [guidDatabaseId]
                push ecx
                repz cmpsb
                pop ecx
                pop esi
                pop edi
                jnz .mismatch

                repz cmpsb
                jz .match

.mismatch:      lea eax, [eax + sizeof.WINBIO_STORAGE_SCHEMA]
                jmp .next_storage

.bad_key:       xor eax, eax
.match:         pop ebp
                retn 0ch
.filter_end:
;-------------------------------------------------------------------------;
                push [wszDeviceInstanceId]
                call IterSensorKeys

                push eax
                push [pStorages]
                call [WinBioFree]
                pop eax

.exit:          pop edi
                pop esi
                ret
endp

; pSensor:WBIOSVC_SENSOR
proc SaveWbiosvcSensorKey hKey:DWORD, pSensor:DWORD
                push ebx
                push esi
                push edi
                mov ebx, [hKey]
                mov edi, [pSensor]

                mov esi, database.szDatabaseId
                call Regaux.setGuid
                jc .error

                mov esi, database.szSensorMode
                call Regaux.setDword
                jc .error

                mov esi, database.szSystemSensor
                call Regaux.setDword
                jc .error

                mov esi, database.szEngineAdapterBinary
                mov ecx, MAX_PATH
                call Regaux.setSz
                jc .error

                mov esi, database.szSensorAdapterBinary
                call Regaux.setSz
                jc .error

                mov esi, database.szStorageAdapterBinary
                call Regaux.setSz
                jc .error

                xor eax, eax
.finish:        pop edi
                pop esi
                pop ebx
                ret
.error:         xor eax, eax
                inc eax
                jmp .finish
endp

; pDatabase:WBIOSVC_DATABASE
proc SaveWbiosvcDatabaseKey hKey:DWORD, pDatabase:DWORD
                push ebx
                push esi
                push edi
                push ebp
                mov ebx, [hKey]
                mov edi, [pDatabase]
                mov ebp, Regaux.setDword

                ;add edi, WBIOSVC_DATABASE.dwAttributes ; offset 0
                mov esi, database.szAttributes
                call ebp
                jc .error

                mov esi, database.szAutoCreate
                call ebp
                jc .error

                mov esi, database.szAutoName
                call ebp
                jc .error

                mov esi, database.szBiometricType
                call ebp
                jc .error

                mov esi, database.szConnectionString
                mov ecx, WINBIO_MAX_STRING_LEN
                call Regaux.setSz
                jc .error

                mov esi, database.szFilePath
                mov ecx, MAX_PATH
                call Regaux.setSz
                jc .error

                mov esi, database.szFormat
                call Regaux.setGuid
                jc .error

                mov esi, database.szInitialSize
                call ebp
                jc .error

                xor eax, eax
.finish:        pop ebp
                pop edi
                pop esi
                pop ebx
                ret
.error:         push 2
                pop eax
                jmp .finish
endp

;-------------------------------------------------------------------------;
; Save WinBio service registry settings.
; pSensor:WBIOSVC_SENSOR, pDatabase:WBIOSVC_DATABASE
;-------------------------------------------------------------------------;
proc SaveWbiosvcSettings pSensor:DWORD, pDatabase:DWORD
                local szKey[65+WINBIO_MAX_STRING_LEN+1+255+1]:BYTE
                local pad0[2]:BYTE
                local hParentKey:DWORD
                local hKey:DWORD

                xor eax, eax
                push esi
                push edi
                push eax ; return value (= 0)
                mov [hParentKey], eax

.save_database: mov edi, [pDatabase]
                test edi, edi
                jz .save_sensor

                lea esi, [hParentKey]
                push esi
                push KEY_WRITE
                push 0
                push database.szDatabasesKey
                push HKEY_LOCAL_MACHINE
                call [RegOpenKeyExA]
                cmp eax, ERROR_ACCESS_DENIED
                jz .access_denied
                test eax, eax
                jnz .save_sensor
                mov esi, [esi]

                push 1
                push esp
                lea eax, [hKey]
                push eax
                push NULL
                push KEY_WRITE
                push 0
                push NULL
                push 0
                lea edx, [szKey]
                push edx
                push esi

                lea ecx, [edi + WBIOSVC_DATABASE.guidDatabaseId]
                push edx
                push ecx
                call GuidToString

                call [RegCreateKeyExA]
                push esi
                mov esi, eax
                call [RegCloseKey]
                test esi, esi
                pop eax
                jnz .access_denied
                dec eax                 ; REG_CREATED_NEW_KEY (01h)
                jz .key_created
                dec eax                 ; REG_OPENED_EXISTING_KEY (02h)
                jz .existing_key
                jmp .access_denied      ; Unknown disposition

.key_created:   mov eax, [hKey]
                push eax ; RegCloseKey
                push edi
                push eax
                call SaveWbiosvcDatabaseKey
                mov [esp+4], eax
                call [RegCloseKey]
                cmp byte [esp], 0
                jnz .exit

.save_sensor:   mov edi, [pSensor]
                test edi, edi
                jz .exit

                lea eax, [edi + WBIOSVC_SENSOR.guidDatabaseId]
                push FALSE
                push TRUE
                push eax
                call .filter_end
;-------------------------------------------------------------------------;
                virtual at esp
                  .retAddr dd ?
                  .hKey    dd ?
                  .guidDB  dd ?
                  .user    dd ?
                end virtual
                push 16
                pop ecx
                mov esi, [.user]
                mov edi, [.guidDB]
                xor eax, eax
                repz cmpsb
                jnz @f
                inc eax
@@:             retn 0ch
.filter_end:
;-------------------------------------------------------------------------;
                lea eax, [edi + WBIOSVC_SENSOR.wszDeviceInstanceId]
                push eax
                call IterSensorKeys
                test eax, eax
                jz .sensor_error
                push eax ; RegCloseKey

                push edi
                push eax
                call SaveWbiosvcSensorKey
                or [esp+4], eax
                call [RegCloseKey]

.exit:          pop eax
                pop edi
                pop esi
                ret

.access_denied: mov eax, database.szRegistryAccessDenied
                jmp .error_message
.sensor_error:  mov eax, database.szSensorError
                jmp .error_message
.existing_key:  mov eax, database.szExistingDatabaseKey
.error_message: push MB_ICONEXCLAMATION
                push gui.szError
                push eax
                push 0
                call [MessageBoxA]
                mov byte [esp], 1
                jmp .exit
endp

; out[92]:BYTE
proc DatabaseKey guid:DWORD, szOut:DWORD
                push esi
                push edi
                mov edi, [szOut]
                mov esi, database.szDatabasesKey
                push edi ; return value
@@:             lodsb
                test al, al
                jz @f
                stosb
                jmp @b
@@:             mov al, '\'
                stosb
                push edi
                push [guid]
                call GuidToString
                pop eax
                pop edi
                pop esi
                ret
endp

proc IsDatabaseInstalled guid:DWORD
                local pStorages:DWORD
                local dwCount:DWORD
                push ebx
                push esi
                push edi

                xor ebx, ebx
.retry:         lea eax, [dwCount]
                push eax
                lea esi, [pStorages]
                push esi
                push WINBIO_TYPE_FINGERPRINT
                call [WinBioEnumDatabases]
                cmp eax, RPC_S_SERVER_TOO_BUSY
                jz .retry
                test eax, eax
                jnz .fail

                lodsd
                push eax        ; for [WinBioFree]
.next:          dec [dwCount]
                js .fail_cleanly
                push 16
                mov esi, [guid]
                lea edi, [eax + WINBIO_STORAGE_SCHEMA.guidDatabaseId]
                add eax, sizeof.WINBIO_STORAGE_SCHEMA
                pop ecx
                repz cmpsb
                jnz .next

.match:         inc ebx
.fail_cleanly:  call [WinBioFree]
.fail:          mov eax, ebx
                pop edi
                pop esi
                pop ebx
                ret
endp

proc InstallDatabaseForSensor guid:DWORD, wszDeviceInstanceId:DWORD
                local sensorConfig:WBIOSVC_SENSOR
                local databaseConfig:WBIOSVC_DATABASE

                push ebx
                push esi
                push edi

                lea ebx, [databaseConfig]
                lea edi, [sensorConfig]
                mov eax, [wszDeviceInstanceId]
                push ebx
                push edi
                push eax
                call ReadWbiosvcSettings
                test eax, eax
                jz .save_database
                mov eax, WINBIO_E_INVALID_UNIT
                jmp .fail

.save_database: mov [ebx + WBIOSVC_DATABASE.szConnectionString], al
                mov [ebx + WBIOSVC_DATABASE.szFilePath], al
                mov [edi + WBIOSVC_SENSOR.dwSystemSensor], eax
                inc eax
                mov [ebx + WBIOSVC_DATABASE.dwAutoCreate], eax
                mov [ebx + WBIOSVC_DATABASE.dwAutoName], eax
                mov al, 32
                mov [ebx + WBIOSVC_DATABASE.dwInitialSize], eax
                push ebx ; for SaveWbiosvcSettings
                push edi

                xor ecx, ecx
                mov esi, [guid]
@@:             lodsb
                mov [ebx + WBIOSVC_DATABASE.guidDatabaseId + ecx], al
                mov [edi + WBIOSVC_SENSOR.guidDatabaseId + ecx], al
                inc ecx
                cmp cl, 16
                jb @b

                call SaveWbiosvcSettings

.fail:          pop edi
                pop esi
                pop ebx
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
                push [guid]
                call SensorInstallDialog
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

proc UninstallDatabase guid:DWORD, wszDeviceInstanceId:DWORD
                local szFilePath[MAX_PATH+4]:BYTE
                local szSubKey[40]:BYTE
                local pUnits:DWORD
                local dwCount:DWORD
                local hKey:DWORD
                local hDatabasesKey:DWORD

                push esi
                push edi

                mov eax, [wszDeviceInstanceId]
                test eax, eax
                jz .enum
                mov [dwCount], 0
                jmp .purge_sensor
.retry:
.enum:          lea eax, [dwCount]
                push eax
                lea eax, [pUnits]
                push eax
                push WINBIO_TYPE_FINGERPRINT
                call [WinBioEnumBiometricUnits]
                cmp eax, RPC_S_SERVER_TOO_BUSY
                jz .retry
                test eax, eax
                jnz .fail

                mov edi, [pUnits]
.iter:          dec [dwCount]
                js .sensor_keys_purged
                lea eax, [edi + WINBIO_UNIT_SCHEMA.wszDeviceInstanceId]
                add edi, sizeof.WINBIO_UNIT_SCHEMA

.purge_sensor:  push TRUE
                push FALSE
                push [guid] ; user data
                call .filter_end
;-------------------------------------------------------------------------;
                virtual at esp
                  .edi     dd ?
                  .esi     dd ?
                  .rc      dd ?
                  .retAddr dd ?
                  .hKey    dd ?
                  .guidDB  dd ?
                  .user    dd ?
                end virtual
                push 0
                push esi
                push edi
                mov esi, [.user]
                mov edi, [.guidDB]
                push 16
                pop ecx
                repz cmpsb
                jnz .mismatch
                or dword [.rc], 1
.mismatch:      pop edi
                pop esi
                pop eax
                retn 0ch
.filter_end:
;-------------------------------------------------------------------------;
                push eax
                call IterSensorKeys
                jmp .iter

.sensor_keys_purged:
                cmp [pUnits], 0
                jz @f
                push [pUnits]
                call [WinBioFree]
@@:
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
                mov byte [edi+eax], 0

                push edi
                call [DeleteFileA]

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