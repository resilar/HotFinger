align 16 ; chosen by fair dice roll. guaranteed to be random.
guidDatabase db 00h,11h,22h,33h,44h,55h,66h,77h,88h,99h,0aah,0bbh,0cch,0ddh,0eeh,0ffh

;-------------------------------------------------------------------------;
; main.asm
;-------------------------------------------------------------------------;
hotfinger:
.szSensorUnavailable     db 'Sensor unavailable.',0
.szEnumSensorUnavailable db 'Sensor enumeration unavailable.',0

;-------------------------------------------------------------------------;
; cli.inc
;-------------------------------------------------------------------------;
clidata:
align 2
.wszUninstall du 'un'
.wszInstall   du 'install',0
.wszInstall.strlen = ($-.wszInstall)/2 - 1

;-------------------------------------------------------------------------;
; database.inc
;-------------------------------------------------------------------------;
database:
.szFormatD db '%d',0

.szDatabasesKey db 'SYSTEM\CurrentControlSet\Services\WbioSrvc\Databases',0
.szSensorKeyFmt db 'SYSTEM\CurrentControlSet\Enum\'
                db '%ls'
                db '\Device Parameters\WinBio\Configurations',0

; WinBio device configuration registry values
.szDatabaseId           db 'DatabaseId',0
.szSensorMode           db 'SensorMode',0
.szSystemSensor         db 'SystemSensor',0
.szEngineAdapterBinary  db 'EngineAdapterBinary',0
.szSensorAdapterBinary  db 'SensorAdapterBinary',0
.szStorageAdapterBinary db 'StorageAdapterBinary',0

; WinBio database configuration registry values
.szAttributes       db 'Attributes',0
.szAutoCreate       db 'AutoCreate',0
.szAutoName         db 'AutoName',0
.szBiometricType    db 'BiometricType',0
.szConnectionString db 'ConnectionString',0
.szFilePath         db 'FilePath',0
.szFormat           db 'Format',0
.szInitialSize      db 'InitialSize',0

; Messages
.szSensorError          db 'Sensor registration failed.',0
.szRegistryAccessDenied db 'Cannot write to HKEY_LOCAL_MACHINE.',0dh,0ah
                        db 'Try running the program as administrator.',0
.szExistingDatabaseKey  db 'Database registry key already exists.',0

;-------------------------------------------------------------------------;
; gui.inc
;-------------------------------------------------------------------------;
gui:
.szClassName     db 'hotfingerClass',0
.szEmpty = $-1
.szCaption       db 'HotFinger',0

.szButtonClass   db 'BUTTON',0
.szEditClass     db 'EDIT',0
.szStaticClass   db 'STATIC',0
.szListViewClass db 'SysListView32',0
.szStatusClass   db 'msctls_statusbar32',0

.szOptions       db '&Options',0
.szPreferences   db '&Preferences',0
.szUninstall     db '&Uninstall',0
.szQuit          db '&Quit',0

.szHelp          db '&Help',0
.szGitHub        db '&GitHub',0
.szAbout         db '&About',0

.szGitHubURL     db 'https://github.com/resilar/HotFinger',0
.szOpen          db 'open',0
.wszUninstall = clidata.wszUninstall

.szFinger        db 'Finger',0
.szCommand       db 'Command',0

align 2
.wszPathCue      du 'Command to execute',0
.szBrowse        db 'Browse',0
.szEnroll        db '&Enroll',0
.szError         db 'Error',0

.szBrowseFilter  db 'Executables (*.exe)',0, '*.exe',0
                 db 'All Files (*.*)',0, '*.*',0
                 db 0

.szWindowRegistrationFailed db 'Window registration failed.',0
.szWindowCreationFailed     db 'Window creation failed.',0
.szWindowContextMissing     db 'Window context lost.',0

.szOutOfMemoryError db 'Out of memory.',0
.szSessionError     db 'Error opening WinBio session.',0
.szIdentifyError    db 'Identify error (0x%08X)',0dh,0ah,0dh,0ah
                    db 'Restart the program',0
