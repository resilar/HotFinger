;-------------------------------------------------------------------------;
; Sensor dialog
;-------------------------------------------------------------------------;

DLG_WIDTH = 350
DLG_HEIGHT = 120

DLG_WIDTH_MIN = 250
DLG_HEIGHT_MIN = 80

SCALE_DENOMINATOR = 1024

struct SENSOR_PARAMS
  szTitle   dd ?
  pUnits    dd ?
  dwCount   dd ?
  dwInstall dd ?
  hSession  dd ?
  hdlg      dd ?

  ; Scaling factors for converting dialog units (DLUs) to pixels
  dwScaleX  dd ?
  dwScaleY  dd ?
ends

struct SHELLEXECUTEINFO
  cbSize       dd ?
  fMask        dd ?
  hwnd         dd ?
  lpVerb       dd ?
  lpFile       dd ?
  lpParameters dd ?
  lpDirectory  dd ?
  nShow        dd ?
  hInstApp     dd ?
  lpIDList     dd ?
  lpClass      dd ?
  hkeyClass    dd ?
  dwHotKey     dd ?
  union
    hIcon      dd ?
    hMonitor   dd ?
  ends
  hProcess     dd ?
ends

SEE_MASK_NOCLOSEPROCESS = 00000040h

; pUnits[dwCount]:WINBIO_UNIT_SCHEMA
;proc SensorSelectDialog pUnits:DWORD, dwCount:DWORD
SensorSelectDialog:
                push ebp
                mov ebp, esp
                sub esp, .allocaSize
                virtual at ebp - .allocaSize
                  .params   SENSOR_PARAMS
                  .allocaSize = $ - $$
                  .ebp      dd ?
                  .retAddr  dd ?
                  .pUnits   dd ?
                  .dwCount  dd ?
                end virtual

                mov [.params.szTitle], sensor.szInstallModeTitle
                mov [.params.dwInstall], 1
                mov edx, [.pUnits]
                mov ecx, [.dwCount]
                mov [.params.pUnits], edx
                mov [.params.dwCount], ecx
                mov [.params.hSession], NULL
                mov [.params.hdlg], NULL
                mov [.params.dwScaleX], 4 ; Updated to real accurate ...
                mov [.params.dwScaleY], 8 ; ... value in WM_INITIDIALOG

                lea eax, [.params]
                push eax
                push SensorDialogProc
                push 0
                push IDD_SENSOR
                push NULL
                call [DialogBoxParamA]

                cmp [.params.hSession], 0
                jz @f
                push eax
                mov eax, [.params.hSession]
                push 0
                push eax
                call [WinBioCloseSession]
                pop [.params.hSession]
                pop eax
@@:
                test eax, eax
                js .fail
                imul eax, sizeof.WINBIO_UNIT_SCHEMA
                add eax, [.pUnits]

                cmp byte [.params.dwInstall], 0
                jz .exit

                push ebx
                push esi
                push edi
                push eax

                ; Prepare command-line parameters: install "wszDeviceInstanceId"
                lea eax, [eax + WINBIO_UNIT_SCHEMA.wszDeviceInstanceId]
                push eax
                call [lstrlenW]
                lea ecx, [eax+eax + 2*(sensor.wszInstall.strlen + 1 + 1 + 0 + 1 + 1)]
                push ecx
                push 0
                call [GetProcessHeap]
                push eax
                call [HeapAlloc]
                test eax, eax
                jz .install_fail
                mov ebx, eax
                mov edi, eax
                mov esi, sensor.wszInstall
                push sensor.wszInstall.strlen
                xor eax, eax
                pop ecx
                rep movsw
                mov al, ' '
                stosw
                mov al, '"'
                stosw
                mov edx, dword [esp]
                lea esi, [edx + WINBIO_UNIT_SCHEMA.wszDeviceInstanceId]
@@:             lodsw
                test ax, ax
                jz @f
                stosw
                jmp @b
@@:             push '"'
                pop eax
                stosw
                xor al, al
                stosw

                push NULL
                push ebx
                call RunAsAdmin
                test eax, eax
                jz .installed

