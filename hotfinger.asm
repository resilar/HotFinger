format PE GUI 4.0

include 'win32a.inc'
include 'winbio.inc'

;-------------------------------------------------------------------------
; Program settings
;-------------------------------------------------------------------------
struct GUI_SETTINGS
  x  dd ?
  y  dd ?
  cx dd ?
  cy dd ?
  dwMinimized dd ?
ends

struct COMMAND_SETTINGS
  RThumb  db MAX_PATH dup (?)
  RIndex  db MAX_PATH dup (?)
  RMiddle db MAX_PATH dup (?)
  RRing   db MAX_PATH dup (?)
  RLittle db MAX_PATH dup (?)
  LThumb  db MAX_PATH dup (?)
  LIndex  db MAX_PATH dup (?)
  LMiddle db MAX_PATH dup (?)
  LRing   db MAX_PATH dup (?)
  LLittle db MAX_PATH dup (?)
ends

struct VOLATILE_SETTINGS
  wszDeviceInstanceId du WINBIO_MAX_STRING_LEN dup (?)
ends

struct SETTINGS
  GUI GUI_SETTINGS
  cmd COMMAND_SETTINGS
  vol VOLATILE_SETTINGS
ends

section '.text' code readable executable
entry $
                push SW_SHOWDEFAULT
                call [GetCommandLineA]
                push eax
                push NULL
                push NULL
                call [GetModuleHandleA]
                push eax
                call WinMain

                push eax
                call [ExitProcess]

;-------------------------------------------------------------------------;
; WinMain
;-------------------------------------------------------------------------;
proc WinMain hInst:DWORD, hPrevInst:DWORD, szCmdLine:DWORD, nCmdShow:DWORD
                local ini:SETTINGS
                local wc:WNDCLASSEX
                local msg:MSG
                local icl:INITCOMMONCONTROLSEX
                local rc:DWORD

                push ebx
                push esi
                push edi

                push 1
                pop [rc]

                ; Initialize common controls & OLE
                push sizeof.INITCOMMONCONTROLSEX
                push ICC_LISTVIEW_CLASSES or ICC_BAR_CLASSES
                lea eax, [icl]
                pop [icl.dwICC]
                pop [icl.dwSize]
                push eax
                call [InitCommonControlsEx]
                push NULL
                call [CoInitialize]

                ; Parse command-line arguments
                call CommandLineInterface

                ; Focus existing window
                push NULL
                push hotfinger.szClassName
                call [FindWindowA]
                test eax, eax
                jz .not_found
                push NULL
                push NULL
                push WM_NUDGE
                push eax
                call [SendMessageA]
                push 2
                pop eax
                ret
.not_found:
                ; Read settings
                lea ebx, [ini]
                push ebx
                call ReadAppDataSettings

                lea eax, [ebx + SETTINGS.vol.wszDeviceInstanceId]
                push eax
                push guidDatabase
                call GetSensorDeviceInstanceIdForDatabase
                test eax, eax
                jz .die

;--------------------------------------------------------------------------;
; Register window class
;--------------------------------------------------------------------------;
                xor eax, eax
                lea ecx, [eax + sizeof.WNDCLASSEX]
                push ecx
                lea edi, [wc]
                rep stosb
                pop [wc.cbSize]
                mov [wc.style], CS_HREDRAW or CS_VREDRAW or CS_BYTEALIGNWINDOW
                mov [wc.lpfnWndProc], WndProc
                push [hInst]
                pop [wc.hInstance]

                push IDI_MAIN
                push [hInst]
                call [LoadIconA]
                mov [wc.hIcon], eax
                mov [wc.hIconSm], eax

                push IDC_ARROW
                push NULL
                call [LoadCursorA]
                mov [wc.hCursor], eax

                mov [wc.hbrBackground], COLOR_BTNFACE+1
                mov [wc.lpszClassName], hotfinger.szClassName

                lea eax, [wc]
                push eax
                call [RegisterClassExA]
                test eax, eax
                jnz .create_window

                push MB_ICONERROR
                push hotfinger.szError
                push hotfinger.szWindowRegistrationFailed
                push NULL
                call [MessageBoxA]
                jmp .die