.szUninstallError   db 'Uninstallation failed',0

.szStatusDefault = $-1 ; Empty string
.szStatusEnrolled    db 'Enrolled',0
.szStatusEnrolling   db 'Enrolling ...',0
.szStatusError       db 'Error (0x%08X)',0
.szStatusErrorText   db 'Error %s (0x%08X)',0
.szStatusCommitting  db 'Commiting ...',0
.szStatusDiscarding  db 'Discarding ...',0
.szStatusCommitted   db 'Committed',0
.szStatusDiscarded   db 'Discarded',0
.szStatusIdentifying db 'Identifying',0

.bEnrollCounter db 0

;-------------------------------------------------------------------------;
; sensor.inc
;-------------------------------------------------------------------------;
sensor:
.szInstallModeTitle db 'Install HotFinger database for the selected sensor',0
.szActivationText   db 'Select sensor from the list or by activating it',0
.szInstall          db 'Install',0
.wszInstall = clidata.wszInstall
.wszInstall.strlen = clidata.wszInstall.strlen

.szHeaderTextArray:
  db 'Unit',0
  db 'Device instance ID',0
  db 'Description',0
  db 'Manufacturer',0
  db 'Model',0
  db 'Serial no',0
  db 'Firmware',0
  db 'Pool type',0
  db 'Biometric factor',0
  db 'Sensor subtype',0
  db 'Sensor capabilities',0
  db 0

.szInstallError db 'Error installing database for the sensor',0
.szError        = gui.szError
.szSessionError = gui.szSessionError

;-------------------------------------------------------------------------;
; settings.inc
;-------------------------------------------------------------------------;
settings:
.szDirectory db 'HotFinger'
.szDirectory.strlen = $-.szDirectory
.szFilename  db 'settings.ini',0
.szFilename.size = $-.szFilename
.RelativeSettingsPathLen = .szDirectory.strlen + 1 + .szFilename.size
.szFormatD = database.szFormatD

.szSectionGUI      db 'GUI',0
.szSectionDevice   db 'Device',0
.szSectionCommand  db 'Command',0

.szKeyX  db 'x',0
.szKeyY  db 'y',0
.szKeyCX db 'cx',0
.szKeyCY db 'cy',0

.szInstanceId db 'InstanceId',0
.szInstanceId.size = $-.szInstanceId

.szRThumb        db 'RThumb',0
.szRIndexFinger  db 'RIndex',0
.szRMiddleFinger db 'RMiddle',0
.szRRingFinger   db 'RRing',0
.szRLittleFinger db 'RLittle',0
.szLThumb        db 'LThumb',0
.szLIndexFinger  db 'LIndex',0
.szLMiddleFinger db 'LMiddle',0
.szLRingFinger   db 'LRing',0
.szLLittleFinger db 'LLittle',0

;-------------------------------------------------------------------------;
; utils.inc
;-------------------------------------------------------------------------;
utils:

; GUID wsprintfA format
.szGuidFormat db '{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}',0

align 2
.wszRunAs           du 'runas',0