.install_fail:  xor eax, eax
                mov [esp], eax
                push MB_ICONERROR or MB_OK
                push sensor.szError
                push sensor.szInstallError
                push eax
                call [MessageBoxA]

.installed:     push ebx
                push 0
                call [GetProcessHeap]
                push eax
                call [HeapFree]

                pop eax
                pop edi
                pop esi
                pop ebx
                jmp .exit
.fail:          xor eax, eax
.exit:          leave
                retn 8h

proc SensorDialogProc hdlg:DWORD, msg:DWORD, wParam:DWORD, lParam:DWORD
                push .handled
                mov eax, [msg]
                cmp eax, WM_INITDIALOG
                jz .WM_INITDIALOG
                cmp eax, WM_GETMINMAXINFO
                jz .WM_GETMINMAXINFO
                cmp eax, WM_SIZE
                jz .WM_SIZE
                cmp eax, WM_COMMAND
                jz .WM_COMMAND
                cmp eax, WM_NOTIFY
                jz .WM_NOTIFY
                cmp eax, WM_WINBIO
                jz .WM_WINBIO
                cmp eax, WM_CLOSE
                jz .WM_CLOSE
.unhandled:     ;pop eax ; don't bother, leave balances the stack anyway
                xor eax, eax
                jmp @f

.handled:       xor eax, eax
                inc eax
@@:             ;leave ; auto-generated by FASM
                ret

.get_ebx_pars:  push DWL_USER
                push [hdlg]
                call [GetWindowLongA]
                test eax, eax
                jz .unhandled
                mov ebx, eax
                retn

;-------------------------------------------------------------------------;

.WM_INITDIALOG: push ebx
                mov ebx, [lParam]
                push [hdlg]
                pop [ebx + SENSOR_PARAMS.hdlg]

                push ebx
                push DWL_USER
                push [hdlg]
                call [SetWindowLongA]

                push [hdlg]
                call CalculatePixelScaleFactors

                push IDI_MAIN
                push NULL
                call [GetModuleHandleA]
                push eax
                call [LoadIconA]
                push eax
                push eax
                push ICON_BIG
                push WM_SETICON
                push [hdlg]
                call [SendMessageA]
                push ICON_SMALL
                push WM_SETICON
                push [hdlg]
                call [SendMessageA]

                push LVS_EX_FULLROWSELECT
                push LVS_EX_FULLROWSELECT
                push LVM_SETEXTENDEDLISTVIEWSTYLE
                push IDC_LIST_SENSORS
                push [hdlg]
                call [SendDlgItemMessageA]

                push [hdlg]
                call InitSensorListHeader

                cmp [ebx + SENSOR_PARAMS.dwInstall], 0
                jz @f
                push sensor.szInstall
                push 0
                push WM_SETTEXT
                push IDC_BTN_SELECT
                push [hdlg]
                call [SendDlgItemMessageA]
                push 1
                push 0
                push BCM_SETSHIELD
                push IDC_BTN_SELECT
                push [hdlg]
                call [SendDlgItemMessageA]
@@:
                push [ebx + SENSOR_PARAMS.dwCount]
                push [ebx + SENSOR_PARAMS.pUnits]
                push [hdlg]
                call InitListItems
                push [ebx + SENSOR_PARAMS.szTitle]
                push [hdlg]
                call [SetWindowTextA]

                ; Dialog positioning
                push SWP_NOACTIVATE or SWP_NOZORDER

                push eax
                push eax
                push eax
                push eax
                push esp
                push [hdlg]
                call GetInitialDialogRect

                mov eax, [esp]
                sub [esp+8h], eax

                mov eax, [esp+4h]
                sub [esp+0ch], eax

                push HWND_TOPMOST ; HWND_TOP?
                push [hdlg]
                call [SetWindowPos]

                push 0
                push 0
                push WM_SETFOCUS
                push IDC_LIST_SENSORS
                push [hdlg]
                call [SendDlgItemMessageA]

                ; Open async WinBio session for locating sensors by touch
                lea eax, [ebx + SENSOR_PARAMS.hSession]
                push eax
                push TRUE
                push ebx
                push NULL
                push WM_WINBIO
                push [hdlg]
                push WINBIO_ASYNC_NOTIFY_MESSAGE
                push WINBIO_DB_DEFAULT
                push 0
                push NULL
                push WINBIO_FLAG_DEFAULT
                push WINBIO_POOL_SYSTEM
                push WINBIO_TYPE_FINGERPRINT
                call [WinBioAsyncOpenSession]
                test eax, eax
                pop ebx
                jnz .session_error
                retn