;--------------------------------------------------------------------------;
; Create the window
;--------------------------------------------------------------------------;
.create_window: push ebx
                push [hInst]
                push NULL
                push NULL

                push [ini + SETTINGS.GUI.cy] ; height
                mov ecx, [ini + SETTINGS.GUI.y]
                sub [esp], ecx
                push [ini + SETTINGS.GUI.cx] ; width
                mov eax, [ini + SETTINGS.GUI.x]
                sub [esp], eax

                push ecx ; y
                push eax ; x

                push WS_OVERLAPPEDWINDOW
                push hotfinger.szCaption
                push hotfinger.szClassName
                push 0
                call [CreateWindowExA]
                test eax, eax
                jnz .show_window

                push MB_ICONERROR
                push hotfinger.szError
                push hotfinger.szWindowCreationFailed
                push eax ; NULL
                call [MessageBoxA]
                jmp .die

.show_window:   mov esi, eax
                lea edi, [msg]
                cmp [ini + SETTINGS.GUI.dwMinimized], 0
                jnz .show_in_tray
                push esi
                push [nCmdShow]
                push esi
                call [ShowWindow]
                call [UpdateWindow]
                jmp .msg_loop

.show_in_tray:  push SW_HIDE
                push esi
                call [ShowWindow]
                push esi
                call ShowTrayIcon

;--------------------------------------------------------------------------;
; Message loop
;--------------------------------------------------------------------------;
.msg_loop:      push 0
                push 0
                push NULL
                push edi
                call [GetMessageA]
                test eax, eax
                jz .finish

                push edi
                push esi
                call [IsDialogMessage]
                test eax, eax
                jnz .msg_loop

                push edi
                push edi
                call [TranslateMessage]
                call [DispatchMessageA]
                jmp .msg_loop
.finish:        push [msg.wParam]
                pop [rc]

;--------------------------------------------------------------------------;
; Finish
;--------------------------------------------------------------------------;
                cmp word [ini.vol.wszDeviceInstanceId], 0
                jz .die ; uninstalled or something, do not save settings

                push ebx
                call WriteAppDataSettings

.die:           call [CoUninitialize]

                pop edi
                pop esi
                pop ebx
                mov eax, [rc]
                ret
endp

include 'cli.inc'
include 'database.inc'
include 'gui.inc'
include 'sensor.inc'
include 'settings.inc'
include 'utils.inc'

section '.data' data readable writeable
include 'data.inc'