; ASCII digit to integer value table
align 16
.asciiDigitValueTable:
;  x0h x1h x2h x3h x4h x5h x6h x7h  x8h x9h xAh xBh xCh xDh xEh xFh
db 00h,00h,00h,00h,00h,00h,00h,00h, 00h,00h,00h,00h,00h,00h,00h,00h ; 0xh
db 00h,00h,00h,00h,00h,00h,00h,00h, 00h,00h,00h,00h,00h,00h,00h,00h ; 1xh
db 00h,00h,00h,00h,00h,00h,00h,00h, 00h,00h,00h,00h,00h,00h,00h,00h ; 2xh
db 00h,01h,02h,03h,04h,05h,06h,07h, 08h,09h,00h,00h,00h,00h,00h,00h ; 3xh
db 00h,0Ah,0Bh,0Ch,0Dh,0Eh,0Fh,00h, 00h,00h,00h,00h,00h,00h,00h,00h ; 4xh
db 00h,00h,00h,00h,00h,00h,00h,00h, 00h,00h,00h,00h,00h,00h,00h,00h ; 5xh
db 00h,0Ah,0Bh,0Ch,0Dh,0Eh,0Fh,00h, 00h,00h,00h,00h,00h,00h,00h,00h ; 6xh
db 00h,00h,00h,00h,00h,00h,00h,00h, 00h,00h,00h,00h,00h,00h,00h,00h ; 7xh
db 00h,00h,00h,00h,00h,00h,00h,00h, 00h,00h,00h,00h,00h,00h,00h,00h ; 8xh
db 00h,00h,00h,00h,00h,00h,00h,00h, 00h,00h,00h,00h,00h,00h,00h,00h ; 9xh
db 00h,00h,00h,00h,00h,00h,00h,00h, 00h,00h,00h,00h,00h,00h,00h,00h ; Axh
db 00h,00h,00h,00h,00h,00h,00h,00h, 00h,00h,00h,00h,00h,00h,00h,00h ; Bxh
db 00h,00h,00h,00h,00h,00h,00h,00h, 00h,00h,00h,00h,00h,00h,00h,00h ; Cxh
db 00h,00h,00h,00h,00h,00h,00h,00h, 00h,00h,00h,00h,00h,00h,00h,00h ; Dxh
db 00h,00h,00h,00h,00h,00h,00h,00h, 00h,00h,00h,00h,00h,00h,00h,00h ; Exh
db 00h,00h,00h,00h,00h,00h,00h,00h, 00h,00h,00h,00h,00h,00h,00h,00h ; Fxh

; Subfactor names
.szUnknown       db 'Unknown',0
.szRThumb        db 'R Thumb',0
.szRIndexFinger  db 'R Index',0
.szRMiddleFinger db 'R Middle',0
.szRRingFinger   db 'R Ring',0
.szRLittleFinger db 'R Little',0
.szLThumb        db 'L Thumb',0
.szLIndexFinger  db 'L Index',0
.szLMiddleFinger db 'L Middle',0
.szLRingFinger   db 'L Ring',0
.szLLittleFinger db 'L Little',0

align 4
.pszSubfactorTable:
  dd .szUnknown
  dd .szRThumb
  dd .szRIndexFinger
  dd .szRMiddleFinger
  dd .szRRingFinger
  dd .szRLittleFinger
  dd .szLThumb
  dd .szLIndexFinger
  dd .szLMiddleFinger
  dd .szLRingFinger
  dd .szLLittleFinger

; Non-abbreviated subfactor names
.szUnknownFull       db 'Unknown Finger',0
.szRThumbFull        db 'Right Thumb',0
.szRIndexFingerFull  db 'Right Index',0
.szRMiddleFingerFull db 'Right Middle',0
.szRRingFingerFull   db 'Right Ring',0
.szRLittleFingerFull db 'Right Little',0
.szLThumbFull        db 'Left Thumb',0
.szLIndexFingerFull  db 'Left Index',0
.szLMiddleFingerFull db 'Left Middle',0
.szLRingFingerFull   db 'Left Ring',0
.szLLittleFingerFull db 'Left Little',0

align 4
.pszSubfactorFullTable:
  dd .szUnknownFull
  dd .szRThumbFull
  dd .szRIndexFingerFull
  dd .szRMiddleFingerFull
  dd .szRRingFingerFull
  dd .szRLittleFingerFull
  dd .szLThumbFull
  dd .szLIndexFingerFull
  dd .szLMiddleFingerFull
  dd .szLRingFingerFull
  dd .szLLittleFingerFull