.session_error: push MB_ICONERROR or MB_OK
                push sensor.szError
                push sensor.szSessionError
                push NULL
                call [MessageBoxA]
                jmp .WM_CLOSE

;--------------------------------------------------------------------------;

.WM_GETMINMAXINFO:
                cmp [lParam], 0
                jz .unhandled

                push ebx
                call .get_ebx_pars
                push [hdlg]
                call CalculatePixelScaleFactors

                push DLG_HEIGHT_MIN
                call DLU2PixelsY
                push eax

                push DLG_WIDTH_MIN
                call DLU2PixelsX
                push eax

                mov eax, [lParam]
                pop [eax + MINMAXINFO.ptMinTrackSize.x]
                pop [eax + MINMAXINFO.ptMinTrackSize.y]

                pop ebx
                retn

.WM_SIZE:       push ebx
                call .get_ebx_pars
                push [hdlg]
                call CalculatePixelScaleFactors
                push esi
                movzx esi, word [lParam]
                push edi
                movzx edi, word [lParam+2]
                mov eax, .move_window

                ; Return-oriented programming for fun.
                ; And for the sake of premature optimization too, of course.
                push IDC_BTN_SELECT
                push 14
                push 50
                push -18
                push -108
                call .push_exit ; pushes .all_moved to the stack

.all_moved:     push RDW_INVALIDATE
                push 0
                push 0
                push [ebx + SENSOR_PARAMS.hdlg]
                call [RedrawWindow]

                pop edi
                pop esi
                pop ebx
                retn
.push_exit:
                push IDC_BTN_CANCEL
                push 14
                push 50
                push -18
                push -54
                push eax

                push IDC_STATIC_MESSAGE
                push 8
                push -112
                push -14
                push 4
                push eax

                push IDC_LIST_SENSORS
                push -24
                push -8
                push 4
                push 4
                push eax

                ; Fall-through

;proc move_window x:DWORD, y:DWORD, cx:DWORD, cy:DWORD, idc:DWORD
; Expects ebx = SENSOR_PARAMS, esi = DLG_WIDTH, edi = DLG_HEIGHT
.move_window:
                push ebp
                mov ebp, esp

                virtual at ebp
                  .ebp dd ?
                  .ret dd ?
                  .x   dd ?
                  .y   dd ?
                  .cx  dd ?
                  .cy  dd ?
                  .idc dd ?
                  virtual at .idc
                    .repaint dd ?
                  end virtual
                end virtual

                ; Convert DLUs to pixels
                push [.x]
                call DLU2PixelsX
                test eax, eax
                jns @f
                add eax, esi
@@:             mov [.x], eax

                push [.y]
                call DLU2PixelsY
                test eax, eax
                jns @f
                add eax, edi
@@:             mov [.y], eax

                push [.cx]
                call DLU2PixelsX
                test eax, eax
                jns @f
                add eax, esi
@@:             mov [.cx], eax

                push [.cy]
                call DLU2PixelsY
                test eax, eax
                jns @f
                add eax, edi
@@:             mov [.cy], eax

                ; Prepare the stack for a MoveWindow call.
                push dword [.idc]
                push TRUE
                pop [.repaint] ; [.idc]
                push [ebx + SENSOR_PARAMS.hdlg]
                call [GetDlgItem]
                leave
                pop ecx ; return address
                push eax
                push ecx
                jmp [MoveWindow]

;--------------------------------------------------------------------------;

.WM_COMMAND:    mov eax, [wParam]
                cmp eax, IDC_BTN_CANCEL + (BN_CLICKED shl 16)
                jz .WM_CLOSE
                cmp eax, IDC_BTN_SELECT + (BN_CLICKED shl 16)
                jnz .unhandled