section '.idata' import readable
  library advapi32, 'advapi32.dll', \
          comctl32, 'comctl32.dll', \
          comdlg32, 'comdlg32.dll', \
          gdi32, 'gdi32.dll', \
          kernel32, 'kernel32.dll', \
          ole32, 'ole32.dll', \
          shell32, 'shell32.dll', \
          shlwapi, 'shlwapi.dll', \
          user32, 'user32.dll', \
          winbio, 'winbio.dll'

  import advapi32, \
         CloseServiceHandle, 'CloseServiceHandle', \
         ControlService, 'ControlService', \
         OpenSCManagerA, 'OpenSCManagerA', \
         OpenServiceA, 'OpenServiceA', \
         QueryServiceStatus, 'QueryServiceStatus', \
         RegCloseKey, 'RegCloseKey', \
         RegCreateKeyExA, 'RegCreateKeyExA', \
         RegDeleteKeyA, 'RegDeleteKeyA', \
         RegEnumKeyExA, 'RegEnumKeyExA', \
         RegEnumKeyExW, 'RegEnumKeyExW', \
         RegGetValueA, 'RegGetValueA', \
         RegOpenKeyExA, 'RegOpenKeyExA', \
         RegOpenKeyExW, 'RegOpenKeyExW', \
         RegSetValueExA, 'RegSetValueExA', \
         StartServiceA, 'StartServiceA'

  import comctl32, \
         InitCommonControlsEx, 'InitCommonControlsEx'

  import comdlg32, \
         GetOpenFileNameA, 'GetOpenFileNameA'

  import gdi32, \
         GetStockObject, 'GetStockObject'

  import kernel32, \
         CloseHandle, 'CloseHandle', \
         CreateDirectoryA, 'CreateDirectoryA', \
         DeleteFileA, 'DeleteFileA', \
         ExitProcess, 'ExitProcess', \
         GetCommandLineA, 'GetCommandLineA', \
         GetCommandLineW, 'GetCommandLineW', \
         GetExitCodeProcess, 'GetExitCodeProcess', \
         GetFileAttributesA, 'GetFileAttributesA', \
         GetLastError, 'GetLastError', \
         GetModuleFileNameW, 'GetModuleFileNameW', \
         GetModuleHandleA, 'GetModuleHandleA', \
         GetPrivateProfileIntA, 'GetPrivateProfileIntA', \
         GetPrivateProfileStringA, 'GetPrivateProfileStringA', \
         GetProcessHeap, 'GetProcessHeap', \
         HeapAlloc, 'HeapAlloc', \
         HeapFree, 'HeapFree', \
         HeapReAlloc, 'HeapReAlloc', \
         LocalFree, 'LocalFree', \
         MulDiv, 'MulDiv', \
         RemoveDirectoryA, 'RemoveDirectoryA', \
         Sleep, 'Sleep', \
         WaitForSingleObject, 'WaitForSingleObject', \
         Wow64DisableWow64FsRedirection, 'Wow64DisableWow64FsRedirection', \
         Wow64RevertWow64FsRedirection, 'Wow64RevertWow64FsRedirection', \
         WritePrivateProfileStringA, 'WritePrivateProfileStringA', \
         lstrcmpW, 'lstrcmpW', \
         lstrcmpiW, 'lstrcmpiW', \
         lstrlenA, 'lstrlenA', \
         lstrlenW, 'lstrlenW'

  import ole32, \
         CoInitialize, 'CoInitialize', \
         CoUninitialize, 'CoUninitialize'

  import shell32, \
         CommandLineToArgvW, 'CommandLineToArgvW', \
         SHGetFileInfo, 'SHGetFileInfo', \
         SHGetFolderPathA, 'SHGetFolderPathA', \
         Shell_NotifyIconA, 'Shell_NotifyIconA', \
         ShellExecuteA, 'ShellExecuteA', \
         ShellExecuteExW, 'ShellExecuteExW'

  import shlwapi, \
         SHAutoComplete, 'SHAutoComplete'

  import user32, \
        AdjustWindowRectExForDpi, 'AdjustWindowRectExForDpi', \
        AppendMenuA, 'AppendMenuA', \
        CreateMenu, 'CreateMenu', \
        CreatePopupMenu, 'CreatePopupMenu', \
        CreateWindowExA, 'CreateWindowExA', \
        DefWindowProcA, 'DefWindowProcA', \
        DestroyMenu, 'DestroyMenu', \
        DestroyWindow, 'DestroyWindow', \
        DialogBoxParamA, 'DialogBoxParamA', \
        DispatchMessageA, 'DispatchMessageA', \
        DrawMenuBar, 'DrawMenuBar', \
        EnableMenuItem, 'EnableMenuItem', \
        EnableWindow, 'EnableWindow', \
        EndDialog, 'EndDialog', \
        FindWindowA, 'FindWindowA', \
        GetClientRect, 'GetClientRect', \
        GetCursorPos, 'GetCursorPos', \
        GetDlgItem, 'GetDlgItem', \
        GetDpiForSystem, 'GetDpiForSystem', \
        GetDpiForWindow, 'GetDpiForWindow', \
        GetMenuItemCount, 'GetMenuItemCount', \
        GetMessageA, 'GetMessageA', \
        GetSystemMetricsForDpi, 'GetSystemMetricsForDpi', \
        GetWindowLongA, 'GetWindowLongA', \
        GetWindowRect, 'GetWindowRect', \
        GetWindowTextA, 'GetWindowTextA', \
        GetWindowTextLengthA, 'GetWindowTextLengthA', \
        IsDialogMessage, 'IsDialogMessage', \
        LoadCursorA, 'LoadCursorA', \
        LoadIconA, 'LoadIconA', \
        MapDialogRect, 'MapDialogRect', \
        MessageBoxA, 'MessageBoxA', \
        MoveWindow, 'MoveWindow', \
        PostQuitMessage, 'PostQuitMessage', \
        RedrawWindow, 'RedrawWindow', \
        RegisterClassExA, 'RegisterClassExA', \
        SendDlgItemMessageA, 'SendDlgItemMessageA', \
        SendMessageA, 'SendMessageA', \
        SetFocus, 'SetFocus', \
        SetForegroundWindow, 'SetForegroundWindow', \
        SetMenu, 'SetMenu', \
        SetWindowLongA, 'SetWindowLongA', \
        SetWindowPos, 'SetWindowPos', \
        SetWindowTextA, 'SetWindowTextA', \
        ShowWindow, 'ShowWindow', \
        TrackPopupMenu, 'TrackPopupMenu', \
        TranslateMessage, 'TranslateMessage', \
        UpdateWindow, 'UpdateWindow', \
        wsprintfA, 'wsprintfA'

  import winbio, \
        WinBioAsyncEnumBiometricUnits, 'WinBioAsyncEnumBiometricUnits', \
        WinBioAsyncOpenFramework, 'WinBioAsyncOpenFramework', \
        WinBioAsyncOpenSession, 'WinBioAsyncOpenSession', \
        WinBioCancel,'WinBioCancel', \
        WinBioCloseFramework, 'WinBioCloseFramework', \
        WinBioCloseSession, 'WinBioCloseSession', \
        WinBioDeleteTemplate, 'WinBioDeleteTemplate', \
        WinBioEnrollBegin, 'WinBioEnrollBegin', \
        WinBioEnrollCapture, 'WinBioEnrollCapture', \
        WinBioEnrollCommit, 'WinBioEnrollCommit', \
        WinBioEnrollDiscard, 'WinBioEnrollDiscard', \
        WinBioEnumBiometricUnits, 'WinBioEnumBiometricUnits', \
        WinBioFree, 'WinBioFree', \
        WinBioIdentify, 'WinBioIdentify', \
        WinBioLocateSensor, 'WinBioLocateSensor', \
        WinBioWait, 'WinBioWait'