; WinBio error texts
.szUNSUPPORTED_FACTOR                    db 'UNSUPPORTED_FACTOR',0
.szINVALID_UNIT                          db 'INVALID_UNIT',0
.szUNKNOWN_ID                            db 'UNKNOWN_ID',0
.szCANCELED                              db 'CANCELED',0
.szNO_MATCH                              db 'NO_MATCH',0
.szCAPTURE_ABORTED                       db 'CAPTURE_ABORTED',0
.szENROLLMENT_IN_PROGRESS                db 'ENROLLMENT_IN_PROGRESS',0
.szBAD_CAPTURE                           db 'BAD_CAPTURE',0
.szINVALID_CONTROL_CODE                  db 'INVALID_CONTROL_CODE',0
.szFACTOR_NOT_PRESENT                    db 'FACTOR_NOT_PRESENT',0
.szDATA_COLLECTION_IN_PROGRESS           db 'DATA_COLLECTION_IN_PROGRESS',0
.szUNSUPPORTED_DATA_FORMAT               db 'UNSUPPORTED_DATA_FORMAT',0
.szUNSUPPORTED_DATA_TYPE                 db 'UNSUPPORTED_DATA_TYPE',0
.szUNSUPPORTED_PURPOSE                   db 'UNSUPPORTED_PURPOSE',0
.szINVALID_DEVICE_STATE                  db 'INVALID_DEVICE_STATE',0
.szDEVICE_BUSY                           db 'DEVICE_BUSY',0
.szDATABASE_CANT_CREATE                  db 'DATABASE_CANT_CREATE',0
.szDATABASE_CANT_OPEN                    db 'DATABASE_CANT_OPEN',0
.szDATABASE_CANT_CLOSE                   db 'DATABASE_CANT_CLOSE',0
.szDATABASE_CANT_ERASE                   db 'DATABASE_CANT_ERASE',0
.szDATABASE_CANT_FIND                    db 'DATABASE_CANT_FIND',0
.szDATABASE_ALREADY_EXISTS               db 'DATABASE_ALREADY_EXISTS',0
.szDATABASE_INVALID_NAME                 db 'DATABASE_INVALID_NAME',0
.szDATABASE_FULL                         db 'DATABASE_FULL',0
.szDATABASE_LOCKED                       db 'DATABASE_LOCKED',0
.szDATABASE_CORRUPTED                    db 'DATABASE_CORRUPTED',0
.szDATABASE_NO_SUCH_RECORD               db 'DATABASE_NO_SUCH_RECORD',0
.szDUPLICATE_ENROLLMENT                  db 'DUPLICATE_ENROLLMENT',0
.szDATABASE_READ_ERROR                   db 'DATABASE_READ_ERROR',0
.szDATABASE_WRITE_ERROR                  db 'DATABASE_WRITE_ERROR',0
.szDATABASE_NO_RESULTS                   db 'DATABASE_NO_RESULTS',0
.szDATABASE_NO_MORE_RECORDS              db 'DATABASE_NO_MORE_RECORDS',0
.szDATABASE_EOF                          db 'DATABASE_EOF',0
.szDATABASE_BAD_X_VECTOR                 db 'DATABASE_BAD_X_VECTOR',0
.szINVALID_IDENTITY                      db 'INVALID_IDENTITY',0
.szINCORRECT_BSP                         db 'INCORRECT_BSP',0
.szINCORRECT_SENSOR_POOL                 db 'INCORRECT_SENSOR_POOL',0
.szNO_CAPTURE_DATA                       db 'NO_CAPTURE_DATA',0
.szINVALID_SENSOR_MODE                   db 'INVALID_SENSOR_MODE',0
.szUNKNOWN_0                             db 'UNKNOWN_0',0
.szUNKNOWN_1                             db 'UNKNOWN_1',0
.szLOCK_VIOLATION                        db 'LOCK_VIOLATION',0
.szDUPLICATE_TEMPLATE                    db 'DUPLICATE_TEMPLATE',0
.szINVALID_OPERATION                     db 'INVALID_OPERATION',0
.szSESSION_BUSY                          db 'SESSION_BUSY',0
.szASYNC_OPERATION_IN_PROGRESS           db 'ASYNC_OPERATION_IN_PROGRESS',0
.szINVALID_ASYNC_OPERATION               db 'INVALID_ASYNC_OPERATION',0
.szCRED_PROV_DISABLED                    db 'CRED_PROV_DISABLED',0
.szCRED_PROV_NO_CREDENTIAL               db 'CRED_PROV_NO_CREDENTIAL',0
.szDISABLED                              db 'DISABLED',0
.szCONFIGURATION_FAILURE                 db 'CONFIGURATION_FAILURE',0
.szSENSOR_UNAVAILABLE                    db 'SENSOR_UNAVAILABLE',0
.szSAS_ENABLED                           db 'SAS_ENABLED',0
.szDEVICE_FAILURE                        db 'DEVICE_FAILURE',0
.szFAST_USER_SWITCH_DISABLED             db 'FAST_USER_SWITCH_DISABLED',0
.szNOT_ACTIVE_CONSOLE                    db 'NOT_ACTIVE_CONSOLE',0
.szEVENT_MONITOR_ACTIVE                  db 'EVENT_MONITOR_ACTIVE',0
.szINVALID_PROPERTY_TYPE                 db 'INVALID_PROPERTY_TYPE',0
.szINVALID_PROPERTY_ID                   db 'INVALID_PROPERTY_ID',0
.szUNSUPPORTED_PROPERTY                  db 'UNSUPPORTED_PROPERTY',0
.szADAPTER_INTEGRITY_FAILURE             db 'ADAPTER_INTEGRITY_FAILURE',0
.szINCORRECT_SESSION_TYPE                db 'INCORRECT_SESSION_TYPE',0
.szSESSION_HANDLE_CLOSED                 db 'SESSION_HANDLE_CLOSED',0
.szDEADLOCK_DETECTED                     db 'DEADLOCK_DETECTED',0
.szNO_PREBOOT_IDENTITY                   db 'NO_PREBOOT_IDENTITY',0
.szMAX_ERROR_COUNT_EXCEEDED              db 'MAX_ERROR_COUNT_EXCEEDED',0
.szAUTO_LOGON_DISABLED                   db 'AUTO_LOGON_DISABLED',0
.szINVALID_TICKET                        db 'INVALID_TICKET',0
.szTICKET_QUOTA_EXCEEDED                 db 'TICKET_QUOTA_EXCEEDED',0
.szDATA_PROTECTION_FAILURE               db 'DATA_PROTECTION_FAILURE',0
.szCRED_PROV_SECURITY_LOCKOUT            db 'CRED_PROV_SECURITY_LOCKOUT',0
.szUNSUPPORTED_POOL_TYPE                 db 'UNSUPPORTED_POOL_TYPE',0
.szSELECTION_REQUIRED                    db 'SELECTION_REQUIRED',0
.szPRESENCE_MONITOR_ACTIVE               db 'PRESENCE_MONITOR_ACTIVE',0
.szINVALID_SUBFACTOR                     db 'INVALID_SUBFACTOR',0
.szINVALID_CALIBRATION_FORMAT_ARRAY      db 'INVALID_CALIBRATION_FORMAT_ARRAY',0
.szNO_SUPPORTED_CALIBRATION_FORMAT       db 'NO_SUPPORTED_CALIBRATION_FORMAT',0
.szUNSUPPORTED_SENSOR_CALIBRATION_FORMAT db 'UNSUPPORTED_SENSOR_CALIBRATION_FORMAT',0
.szCALIBRATION_BUFFER_TOO_SMALL          db 'CALIBRATION_BUFFER_TOO_SMALL',0
.szCALIBRATION_BUFFER_TOO_LARGE          db 'CALIBRATION_BUFFER_TOO_LARGE',0
.szCALIBRATION_BUFFER_INVALID            db 'CALIBRATION_BUFFER_INVALID',0