.doSelect:      push LVNI_SELECTED
                push -1
                push LVM_GETNEXTITEM
                push IDC_LIST_SENSORS
                push [hdlg]
                call [SendDlgItemMessageA]

                push eax
                push [hdlg]
                call [EndDialog]
                retn

;--------------------------------------------------------------------------;

.WM_NOTIFY:     mov eax, [lParam]
                cmp [eax + NMHDR.idFrom], IDC_LIST_SENSORS
                jnz @f
                cmp [eax + NMHDR.code], LVN_ITEMCHANGED
                jnz .unhandled
                push [hdlg]
                call UpdateButtons
                retn

;--------------------------------------------------------------------------;

.WM_WINBIO:     mov eax, [lParam]
                test eax, eax
                jz .unhandled
                push ebx
                mov ebx, eax

.check_open:    xor ecx, ecx
                inc ecx ; WINBIO_OPERATION_OPEN
                cmp [ebx + WINBIO_ASYNC_RESULT.dwOperation], ecx
                jnz .check_locate
                mov eax, [ebx + WINBIO_ASYNC_RESULT.hSessionHandle]
                mov ecx, [ebx + WINBIO_ASYNC_RESULT.pUserData]
                test eax, eax
                mov [ecx + SENSOR_PARAMS.hSession], eax
                jz .open_failed
                push 0
                push eax
                call [WinBioLocateSensor]
                test eax, eax
                jz @f
                push 0
                push [ebx + SENSOR_PARAMS.hSession]
                call [WinBioCloseSession]
                pop [ebx + SENSOR_PARAMS.hSession]
                jmp .winbio_free
@@:             push sensor.szActivationText
                push 0
                push WM_SETTEXT
                push IDC_STATIC_MESSAGE
                push [hdlg]
                call [SendDlgItemMessageA]
                jmp .winbio_free

.check_locate:  mov cl, WINBIO_OPERATION_LOCATE_SENSOR
                cmp [ebx + WINBIO_ASYNC_RESULT.dwOperation], ecx
                jnz .winbio_free
                push [ebx + WINBIO_ASYNC_RESULT.dwUnitId]
                push [hdlg]
                call SelectUnit
                push 0
                push [ebx + WINBIO_ASYNC_RESULT.hSessionHandle]
                call [WinBioLocateSensor]

.winbio_free:   push ebx
                call [WinBioFree]
                pop ebx
                jmp .unhandled

.open_failed:   call [WinBioFree]
                jmp .session_error

;-------------------------------------------------------------------------;

.WM_CLOSE:      push -1
                push [hdlg]
                call [EndDialog]
                retn
endp
;-------------------------------------------------------------------------;

; Expects ebx = SENSOR_PARAMS
;proc CalculatePixelScaleFactors hdlg:DWORD
CalculatePixelScaleFactors:
                mov eax, SCALE_DENOMINATOR
                xor ecx, ecx
                push eax  ; bottom
                push eax  ; right
                push ecx  ; top
                push ecx  ; left

                push esp
                push dword [esp + 4h + 4*4h + 4h] ; [hdlg]
                call [MapDialogRect]

                pop eax ; left
                pop eax ; top
                pop [ebx + SENSOR_PARAMS.dwScaleX] ; right
                pop [ebx + SENSOR_PARAMS.dwScaleY] ; bottom
                retn 4h

; Expects ebx = SENSOR_PARAMS
;proc DLU2PixelsX x:DWORD
DLU2PixelsX:
                push SCALE_DENOMINATOR
                push [ebx + SENSOR_PARAMS.dwScaleX]
                push dword [esp + 2*4h + 4h]
                call [MulDiv]
                retn 4h

; Expects ebx = SENSOR_PARAMS
;proc DLU2PixelsY y:DWORD
DLU2PixelsY:
                push SCALE_DENOMINATOR
                push [ebx + SENSOR_PARAMS.dwScaleY]
                push dword [esp + 2*4h + 4h]
                call [MulDiv]
                retn 4h