section '.rsrc' resource data readable

; Dialogs
IDD_SENSOR = 100
IDD_ABOUT  = 101

; Sensor dialog
IDC_BTN_SELECT     = 2000
IDC_BTN_CANCEL     = 2001
IDC_STATIC_MESSAGE = 2002
IDC_LIST_SENSORS   = 2003

; About dialog
IDC_SYSLINK = 3000

; Icons
IDI_MAIN = 401

  directory RT_DIALOG, dialogs, \
            RT_ICON, icons, \
            RT_GROUP_ICON, group_icons, \
            RT_MANIFEST, manifests, \
            RT_VERSION, versions

  resource dialogs, \
           IDD_SENSOR, LANG_ENGLISH+SUBLANG_DEFAULT, sensor_dialog, \
           IDD_ABOUT, LANG_ENGLISH+SUBLANG_DEFAULT, about_dialog
  resource icons,     1, LANG_NEUTRAL, icon_data
  resource group_icons, IDI_MAIN, LANG_NEUTRAL, main_icon
  resource manifests, 1, LANG_NEUTRAL, manifest
  resource versions,  1, LANG_NEUTRAL, version

  dialog sensor_dialog, 'Sensor dialog', 0,0,DLG_WIDTH,DLG_HEIGHT, WS_POPUP+WS_OVERLAPPEDWINDOW+DS_MODALFRAME+DS_CENTER
    dialogitem 'BUTTON','&Select', IDC_BTN_SELECT, DLG_WIDTH-108,DLG_HEIGHT-18,50,14, BS_DEFPUSHBUTTON+WS_VISIBLE+WS_TABSTOP+WS_DISABLED
    dialogitem 'BUTTON','&Cancel', IDC_BTN_CANCEL, DLG_WIDTH-54,DLG_HEIGHT-18,50,14, BS_PUSHBUTTON+WS_VISIBLE+WS_TABSTOP
    dialogitem 'STATIC','Select sensor from the list', IDC_STATIC_MESSAGE, 4,DLG_HEIGHT-14,DLG_WIDTH-112,8, WS_VISIBLE
    dialogitem 'SysListView32','', IDC_LIST_SENSORS, 4,4,DLG_WIDTH-8,DLG_HEIGHT-24, WS_VISIBLE+WS_BORDER+WS_TABSTOP+LVS_REPORT+LVS_SINGLESEL+LVS_SHOWSELALWAYS
  enddialog

  dialog about_dialog, 'About', 40,40,160,55, WS_CAPTION+WS_POPUP+WS_SYSMENU+DS_MODALFRAME
    dialogitem 'STATIC', <'HotFinger application launcher'>, -1, 36,10,120,10, WS_VISIBLE
    dialogitem 'SysLink', <'<a>https://github.com/resilar/HotFinger</a>'>, IDC_SYSLINK, 36,20,120,10, WS_VISIBLE
    dialogitem 'STATIC', IDI_MAIN, -1, 8,8,32,32, WS_VISIBLE+SS_ICON
    dialogitem 'STATIC', '', -1, 4,35,152,11, WS_VISIBLE+SS_ETCHEDHORZ
    dialogitem 'STATIC', <'HotFinger v0.0.0'>, -1, 4,42,100,10, WS_VISIBLE+SS_LEFT
    dialogitem 'BUTTON', 'OK', IDOK, 114,38,42,14, WS_VISIBLE+WS_TABSTOP+BS_DEFPUSHBUTTON
  enddialog

  icon main_icon, icon_data, 'sysclass.ico'

  resdata manifest
    db '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>',0dh,0ah
    db '<assembly xmlns="urn:schemas-microsoft-com:asm.v1" xmlns:asmv3="urn:schemas-microsoft-com:asm.v3" manifestVersion="1.0">',0dh,0ah
    db ' <assemblyIdentity version="1.0.0.0" processorArchitecture="x86" type="win32" name="hotfinger.exe"/>',0dh,0ah
    db ' <description>HotFinger</description>',0dh,0ah
    db ' <dependency>',0dh,0ah
    db '  <dependentAssembly>',0dh,0ah
    db '    <assemblyIdentity type="win32" name="Microsoft.Windows.Common-Controls" version="6.0.0.0" processorArchitecture="x86" publicKeyToken="6595b64144ccf1df" language="*"/> ',0dh,0ah
    db '  </dependentAssembly>',0dh,0ah
    db ' </dependency>',0dh,0ah
    db ' <asmv3:application>',0dh,0ah
    db '  <asmv3:windowsSettings xmlns="http://schemas.microsoft.com/SMI/2005/WindowsSettings">',0dh,0ah
;    db '   <dpiAwareness xmlns="http://schemas.microsoft.com/SMI/2016/WindowsSettings">PerMonitorV2</dpiAwareness>',0dh,0ah
    db '   <dpiAware>true</dpiAware>',0dh,0ah
    db '  </asmv3:windowsSettings>',0dh,0ah
    db ' </asmv3:application>',0dh,0ah
    db '</assembly>',0dh,0ah
  endres

  versioninfo version, \
    VOS__WINDOWS32, VFT_APP,VFT2_UNKNOWN, LANG_ENGLISH+SUBLANG_DEFAULT, 0, \
    'FileDescription',  'Biometric hotkeys', \
    'FileVersion',      '0.0.0.0', \
    'ProductName',      'HotFinger', \
    'ProductVersion',   '0.0.0', \
    'LegalCopyright',   'UNLICENSE', \
    'OriginalFilename', 'hotfinger.exe'