align 4
.pszErrorTextTable:
  dd .szUNSUPPORTED_FACTOR
  dd .szINVALID_UNIT
  dd .szUNKNOWN_ID
  dd .szCANCELED
  dd .szNO_MATCH
  dd .szCAPTURE_ABORTED
  dd .szENROLLMENT_IN_PROGRESS
  dd .szBAD_CAPTURE
  dd .szINVALID_CONTROL_CODE
  dd .szFACTOR_NOT_PRESENT
  dd .szDATA_COLLECTION_IN_PROGRESS
  dd .szUNSUPPORTED_DATA_FORMAT
  dd .szUNSUPPORTED_DATA_TYPE
  dd .szUNSUPPORTED_PURPOSE
  dd .szINVALID_DEVICE_STATE
  dd .szDEVICE_BUSY
  dd .szDATABASE_CANT_CREATE
  dd .szDATABASE_CANT_OPEN
  dd .szDATABASE_CANT_CLOSE
  dd .szDATABASE_CANT_ERASE
  dd .szDATABASE_CANT_FIND
  dd .szDATABASE_ALREADY_EXISTS
  dd .szDATABASE_INVALID_NAME
  dd .szDATABASE_FULL
  dd .szDATABASE_LOCKED
  dd .szDATABASE_CORRUPTED
  dd .szDATABASE_NO_SUCH_RECORD
  dd .szDUPLICATE_ENROLLMENT
  dd .szDATABASE_READ_ERROR
  dd .szDATABASE_WRITE_ERROR
  dd .szDATABASE_NO_RESULTS
  dd .szDATABASE_NO_MORE_RECORDS
  dd .szDATABASE_EOF
  dd .szDATABASE_BAD_X_VECTOR
  dd .szINVALID_IDENTITY
  dd .szINCORRECT_BSP
  dd .szINCORRECT_SENSOR_POOL
  dd .szNO_CAPTURE_DATA
  dd .szINVALID_SENSOR_MODE
  dd .szUNKNOWN_0
  dd .szUNKNOWN_1
  dd .szLOCK_VIOLATION
  dd .szDUPLICATE_TEMPLATE
  dd .szINVALID_OPERATION
  dd .szSESSION_BUSY
  dd .szASYNC_OPERATION_IN_PROGRESS
  dd .szINVALID_ASYNC_OPERATION
  dd .szCRED_PROV_DISABLED
  dd .szCRED_PROV_NO_CREDENTIAL
  dd .szDISABLED
  dd .szCONFIGURATION_FAILURE
  dd .szSENSOR_UNAVAILABLE
  dd .szSAS_ENABLED
  dd .szDEVICE_FAILURE
  dd .szFAST_USER_SWITCH_DISABLED
  dd .szNOT_ACTIVE_CONSOLE
  dd .szEVENT_MONITOR_ACTIVE
  dd .szINVALID_PROPERTY_TYPE
  dd .szINVALID_PROPERTY_ID
  dd .szUNSUPPORTED_PROPERTY
  dd .szADAPTER_INTEGRITY_FAILURE
  dd .szINCORRECT_SESSION_TYPE
  dd .szSESSION_HANDLE_CLOSED
  dd .szDEADLOCK_DETECTED
  dd .szNO_PREBOOT_IDENTITY
  dd .szMAX_ERROR_COUNT_EXCEEDED
  dd .szAUTO_LOGON_DISABLED
  dd .szINVALID_TICKET
  dd .szTICKET_QUOTA_EXCEEDED
  dd .szDATA_PROTECTION_FAILURE
  dd .szCRED_PROV_SECURITY_LOCKOUT
  dd .szUNSUPPORTED_POOL_TYPE
  dd .szSELECTION_REQUIRED
  dd .szPRESENCE_MONITOR_ACTIVE
  dd .szINVALID_SUBFACTOR
  dd .szINVALID_CALIBRATION_FORMAT_ARRAY
  dd .szNO_SUPPORTED_CALIBRATION_FORMAT
  dd .szUNSUPPORTED_SENSOR_CALIBRATION_FORMAT
  dd .szCALIBRATION_BUFFER_TOO_SMALL
  dd .szCALIBRATION_BUFFER_TOO_LARGE
  dd .szCALIBRATION_BUFFER_INVALID