proc InitSensorListHeader hdlg:DWORD
                local column:LV_COLUMN

                push ebx
                push edi
                xor eax, eax
                lea ecx, [eax + sizeof.LV_COLUMN]
                lea edi, [column]
                mov ebx, edi
                rep stosb

                mov edi, sensor.szHeaderTextArray
                mov [column.mask], LVCF_TEXT+LVCF_WIDTH+LVCF_SUBITEM
                mov [column.cx], 52
.init_column:   xor eax, eax
                mov [column.pszText], edi
                scasb
                jz .done
                lea ecx, [eax-1]
                repnz scasb
                not ecx

                push ebx
                push [column.iSubItem]
                push LVM_INSERTCOLUMN
                push IDC_LIST_SENSORS
                push [hdlg]
                call [SendDlgItemMessageA]
                push LVSCW_AUTOSIZE_USEHEADER
                push 0
                push LVM_SETCOLUMNWIDTH
                push IDC_LIST_SENSORS
                push [hdlg]
                call [SendDlgItemMessageA]

                inc [column.iSubItem]
                jmp .init_column

.done:          push LVSCW_AUTOSIZE_USEHEADER
                push 0
                push LVM_SETCOLUMNWIDTH
                push IDC_LIST_SENSORS
                push [hdlg]
                call [SendDlgItemMessageA]

.quit:          pop edi
                pop ebx
                ret
endp

; pUnits[dwCount]:WINBIO_UNIT_SCHEMA
proc InitListItems hdlg:DWORD, pUnits:DWORD, dwCount:DWORD
                local szBuf[WINBIO_MAX_STRING_LEN]:BYTE
                local item:LV_ITEM

                push ebx
                push esi
                push edi
                xor eax, eax
                lea edi, [item]
                mov ebx, edi
                mov ecx, sizeof.LV_ITEM
                rep stosb

                lea edi, [szBuf]
                mov esi, [pUnits]
                mov [item.mask], LVIF_TEXT+LVIF_STATE
                mov [item.state], LVIS_SELECTED
                mov [item.pszText], edi

.next:          dec [dwCount]
                js .done

                lea eax, [item]
                mov [eax + LV_ITEM.iSubItem], 0
                push eax
                push 0
                push LVM_INSERTITEM
                push IDC_LIST_SENSORS
                push [hdlg]
                call [SendDlgItemMessageA]

                ; Unit ID
                mov eax, [esi + WINBIO_UNIT_SCHEMA.dwUnitId]
                call .set_cell_d

                ; Device instance ID
                mov ebx, .set_cell_ls
                lea eax, [esi + WINBIO_UNIT_SCHEMA.wszDeviceInstanceId]
                call ebx

                ; Description
                lea eax, [esi + WINBIO_UNIT_SCHEMA.wszDescription]
                call ebx

                ; Manufacturer
                lea eax, [esi + WINBIO_UNIT_SCHEMA.wszManufacturer]
                call ebx

                ; Model
                lea eax, [esi + WINBIO_UNIT_SCHEMA.wszModel]
                call ebx

                ; Serial no
                lea eax, [esi + WINBIO_UNIT_SCHEMA.wszSerialNumber]
                call ebx

                ; Firmware version
                mov eax, [esi + WINBIO_UNIT_SCHEMA.FirmwareVersion.dwMajorVersion]
                mov ecx, [esi + WINBIO_UNIT_SCHEMA.FirmwareVersion.dwMinorVersion]
                call .set_cell_dd

                ; Pool type
                mov ebx, .set_cell_d
                mov eax, [esi + WINBIO_UNIT_SCHEMA.dwPoolType]
                call ebx

                ; Biometric factor
                mov eax, [esi + WINBIO_UNIT_SCHEMA.dwBiometricFactor]
                call ebx

                ; Sensor subtype
                mov eax, [esi + WINBIO_UNIT_SCHEMA.dwSensorSubType]
                call ebx

                ; Sensor capabilities
                mov eax, [esi + WINBIO_UNIT_SCHEMA.dwCapabilities]
                call .set_cell_08x

                inc [item.iItem]
                mov [item.mask], LVIF_TEXT
                add esi, sizeof.WINBIO_UNIT_SCHEMA
                jmp .next

.done:          pop edi
                pop esi
                pop ebx
                ret

.set_cell_d:    call @f
                db '%d',0
.set_cell_dd:   call @f
                db '%d.%d',0
.set_cell_08x:  call @f
                db '0x%08X',0
.set_cell_ls:   call @f
                db '%ls',0
@@:             xchg [esp], ecx
                push eax
                push ecx
                push edi
                call [wsprintfA]
                add esp, 10h

                lea eax, [item]
                push LVSCW_AUTOSIZE_USEHEADER
                push [eax + LV_ITEM.iSubItem]

                push eax
                push 0
                push LVM_SETITEM
                push IDC_LIST_SENSORS
                push [hdlg]
                call [SendDlgItemMessageA]

                push LVM_SETCOLUMNWIDTH
                push IDC_LIST_SENSORS
                push [hdlg]
                call [SendDlgItemMessageA]

                inc [item.iSubItem]
                retn
endp

; Precondition is that the sensor list is populated.
; rect:RECT
proc GetInitialDialogRect hdlg:DWORD, rect:DWORD
                local hwnd:DWORD
                local dpi:DWORD
                local width:DWORD
                local height:DWORD
                local cxscreen:DWORD
                local cyscreen:DWORD

                push IDC_LIST_SENSORS
                push [hdlg]
                call [GetDlgItem]
                mov [hwnd], eax
                push eax
                call [GetDpiForWindow]
                mov [dpi], eax

                ; Get dimensions of the list view control
                push 0
                push 0
                push LVM_GETITEMCOUNT
                push [hwnd]
                call [SendMessageA]
                cmp eax, 4
                lea eax, [eax+1]
                jae @f
                mov al, 5
@@:             push -1
                push eax
                push LVM_APPROXIMATEVIEWRECT
                push [hwnd]
                call [SendMessageA]

                ; Adjust for its non-client area
                movzx ecx, ax
                shr eax, 16
                push eax
                push ecx
                push 0
                push 0
                push esp
                push [dpi]
                push [hwnd]
                call AdjustDialogRectForDPI
                pop ecx
                pop edx
                pop eax
                sub eax, ecx
                mov [width], eax
                pop eax
                sub eax, edx
                mov [height], eax

                ; Add space outside the list view control
                push 8
                call DLU2PixelsX
                add [width], eax
                push 20
                call DLU2PixelsY
                add [height], eax

                ; Check that the resulting dimensions are within bounds
                push DLG_WIDTH_MIN
                call DLU2PixelsX
                cmp [width], eax
                jae @f
                mov [width], eax
@@:             push [dpi]
                push SM_CXSCREEN
                call [GetSystemMetricsForDpi]
                mov [cxscreen], eax
                push 5
                push 4
                push eax
                call [MulDiv]
                cmp [width], eax
                jbe @f
                mov [width], eax
@@:
                push DLG_HEIGHT_MIN
                call DLU2PixelsY
                cmp [height], eax
                jae @f
                mov [height], eax
@@:             push [dpi]
                push SM_CYSCREEN
                call [GetSystemMetricsForDpi]
                mov [cyscreen], eax
                push 5
                push 4
                push eax
                call [MulDiv]
                cmp [height], eax
                jbe @f
                mov [height], eax
@@:
                ; Position in the center of the screen
                mov eax, [cxscreen]
                mov ecx, [width]
                lea edx, [eax+ecx]
                sub eax, ecx
                shr eax, 1
                shr edx, 1
                mov ecx, [rect]
                mov [ecx + RECT.left], eax
                mov [ecx + RECT.right], edx

                mov eax, [cyscreen]
                mov ecx, [height]
                lea edx, [eax+ecx]
                sub eax, ecx
                shr eax, 1
                shr edx, 1
                mov ecx, [rect]
                mov [ecx + RECT.top], eax
                mov [ecx + RECT.bottom], edx

                ; Finally, adjust for the non-client area of the dialog
                push ecx
                push [dpi]
                push [hdlg]
                call AdjustDialogRectForDPI
                ret
endp

; rect:RECT
proc AdjustDialogRectForDPI hwnd:DWORD, dpi:DWORD, rect:DWORD
                push [dpi]

                push GWL_EXSTYLE
                push [hwnd]
                call [GetWindowLongA]
                push eax

                push GWL_STYLE
                push [hwnd]
                call [GetWindowLongA]

                push FALSE
                push eax
                push [rect]
                call [AdjustWindowRectExForDpi]

                ret
endp

proc UpdateButtons hdlg:DWORD
                push 0
                push 0
                push LVM_GETSELECTEDCOUNT
                push IDC_LIST_SENSORS
                push [hdlg]
                call [SendDlgItemMessageA]
                dec eax
                jnz .disable
.enable:        push 1
                jmp @f
.disable:       push 0
@@:             push IDC_BTN_SELECT
                push [hdlg]
                call [GetDlgItem]
                push eax
                call [EnableWindow]
                ret
endp

proc SelectUnit hdlg:DWORD, dwUnitId:DWORD
                local item:LV_ITEM
                local buffer[64]:BYTE
                local dwCount:DWORD

                push esi
                push edi
                xor eax, eax
                mov ecx, sizeof.LV_ITEM
                lea esi, [buffer]
                lea edi, [item]
                rep stosb
                mov [item.pszText], esi
                mov [item.cchTextMax], 64-1

                push 0
                push 0
                push LVM_GETITEMCOUNT
                push IDC_LIST_SENSORS
                push [hdlg]
                call [SendDlgItemMessageA]
                mov [dwCount], eax

                xor edi, edi
.next:          xor eax, eax
                cmp [dwCount], eax
                jz .not_found

                lea eax, [item]
                push eax
                push edi
                inc edi
                push LVM_GETITEMTEXT
                push IDC_LIST_SENSORS
                push [hdlg]
                call [SendDlgItemMessageA]
                call atoi
                cmp eax, [dwUnitId]
                jnz .next

                dec edi
                lea esi, [item]
                mov [esi + LV_ITEM.mask], LVIF_STATE
                mov [esi + LV_ITEM.iItem], edi
                mov [esi + LV_ITEM.iSubItem], 0
                push esi
                push 0
                push LVM_GETITEM
                push IDC_LIST_SENSORS
                push [hdlg]
                call [SendDlgItemMessageA]
                mov eax, LVIS_SELECTED+LVIS_FOCUSED
                or [esi + LV_ITEM.state], eax
                mov [esi + LV_ITEM.stateMask], eax
                push esi
                push 0
                push LVM_SETITEM
                push IDC_LIST_SENSORS
                push [hdlg]
                call [SendDlgItemMessageA]

                push 1
                pop eax
.not_found:     pop esi
                pop edi
@@:             ret
endp

; Input:  guid[16]:BYTE
; Output: wszDeviceInstanceId[WINBIO_MAX_STRING_LEN]:WORD
proc GetSensorDeviceInstanceIdForDatabase guid:DWORD, wszDeviceInstanceId:DWORD
                local pUnits:DWORD
                local dwCount:DWORD

                push ebx
                push esi
                push edi

                push [wszDeviceInstanceId]
                push [guid]
                call GetDatabaseSensorDeviceInstanceId
                test eax, eax
                jnz .got_device

.retry:         lea eax, [dwCount]
                push eax
                lea ecx, [pUnits]
                push ecx
                push WINBIO_TYPE_FINGERPRINT
                call [WinBioEnumBiometricUnits]
                cmp eax, RPC_S_SERVER_TOO_BUSY
                jz .retry
                test eax, eax
                jz .enumerated

                mov eax, sensor.szSensorUnavailable
                lea ecx, [eax + sensor.szEnumSensorUnavailable - sensor.szSensorUnavailable]
                push MB_ICONEXCLAMATION+MB_OK
                push eax
                push ecx
                push NULL
                call [MessageBoxA]
                jmp .fail

.enumerated:    push [dwCount]
                push [pUnits]
                call SensorSelectDialog
                test eax, eax
                pushf
                push [pUnits]
                call [WinBioFree]
                popf
                jz .fail

                push [wszDeviceInstanceId]
                push [guid]
                call GetDatabaseSensorDeviceInstanceId
                test eax, eax
                jnz .got_device

.fail:          xor eax, eax
.got_device:    pop edi
                pop esi
                pop ebx
                ret
endp
