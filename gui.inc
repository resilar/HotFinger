;-------------------------------------------------------------------------;
; Main GUI of the application
;-------------------------------------------------------------------------;

IDC_LIST   = 200h
IDC_PATH   = 201h
IDC_BROWSE = 202h
IDC_ENROLL = 203h
IDC_STATUS = 204h

WM_APP        = 8000h
WM_WINBIO     = WM_APP+1
WM_DPICHANGED = 2e0h
WM_NUDGE      = WM_USER+100
WM_TRAY       = WM_USER+101

TRAY_ID = 100

IDM_PREFERENCES = WM_USER+201
IDM_UNINSTALL   = WM_USER+202
IDM_QUIT        = WM_USER+203
IDM_GITHUB      = WM_USER+301
IDM_ABOUT       = WM_USER+302

WIN_WIDTH  = 400
WIN_HEIGHT = 360

WIN_WIDTH_MIN  = WIN_WIDTH-50
WIN_HEIGHT_MIN = 205

SUBFACTOR_MIN = 1       ; R Thumb
SUBFACTOR_MAX = 10      ; L Little

COLUMN_FINGER  = 0
COLUMN_COMMAND = 1

STATUS_DEFAULT     = 1
STATUS_ENROLLED    = 2
STATUS_ENROLLING   = 3
STATUS_ERROR       = 4
STATUS_COMMITTING  = 5
STATUS_DISCARDING  = 6
STATUS_COMMITTED   = 7
STATUS_DISCARDED   = 8
STATUS_IDENTIFYING = 9

SS_ENDELLIPSIS = 4000h

EM_SETCUEBANNER = 1501h
BCM_SETSHIELD   = 160ch

SHACF_FILESYSTEM = 0x00000001
SHACF_URLHISTORY = 0x00000002
SHACF_URLMRU     = 0x00000004
SHACF_URLALL     = 0x00000006
SHACF_USETAB     = 0x00000008
SHACF_AUTOSUGGEST_FORCE_ON = 0x10000000

WINBIO_RPC_X_SS_IN_NULL_CONTEXT = 800106EFh

LVN_ITEMACTIVATE = LVN_FIRST-14
struct NMITEMACTIVATE
  hds NMHDR
  iItem dd ?
  iSubItem dd ?
  uNewState dd ?
  uOldState dd ?
  uChanged dd ?
  ptAction dd ?
  lParam dd ?
  uKeyFlags dd ?
ends

struct SHFILEINFO
  hIcon         dd      ?
  iIcon         dd      ?
  dwAttributes  dd      ?
  szDisplayName TCHAR   MAX_PATH dup (?)
  szTypeName    TCHAR   80 dup (?)
ends

struct POS
  x  dd ?
  y  dd ?
  cx dd ?
  cy dd ?
ends

struct WIN
  hwnd dd ?
  pos POS
ends

STATE_READY            = 0x00000000
STATE_INITIALIZING     = 0x00000001
STATE_ENROLLING        = 0x00000002
STATE_IDENTIFYING      = 0x00000004
STATE_ENROLL_REQUESTED = 0x00000008
STATE_ERROR            = 0x80000000

struct CONTEXT
  win          WIN
  pSettings    dd ?
  dwUnitId     dd ?
  dwState      dd ?
  dwDPI        dd ?

  hInst        dd ?
  hFont        dd ?
  hSession     dd ?
  hFramework   dd ?

  hwndStatus   dd ?
  hMenu        dd ?
  hOptionsMenu dd ?
  hHelpMenu    dd ?
  hTrayMenu    dd ?

  winList      WIN
  winInfo      WIN
  winPath      WIN
  winBrowse    WIN
  winEnroll    WIN
  winStatus    WIN
ends
CONTEXT_FIRST_WINDOW_OFFSET = CONTEXT.winList
CONTEXT_WINDOW_COUNT = (sizeof.CONTEXT - CONTEXT_FIRST_WINDOW_OFFSET)/sizeof.WIN

proc WndProc hwnd:DWORD, msg:DWORD, wParam:DWORD, lParam:DWORD
                local buf[MAX_PATH+4]:BYTE
                local ofn:OPENFILENAME

                push ebx
                mov eax, [msg]
                push .exit_retn ; Return address
                cmp eax, WM_CREATE
                jz .WM_CREATE
                cmp eax, WM_GETMINMAXINFO
                jz .WM_GETMINMAXINFO
                cmp eax, WM_DPICHANGED
                jz .WM_DPICHANGED
                cmp eax, WM_SIZE
                jz .WM_SIZE
                cmp eax, WM_SYSCOMMAND
                jz .WM_SYSCOMMAND
                cmp eax, WM_COMMAND
                jz .WM_COMMAND
                cmp eax, WM_NOTIFY
                jz .WM_NOTIFY
                cmp eax, WM_NUDGE
                jz .WM_NUDGE
                cmp eax, WM_TRAY
                jz .WM_TRAY
                cmp eax, WM_WINBIO
                jz .WM_WINBIO
                cmp eax, WM_CLOSE
                jz .WM_CLOSE
                cmp eax, WM_DESTROY
                jz .WM_DESTROY
.default:       pop eax
                pop ebx
                push [lParam]
                push [wParam]
                push [msg]
                push [hwnd]
                call [DefWindowProcA]
                ret

;-------------------------------------------------------------------------;

.get_ebx_ctx:   push GWL_USERDATA
                push [hwnd]
                call [GetWindowLongA]
                test eax, eax
                mov ebx, eax
                jz @f
                retn
@@:             push MB_ICONERROR
                push gui.szError
                push gui.szWindowContextMissing
                push [hwnd]
                call [MessageBoxA]
                push [hwnd]
                call [DestroyWindow]
                retn

;-------------------------------------------------------------------------;

.WM_CREATE:     push edi
                push -1 ; return value

                ; Allocate context
                xor edi, edi
                push sizeof.CONTEXT
                push dword [esp]
                push edi
                call [GetProcessHeap]
                push eax
                call [HeapAlloc]
                test eax, eax
                pop ecx
                mov ebx, eax
                mov eax, gui.szOutOfMemoryError
                jz .create_fail
                xor eax, eax
                mov edi, ebx
                rep stosb

                ; lParam from CreateWindowEx
                mov eax, [lParam]
                push [eax + CREATESTRUCT.lpCreateParams]
                pop [ebx + CONTEXT.pSettings]

                ; Store it in GWL_USERDATA
                mov eax, [hwnd]
                push ebx
                push GWL_USERDATA
                push eax
                mov [ebx + CONTEXT.win.hwnd], eax
                call [SetWindowLongA]

                push NULL
                call [GetModuleHandleA]
                mov [ebx + CONTEXT.hInst], eax
                push DEFAULT_GUI_FONT
                call [GetStockObject]
                mov [ebx + CONTEXT.hFont], eax

                ; Create statusbar
                lea edi, [buf]
                mov eax, [ebx + CONTEXT.pSettings]
                lea esi, [eax + SETTINGS.vol.wszDeviceInstanceId]
                mov ecx, MAX_PATH
                push edi
@@:             lodsw
                test ax, ax
                jz @f
                stosb
                loop @b
@@:             rep stosb
                pop eax

                push ecx
                push [ebx + CONTEXT.hInst]
                push ecx
                push [hwnd]
                push ecx
                push ecx
                push ecx
                push ecx
                push WS_CHILD or WS_VISIBLE
                ;push gui.szInfo
                push eax
                push gui.szStatusClass
                push ecx
                call [CreateWindowExA]
                mov [ebx + CONTEXT.hwndStatus], eax

                ; Create menu
                call [CreateMenu]
                mov [ebx + CONTEXT.hMenu], eax

                call [CreatePopupMenu]
                mov [ebx + CONTEXT.hOptionsMenu], eax
                push gui.szPreferences
                push IDM_PREFERENCES
                push MF_STRING or MF_GRAYED
                push [ebx + CONTEXT.hOptionsMenu]
                call [AppendMenuA]
                push NULL
                push 0
                push MF_SEPARATOR
                push [ebx + CONTEXT.hOptionsMenu]
                call [AppendMenuA]
                push gui.szUninstall
                push IDM_UNINSTALL
                push MF_STRING
                push [ebx + CONTEXT.hOptionsMenu]
                call [AppendMenuA]
                push gui.szQuit
                push IDM_QUIT
                push MF_STRING
                push [ebx + CONTEXT.hOptionsMenu]
                call [AppendMenuA]
                push gui.szOptions
                push [ebx + CONTEXT.hOptionsMenu]
                push MF_STRING or MF_POPUP
                push [ebx + CONTEXT.hMenu]
                call [AppendMenuA]

                call [CreatePopupMenu]
                mov [ebx + CONTEXT.hHelpMenu], eax
                push gui.szGitHub
                push IDM_GITHUB
                push MF_STRING
                push [ebx + CONTEXT.hHelpMenu]
                call [AppendMenuA]
                push NULL
                push 0
                push MF_SEPARATOR
                push [ebx + CONTEXT.hHelpMenu]
                call [AppendMenuA]
                push gui.szAbout
                push IDM_ABOUT
                push MF_STRING
                push [ebx + CONTEXT.hHelpMenu]
                call [AppendMenuA]
                push gui.szHelp
                push [ebx + CONTEXT.hHelpMenu]
                push MF_STRING or MF_POPUP
                push [ebx + CONTEXT.hMenu]
                call [AppendMenuA]

                push [ebx + CONTEXT.hMenu]
                push [ebx + CONTEXT.win.hwnd]
                call [SetMenu]

                call [CreatePopupMenu]
                mov [ebx + CONTEXT.hTrayMenu], eax
                push gui.szQuit
                push IDM_QUIT
                push MF_STRING
                push [ebx + CONTEXT.hTrayMenu]
                call [AppendMenuA]

                ; Get client area of our window ...
                sub esp, 4*4
                push esp
                push [ebx + CONTEXT.win.hwnd]
                call [GetClientRect]
                test eax, eax
                pop ecx ; left
                pop edx ; top
                jz .rect_bad
                sub dword [esp], ecx   ; right - left
                sub dword [esp+4], edx ; bottom - top
                jmp .rect_ok
.rect_bad:      mov dword [esp], WIN_WIDTH     ; includes nonclient area
                mov dword [esp+4], WIN_HEIGHT
.rect_ok:       pop [ebx + CONTEXT.win.pos.cx] ; right
                pop [ebx + CONTEXT.win.pos.cy] ; bottom

                ; ... to calcluate child window positions
                call CalculateWindowPositions

                ; List
                push NULL
                push [ebx + CONTEXT.hInst]
                push IDC_LIST
                push [hwnd]
                push [ebx + CONTEXT.winList.pos.cy]
                push [ebx + CONTEXT.winList.pos.cx]
                push [ebx + CONTEXT.winList.pos.y]
                push [ebx + CONTEXT.winList.pos.x]
                push WS_CHILD or WS_VISIBLE or WS_BORDER or \
                     WS_GROUP or WS_TABSTOP or \
                     LVS_REPORT or LVS_EDITLABELS or \
                     LVS_SINGLESEL or LVS_SHOWSELALWAYS
                push gui.szEmpty
                push gui.szListViewClass
                push 0
                call [CreateWindowExA]
                mov [ebx + CONTEXT.winList.hwnd], eax
                push 0
                push [ebx + CONTEXT.hFont]
                push WM_SETFONT
                push eax
                call [SendMessageA]

                push LVS_EX_FULLROWSELECT or LVS_EX_GRIDLINES
                push dword [esp]
                push LVM_SETEXTENDEDLISTVIEWSTYLE
                push [ebx + CONTEXT.winList.hwnd]
                call [SendMessageA]

                ; Info box
                push NULL
                push [ebx + CONTEXT.hInst]
                push NULL
                push [hwnd]
                push [ebx + CONTEXT.winInfo.pos.cy]
                push [ebx + CONTEXT.winInfo.pos.cx]
                push [ebx + CONTEXT.winInfo.pos.y]
                push [ebx + CONTEXT.winInfo.pos.x]
                push WS_CHILD or WS_VISIBLE or BS_GROUPBOX
                push gui.szEmpty
                push gui.szButtonClass
                push 0
                call [CreateWindowExA]
                mov [ebx + CONTEXT.winInfo.hwnd], eax
                push 0
                push [ebx + CONTEXT.hFont]
                push WM_SETFONT
                push eax
                call [SendMessageA]

                ; File controls
                push NULL
                push [ebx + CONTEXT.hInst]
                push IDC_PATH
                push [hwnd]
                push [ebx + CONTEXT.winPath.pos.cy]
                push [ebx + CONTEXT.winPath.pos.cx]
                push [ebx + CONTEXT.winPath.pos.y]
                push [ebx + CONTEXT.winPath.pos.x]
                push WS_CHILD or WS_VISIBLE or WS_TABSTOP or ES_AUTOHSCROLL
                push gui.szEmpty
                push gui.szEditClass
                push WS_EX_CLIENTEDGE
                call [CreateWindowExA]
                mov [ebx + CONTEXT.winPath.hwnd], eax
                push 0
                push [ebx + CONTEXT.hFont]
                push WM_SETFONT
                push eax
                call [SendMessageA]

                push gui.wszPathCue
                push FALSE
                push EM_SETCUEBANNER  ; not supported by wine
                push [ebx + CONTEXT.winPath.hwnd]
                call [SendMessageA]

                push 0
                push MAX_PATH-1
                push EM_LIMITTEXT
                push [ebx + CONTEXT.winPath.hwnd]
                call [SendMessageA]

                push SHACF_AUTOSUGGEST_FORCE_ON or SHACF_FILESYSTEM + \
                      SHACF_USETAB
                push [ebx + CONTEXT.winPath.hwnd]
                call [SHAutoComplete]

                push NULL
                push [ebx + CONTEXT.hInst]
                push IDC_BROWSE
                push [hwnd]
                push [ebx + CONTEXT.winBrowse.pos.cy]
                push [ebx + CONTEXT.winBrowse.pos.cx]
                push [ebx + CONTEXT.winBrowse.pos.y]
                push [ebx + CONTEXT.winBrowse.pos.x]
                push WS_CHILD or WS_VISIBLE or WS_TABSTOP or BS_ICON
                push gui.szBrowse
                push gui.szButtonClass
                push 0
                call [CreateWindowExA]
                mov [ebx + CONTEXT.winBrowse.hwnd], eax

                mov ecx, sizeof.SHFILEINFO
                xor eax, eax
                sub esp, ecx
                mov edi, esp
                push 113h ; SHGFI_{ICON,SMALLICON,OPENICON,USEFILEATTRIBUTES}
                push ecx
                push edi
                push FILE_ATTRIBUTE_DIRECTORY
                push gui.szBrowse
                rep stosb
                call [SHGetFileInfo]
                mov eax, [esp + SHFILEINFO.hIcon]
                mov esp, edi

                push eax
                push IMAGE_ICON
                push BM_SETIMAGE
                push [ebx + CONTEXT.winBrowse.hwnd]
                call [SendMessageA]

                ; Enroll button
                push NULL
                push [ebx + CONTEXT.hInst]
                push IDC_ENROLL
                push [hwnd]
                push [ebx + CONTEXT.winEnroll.pos.cy]
                push [ebx + CONTEXT.winEnroll.pos.cx]
                push [ebx + CONTEXT.winEnroll.pos.y]
                push [ebx + CONTEXT.winEnroll.pos.x]
                push WS_CHILD or WS_VISIBLE or WS_TABSTOP; or WS_DISABLED
                push gui.szEnroll
                push gui.szButtonClass
                push 0
                call [CreateWindowExA]
                mov [ebx + CONTEXT.winEnroll.hwnd], eax
                push 0
                push [ebx + CONTEXT.hFont]
                push WM_SETFONT
                push eax
                call [SendMessageA]

                ; Status static text
                push NULL
                push [ebx + CONTEXT.hInst]
                push IDC_STATUS
                push [hwnd]
                push [ebx + CONTEXT.winStatus.pos.cy]
                push [ebx + CONTEXT.winStatus.pos.cx]
                push [ebx + CONTEXT.winStatus.pos.y]
                push [ebx + CONTEXT.winStatus.pos.x]
                push WS_CHILD or WS_VISIBLE or SS_ENDELLIPSIS
                push gui.szStatusDefault
                push gui.szStaticClass
                push 0
                call [CreateWindowExA]
                mov [ebx + CONTEXT.winStatus.hwnd], eax
                push 0
                push [ebx + CONTEXT.hFont]
                push WM_SETFONT
                push eax
                call [SendMessageA]

                ; Fill fingerprint/commant listview
                call InitFingerList

                ; Make it focused
                push [ebx + CONTEXT.winList.hwnd]
                call [SetFocus]

                ; And show details of the current selection
                call UpdateSelection

                ; Get window DPI
                push [hwnd]
                call [GetDpiForWindow]
                mov [ebx + CONTEXT.dwDPI], eax

                ; Open async WinBio framework for enumerating biometric units
                lea eax, [ebx + CONTEXT.hFramework]
                push eax
                push TRUE
                push ebx
                push NULL
                push WM_WINBIO
                push [hwnd]
                push WINBIO_ASYNC_NOTIFY_MESSAGE
                call [WinBioAsyncOpenFramework]

                test eax, eax
                mov eax, gui.szFrameworkError
                jz .create_ok
                mov [ebx + CONTEXT.dwState], STATE_ERROR

.create_fail:   push MB_ICONERROR or MB_OK
                push gui.szError
                push eax
                push [hwnd]
                call [MessageBoxA]
                jmp @f
.create_ok:     mov [ebx + CONTEXT.dwState], STATE_INITIALIZING
                inc dword [esp] ; +1
                inc dword [esp] ;  0
@@:             pop eax         ; -1
                pop edi
                pop ebx
                pop ebx
                ret

;-------------------------------------------------------------------------;

.WM_GETMINMAXINFO:
                mov eax, [lParam]
                test eax, eax
                jz .exit
                mov ebx, eax
                push [hwnd]
                call [GetDpiForWindow]

                push 96
                push eax
                push 96
                push eax

                push WIN_WIDTH_MIN
                call [MulDiv]
                mov [ebx + MINMAXINFO.ptMinTrackSize.x], eax

                push WIN_HEIGHT_MIN
                call [MulDiv]
                mov [ebx + MINMAXINFO.ptMinTrackSize.y], eax

                retn

;-------------------------------------------------------------------------;

.WM_DPICHANGED: call .get_ebx_ctx
                push [hwnd]
                call [GetDpiForWindow]
                mov [ebx + CONTEXT.dwDPI], eax
                retn

.WM_SIZE:       call .get_ebx_ctx
                push edi
                push esi
                mov eax, [lParam]
                movzx ecx, ax
                shr eax, 16
                mov [ebx + CONTEXT.win.pos.cx], ecx
                mov [ebx + CONTEXT.win.pos.cy], eax

                call CalculateWindowPositions

                lea esi, [ebx + CONTEXT_FIRST_WINDOW_OFFSET]
                push CONTEXT_WINDOW_COUNT
                pop edi

.resize_window: cmp [esi + WIN.hwnd], 0
                jz .skip_null_win

                push FALSE

                push 96
                push [ebx + CONTEXT.dwDPI]
                push [esi + WIN.pos.cy]
                call [MulDiv]
                push eax

                push 96
                push [ebx + CONTEXT.dwDPI]
                push [esi + WIN.pos.cx]
                call [MulDiv]
                push eax

                push 96
                push [ebx + CONTEXT.dwDPI]
                push [esi + WIN.pos.y]
                call [MulDiv]
                push eax

                push 96
                push [ebx + CONTEXT.dwDPI]
                push [esi + WIN.pos.x]
                call [MulDiv]
                push eax

                push [esi + WIN.hwnd]
                call [MoveWindow]

.skip_null_win: add esi, sizeof.WIN
                dec edi
                jnz .resize_window

                push edi
                push edi
                push WM_SIZE
                push [ebx + CONTEXT.hwndStatus]
                call [SendMessageA]

                call ResizeColumns

                push RDW_INVALIDATE
                push edi
                push edi
                push [ebx + CONTEXT.win.hwnd]
                call [RedrawWindow]

                pop esi
                pop edi
                retn

;-------------------------------------------------------------------------;

.WM_SYSCOMMAND: mov eax, [wParam]
                cmp eax, SC_MINIMIZE
                jnz .default
                call .get_ebx_ctx
                push [hwnd]
                call ShowTrayIcon
                test eax, eax
                jz .default
                push SW_HIDE
                push [hwnd]
                call [ShowWindow]
                push 1
                mov eax, [ebx + CONTEXT.pSettings]
                pop [eax + SETTINGS.GUI.dwMinimized]
                retn

;-------------------------------------------------------------------------;

.WM_COMMAND:    mov eax, [wParam]
                cmp eax, IDC_PATH + (EN_CHANGE shl 16)
                jz .IDC_PATH
                cmp eax, IDC_BROWSE + (BN_CLICKED shl 16)
                jz .IDC_BROWSE
                cmp eax, IDC_ENROLL + (BN_CLICKED shl 16)
                jz .IDC_ENROLL

                cmp ax, IDM_PREFERENCES
                jz .IDM_PREFERENCES
                cmp ax, IDM_UNINSTALL
                jz .IDM_UNINSTALL
                cmp ax, IDM_QUIT
                jz .IDM_QUIT
                cmp ax, IDM_GITHUB
                jz .IDM_GITHUB
                cmp ax, IDM_ABOUT
                jz .IDM_ABOUT

                jmp .exit

.IDC_PATH:      call .get_ebx_ctx
                push esi

                push [lParam]
                lea esi, [buf]
                call [GetWindowTextLengthA]
                inc eax           ; +1 for NULL byte
                mov ecx, MAX_PATH ; sizeof.buf
                cmp eax, ecx
                jbe .path_buf_ok
                xor esi, esi
                push eax
                push esi
                call [GetProcessHeap]
                push eax
                call [HeapAlloc]
                test eax, eax
                pop esi
                jz .path_fail
                xchg eax, esi

.path_buf_ok:   push eax
                push esi
                push [lParam]
                call [GetWindowTextA]

                push LVNI_SELECTED
                push -1
                push LVM_GETNEXTITEM
                push [ebx + CONTEXT.winList.hwnd]
                call [SendMessageA]
                test eax, eax
                js .path_fail

                push esi
                push eax
                call SetCommand

                lea eax, [buf]
                cmp esi, eax
                jz .path_changed
                push esi
                push 0
                call [GetProcessHeap]
                push eax
                call [HeapFree]
.path_fail:
.path_changed:  pop esi
                retn

.IDC_BROWSE:    call .get_ebx_ctx

                xor eax, eax
                push edi
                lea edi, [ofn]
                push edi ; for GetOpenFileNameA

                mov ecx, sizeof.OPENFILENAME
                push [hwnd]
                push [ebx + CONTEXT.hInst]
                push ecx
                rep stosb
                pop [ofn.lStructSize]
                pop [ofn.hInstance]
                pop [ofn.hwndOwner]

                lea edi, [buf+1]
                mov [ofn.lpstrFile], edi
                mov [ofn.nMaxFile], MAX_PATH
                mov byte [edi], 0
                mov [ofn.lpstrFilter], gui.szBrowseFilter
                mov [ofn.nFilterIndex], 1
                mov [ofn.Flags], OFN_PATHMUSTEXIST or OFN_FILEMUSTEXIST

                call [GetOpenFileNameA]
                test eax, eax
                jnz .opened
                pop edi
                jmp .exit

.opened:        xor eax, eax
                lea ecx, [eax-1] ; -1
                push edi ; for SendMessageA
                repnz scasb
                not ecx
                dec ecx ; strlen(edi)
                jz .unquoted
                mov al, ' '
                mov edi, [esp]
                repnz scasb
                jnz .unquoted
                mov byte [edi+ecx], '"'
                mov byte [edi+ecx+1], 0
                mov byte [buf], '"'
                dec dword [esp]
.unquoted:
                push 0
                push WM_SETTEXT
                push [ebx + CONTEXT.winPath.hwnd]
                call [SendMessageA]

                pop edi
                retn

.IDC_ENROLL:    call .get_ebx_ctx

                mov eax, [ebx + CONTEXT.dwState]
                test eax, STATE_ENROLLING or STATE_ENROLL_REQUESTED or STATE_IDENTIFYING
                jz .start_enroll

                or [ebx + CONTEXT.dwState], STATE_ENROLL_REQUESTED
                push [ebx + CONTEXT.hSession]
                call [WinBioCancel]
                retn

.start_enroll:  push LVNI_SELECTED
                push -1
                push LVM_GETNEXTITEM
                push [ebx + CONTEXT.winList.hwnd]
                call [SendMessageA]
                test eax, eax
                js .enroll_ret
                cmp eax, 10
                jae .enroll_ret
                inc eax ; add eax, WINBIO_ANSI_381_POS_RH_THUMB (1)
                push [ebx + CONTEXT.dwUnitId]
                push eax
                push [ebx + CONTEXT.hSession]
                call [WinBioEnrollBegin]
                test eax, eax
                jz .enrolling

                push eax
                push STATUS_ERROR
                call SetStatus
                retn

.enrolling:     or [ebx + CONTEXT.dwState], STATE_ENROLLING
.enroll_ret:    retn

.IDM_PREFERENCES:
                ; UNIMPLEMENTED
                retn

.IDM_UNINSTALL: push [hwnd]
                push gui.wszUninstall
                call RunAsAdmin
                test eax, eax
                jz .uninstalled
                push MB_ICONERROR or MB_OK
                push gui.szError
                push gui.szUninstallError
                push [hwnd]
                call [MessageBoxA]
                retn
.uninstalled:   call .get_ebx_ctx
                test ebx, ebx
                jz @f
                mov eax, [ebx + CONTEXT.pSettings]
                test eax, eax
                jz @f
                mov word [eax + SETTINGS.vol.wszDeviceInstanceId], 0
@@:             ;push [hwnd]
                ;call [DestroyWindow]
                ;retn

.IDM_QUIT:      push [hwnd]
                call [DestroyWindow]
                retn

.IDM_GITHUB:    push SW_SHOWNORMAL
                push NULL
                push NULL
                push gui.szGitHubURL
                push gui.szOpen
                push NULL
                call [ShellExecuteA]
                retn

.IDM_ABOUT:     push NULL
                push AboutDialogProc
                push [hwnd]
                push IDD_ABOUT

                push NULL
                call [GetModuleHandleA]
                push eax

                call [DialogBoxParamA]
                retn

;-------------------------------------------------------------------------;

.WM_NOTIFY:     mov edx, [lParam]
                cmp [edx + NMHDR.idFrom], IDC_LIST
                jnz @f
                mov eax, [edx + NMHDR.code]
                cmp eax, LVN_ITEMCHANGED
                jz .LVN_ITEMCHANGED
@@:             retn

.LVN_ITEMCHANGED:
                call .get_ebx_ctx
                call UpdateSelection
                retn

;--------------------------------------------------------------------------;

.WM_NUDGE:      push [hwnd]
                push [hwnd]
                push SW_SHOW
                push [hwnd]
                call [ShowWindow]
                call [SetForegroundWindow]
                call [SetFocus]
                call .get_ebx_ctx
                mov eax, [ebx + CONTEXT.pSettings]
                cmp [eax + SETTINGS.GUI.dwMinimized], 0
                jz @f
                push 0
                pop [eax + SETTINGS.GUI.dwMinimized]
                push [hwnd]
                call HideTrayIcon
@@:             retn

;--------------------------------------------------------------------------;

.WM_TRAY:       mov edx, [lParam]
                cmp edx, WM_LBUTTONDBLCLK ;WM_LBUTTONDOWN
                jnz @f
                call .get_ebx_ctx
                push [ebx + CONTEXT.win.hwnd]
                call HideTrayIcon
                mov eax, [ebx + CONTEXT.pSettings]
                mov [eax + SETTINGS.GUI.dwMinimized], 0
                push SW_SHOW
                push [ebx + CONTEXT.win.hwnd]
                call [ShowWindow]
                retn
@@:
                cmp edx, WM_RBUTTONDOWN
                jnz .default
                call .get_ebx_ctx
                sub esp, sizeof.POINT
                push esp
                call [GetCursorPos]
                test eax, eax
                jnz @f
                mov [esp + POINT.x], eax
                mov [esp + POINT.y], eax
@@:             mov eax, esp
                push NULL
                push [ebx + CONTEXT.win.hwnd]
                push 0
                push [eax + POINT.y]
                push [eax + POINT.x]
                push TPM_BOTTOMALIGN + TPM_LEFTBUTTON + TPM_RIGHTBUTTON
                push [ebx + CONTEXT.hTrayMenu]
                call [TrackPopupMenu]

                add esp, sizeof.POINT
                retn

;--------------------------------------------------------------------------;

.WM_WINBIO:     push esi
                mov esi, [lParam]
                test esi, esi
                jz .winbio_exit
                call .get_ebx_ctx

                mov eax, [esi + WINBIO_ASYNC_RESULT.dwOperation]
                cmp eax, 07fh
                ja .winbio_done

                cmp al, WINBIO_OPERATION_OPEN
                jnz .id
                mov eax, [esi + WINBIO_ASYNC_RESULT.hSessionHandle]
                mov ebx, [esi + WINBIO_ASYNC_RESULT.pUserData]
                test eax, eax
                mov [ebx + CONTEXT.hSession], eax
                jz .session_error
                and [ebx + CONTEXT.dwState], STATE_INITIALIZING xor -1
                call .start_identify

                ;cmp eax, WINBIO_RPC_X_SS_IN_NULL_CONTEXT
                ; TODO: WbioSvc restart solves this...

                jmp .winbio_done

.session_error: or [ebx + CONTEXT.dwState], STATE_ERROR
                push MB_ICONERROR or MB_OK
                push gui.szError
                push gui.szSessionError
                push [hwnd]
                call [MessageBoxA]
                push [hwnd]
                call [DestroyWindow]
                jmp .winbio_done

.id:            cmp al, WINBIO_OPERATION_IDENTIFY
                jnz .enroll_begin

                mov eax, [esi + WINBIO_ASYNC_RESULT.dwApiStatus]
                test eax, eax
                ;mov ecx, [esi + WINBIO_ASYNC_RESULT.Identify.dwSubFactor]
                ;mov ecx, [esi + WINBIO_ASYNC_RESULT.Identify.dwRejectDetail]
                jz .identified
                and [ebx + CONTEXT.dwState], STATE_IDENTIFYING xor -1
                cmp eax, WINBIO_E_UNKNOWN_ID
                jz .id_retry
                cmp eax, WINBIO_E_BAD_CAPTURE
                jz .id_retry
                cmp eax, WINBIO_E_CANCELED
                jnz .id_error

                test [ebx + CONTEXT.dwState], STATE_ENROLLING
                jnz .winbio_done
                push .winbio_done
                test [ebx + CONTEXT.dwState], STATE_ENROLL_REQUESTED
                jnz .start_enroll
                retn

.id_error:      sub esp, 60h
                or [ebx + CONTEXT.dwState], STATE_ERROR
                mov ecx, esp

                push eax
                push gui.szIdentifyError
                push ecx
                call [wsprintfA]

                push MB_ICONERROR
                push gui.szError
                lea eax, [esp+4*3+4*2]
                push eax
                push [ebx + CONTEXT.win.hwnd]
                call [MessageBoxA]

                add esp, 60h + 4*3
                jmp .winbio_done

.identified:    mov eax, [esi + WINBIO_ASYNC_RESULT.Identify.dwSubFactor]
                push eax
                push eax
                call SetSelection
                call RunSubFactorCommand

.id_retry:      call .start_identify
                jmp .winbio_done

.enroll_begin:  cmp al, WINBIO_OPERATION_ENROLL_BEGIN
                jnz .capture
                push STATUS_ENROLLING
                call SetStatus
                lea eax, [buf]
                push eax
                push [ebx + CONTEXT.hSession]
                call [WinBioEnrollCapture]
                jmp .winbio_done
.capture:       cmp al, WINBIO_OPERATION_ENROLL_CAPTURE
                jnz .commit
                mov eax, [esi + WINBIO_ASYNC_RESULT.dwApiStatus]
                test eax, eax
                jz .enroll_ok
                cmp eax, WINBIO_E_CANCELED
                jz .enroll_cancel
                cmp eax, WINBIO_E_BAD_CAPTURE
                jz .capture_more
                cmp eax, WINBIO_I_MORE_DATA
                jnz .enroll_err
.capture_more:  push STATUS_ENROLLING
                call SetStatus
                lea eax, [buf]
                push eax
                push [ebx + CONTEXT.hSession]
                call [WinBioEnrollCapture]
                jmp .winbio_done

.enroll_cancel: push [ebx + CONTEXT.hSession]
                call [WinBioEnrollDiscard]
                jmp .winbio_done

.enroll_err:    or [ebx + CONTEXT.dwState], STATE_ERROR
                push eax
                push [ebx + CONTEXT.hSession]
                call [WinBioEnrollDiscard]
                push STATUS_ERROR
                call SetStatus
                jmp .winbio_done

.enroll_ok:     sub esp, sizeof.WINBIO_IDENTITY + 4
                lea eax, [esp+4]
                push esp
                push eax
                push [ebx + CONTEXT.hSession]
                call [WinBioEnrollCommit]
                test eax, eax
                lea esp, [esp + sizeof.WINBIO_IDENTITY + 4]
                jnz @f
                push STATUS_COMMITTING
                call SetStatus
                jmp .winbio_done
@@:             or [ebx + CONTEXT.dwState], STATE_ERROR
                push eax
                push STATUS_ERROR
                call SetStatus
                jmp .winbio_done

.commit:        cmp al, WINBIO_OPERATION_ENROLL_COMMIT
                jnz .discard
                and [ebx + CONTEXT.dwState], STATE_ENROLLING xor -1

                mov ecx, [esi + WINBIO_ASYNC_RESULT.dwApiStatus]
                mov al, STATUS_COMMITTED
                jecxz @f
                push ecx
                mov al, STATUS_ERROR
                or [ebx + CONTEXT.dwState], STATE_ERROR
@@:             push eax
                call SetStatus
                jmp .ack_req
.discard:       cmp al, WINBIO_OPERATION_ENROLL_DISCARD
                jnz .enum_units
                and [ebx + CONTEXT.dwState], STATE_ENROLLING xor -1
                push STATUS_DISCARDED
                call SetStatus
.ack_req:       test [ebx + CONTEXT.dwState], STATE_ENROLL_REQUESTED
                jz .winbio_done
                and [ebx + CONTEXT.dwState], STATE_ENROLL_REQUESTED xor -1
                call .start_identify
                jmp .winbio_done

.enum_units:    cmp al, WINBIO_OPERATION_ENUM_BIOMETRIC_UNITS
                jnz .open_fw
                cmp [esi + WINBIO_ASYNC_RESULT.dwApiStatus], 0
                jnz .units_error

                xor ecx, ecx
.next_unit:     cmp ecx, [esi + WINBIO_ASYNC_RESULT.EnumBiometricUnits.dwUnitCount]
                jae .sensor_404

                imul eax, ecx, sizeof.WINBIO_UNIT_SCHEMA
                add eax, [esi + WINBIO_ASYNC_RESULT.EnumBiometricUnits.pUnitSchemaArray]

                push eax
                push ecx
                lea eax, [eax + WINBIO_UNIT_SCHEMA.wszDeviceInstanceId]
                mov ecx, [ebx + CONTEXT.pSettings]
                lea ecx, [ecx + SETTINGS.vol.wszDeviceInstanceId]
                push eax
                push ecx
                call [lstrcmpiW]
                pop ecx
                inc ecx
                test eax, eax
                pop eax
                jnz .next_unit

                push [eax + WINBIO_UNIT_SCHEMA.dwUnitId]
                pop [ebx + CONTEXT.dwUnitId]

                ; Open async WinBio session
                lea eax, [ebx + CONTEXT.hSession]
                push eax
                push TRUE
                push ebx
                push NULL
                push WM_WINBIO
                push [hwnd]
                push WINBIO_ASYNC_NOTIFY_MESSAGE
                push guidDatabase
                push 1
                lea eax, [ebx + CONTEXT.dwUnitId]
                push eax
                push WINBIO_FLAG_ADVANCED
                push WINBIO_POOL_PRIVATE
                push WINBIO_TYPE_FINGERPRINT
                call [WinBioAsyncOpenSession]
                test eax, eax
                jz .winbio_done
                or [ebx + CONTEXT.dwState], STATE_ERROR

                push MB_ICONERROR or MB_OK
                push gui.szError
                push gui.szSessionError
                push [hwnd]
                call [MessageBoxA]
                jmp .winbio_done

.sensor_404:    push NULL
                push NULL
                push WM_NUDGE
                push [hwnd]
                call [SendMessageA]

                push MB_ICONERROR or MB_OK
                push gui.szError
                push gui.szSensor404Error
                push [hwnd]
                call [MessageBoxA]

.units_error:   or [ebx + CONTEXT.dwState], STATE_ERROR
                jmp .winbio_done

.open_fw:       cmp al, WINBIO_OPERATION_OPEN_FRAMEWORK
                jnz .winbio_done

                mov eax, [esi + WINBIO_ASYNC_RESULT.hSessionHandle]
                mov ebx, [esi + WINBIO_ASYNC_RESULT.pUserData]
                test eax, eax
                mov [ebx + CONTEXT.hFramework], eax
                jz .fw_error

                push WINBIO_TYPE_FINGERPRINT
                push eax
                call [WinBioAsyncEnumBiometricUnits]
                test eax, eax
                jz .winbio_done
.fw_error:      or [ebx + CONTEXT.dwState], STATE_ERROR
                jmp .winbio_done

.winbio_done:   push esi
                call [WinBioFree]
.winbio_exit:   pop esi
                retn

.start_identify:
                sub esp, sizeof.WINBIO_IDENTITY + 4 + 4

                ; Not needed as these are used only as output variables
                ;mov dword [esp + WINBIO_IDENTITY.dwType], WINBIO_ID_TYPE_WILDCARD
                ;mov dword [esp + WINBIO_IDENTITY.dwWildCard], WINBIO_ID_TYPE_WILDCARD

                lea eax, [esp + sizeof.WINBIO_IDENTITY + 4]
                lea ecx, [esp + sizeof.WINBIO_IDENTITY]
                mov edx, esp

                push eax ; pRejectDetail
                push ecx ; pSubFactor
                push edx ; pIdentity
                lea eax, [ebx + CONTEXT.dwUnitId]
                push eax ; pUnitId
                push [ebx + CONTEXT.hSession] ; SessionHandle
                call [WinBioIdentify]
                test eax, eax
                lea esp, [esp + sizeof.WINBIO_IDENTITY + 4 + 4]
                jz @f

                and [ebx + CONTEXT.dwState], STATE_IDENTIFYING xor -1
                or [ebx + CONTEXT.dwState], STATE_ERROR
                retn
@@:             or [ebx + CONTEXT.dwState], STATE_IDENTIFYING
                retn

;-------------------------------------------------------------------------;

.WM_CLOSE:      push [hwnd]
                call [DestroyWindow]
                retn

;-------------------------------------------------------------------------;

.WM_DESTROY:    push GWL_USERDATA
                push [hwnd]
                call [GetWindowLongA]
                test eax, eax
                mov ebx, eax
                jz .context_freed
                mov eax, [ebx + CONTEXT.pSettings]
                call SaveSettings

.free_context:  mov eax, [ebx + CONTEXT.hFramework]
                test eax, eax
                jz @f
                push eax
                call [WinBioCloseFramework]
@@:             mov eax, [ebx + CONTEXT.hSession]
                test eax, eax
                jz @f
                push eax
                call [WinBioCloseSession]
@@:
                mov eax, [ebx + CONTEXT.hTrayMenu]
                test eax, eax
                jz @f
                push eax
                call [DestroyMenu]
@@:
                mov eax, [ebx + CONTEXT.pSettings]
                cmp [eax + SETTINGS.GUI.dwMinimized], 0
                jz @f
                push [hwnd]
                call HideTrayIcon
@@:
                push ebx
                push 0
                call [GetProcessHeap]
                push eax
                call [HeapFree]

.context_freed: push 0
                call [PostQuitMessage]
                retn

;-------------------------------------------------------------------------;

.exit:          pop eax
.exit_retn:     xor eax, eax
                pop ebx
                ret
endp

proc AboutDialogProc hdlg:DWORD, msg:DWORD, wParam:DWORD, lParam:DWORD
                mov eax, [msg]
                cmp eax, WM_COMMAND
                jz .close
                cmp eax, WM_CLOSE
                jz .close
                xor eax, eax
                ret

.close:         push 0
                push [hdlg]
                call [EndDialog]
                push 1
                pop eax
                ret
endp

; Create a tray icon. The caller should hide the main window.
; Returns eax != 0 if the tray icon was created successfully
proc ShowTrayIcon hwnd:DWORD
                local nid:NOTIFYICONDATAA
                push esi
                push edi

                push sizeof.NOTIFYICONDATAA
                pop ecx
                lea edi, [nid]
                push ecx
                rep stosb
                pop [nid.cbSize]

                push [hwnd]
                pop [nid.hWnd]
                mov [nid.uID], TRAY_ID
                mov [nid.uCallbackMessage], WM_TRAY

                push IDI_MAIN
                push NULL
                call [GetModuleHandleA]
                push eax
                call [LoadIconA]
                mov [nid.hIcon], eax

                push gui.szTip.size
                pop ecx
                mov esi, gui.szTip
                lea edi, [nid.szTip]
                rep movsb

                mov cl, NIF_ICON or NIF_MESSAGE or NIF_TIP
                mov [nid.uFlags], ecx

                lea eax, [nid]
                push eax
                push NIM_ADD
                call [Shell_NotifyIconA]
                pop edi
                pop esi
                ret
endp

proc HideTrayIcon hwnd:DWORD
                mov ecx, sizeof.NOTIFYICONDATAA/4
@@:             push 0
                loop @b

                mov [esp + NOTIFYICONDATAA.cbSize], sizeof.NOTIFYICONDATAA
                mov eax, [hwnd]
                mov [esp + NOTIFYICONDATAA.hWnd], eax
                mov [esp + NOTIFYICONDATAA.uID], TRAY_ID

                push esp
                push NIM_DELETE
                call [Shell_NotifyIconA]

                add esp, sizeof.NOTIFYICONDATAA
                ret
endp

;-------------------------------------------------------------------------;
; Helper functions below all expect ebx = CONTEXT!
; - CalculateWindowPositions
; - StatusBarHeight
; - ResizeColumns
; - InitFingerList
; - SetCommand
; - GetCommand
; - RunSubFactorCommand
; - UpdateSelection
; - SetStatus
; - SaveSettings
;-------------------------------------------------------------------------;
CalculateWindowPositions:
                mov eax, [ebx + CONTEXT.dwDPI]
                test eax, eax
                jnz @f
                call [GetDpiForSystem]
                mov [ebx + CONTEXT.dwDPI], eax

@@:             push eax;[ebx + CONTEXT.dwDPI]
                push 96
                push [ebx + CONTEXT.win.pos.cy]
                call [MulDiv]
                push eax

                push [ebx + CONTEXT.dwDPI]
                push 96
                push [ebx + CONTEXT.win.pos.cx]
                call [MulDiv]
                push eax

                call .status_bar_height
                push [ebx + CONTEXT.dwDPI]
                push 96
                push eax
                call [MulDiv]

                pop ecx
                pop edx

                sub edx, eax

                virtual at ebx
                  .ctx CONTEXT
                end virtual

                ; List
                lea eax, [ecx-10]
                mov [.ctx.winList.pos.cx], eax
                lea eax, [edx-20-24-25-1-10-5]
                mov [.ctx.winList.pos.cy], eax
                mov [.ctx.winList.pos.x], 5
                mov [.ctx.winList.pos.y], 5

                ; Info
                lea eax, [ecx-10]
                mov [.ctx.winInfo.pos.cx], eax
                lea eax, [edx-20-24-25-1-8]
                mov [.ctx.winInfo.pos.y], eax
                neg eax
                lea eax, [eax+edx-5]
                mov [.ctx.winInfo.pos.cy], eax
                mov [.ctx.winInfo.pos.x], 5

                ; Path
                mov [.ctx.winPath.pos.cy], 20
                lea eax, [ecx-68+5]
                mov [.ctx.winPath.pos.cx], eax
                lea eax, [edx-15-20-25]
                mov [.ctx.winPath.pos.y], eax
                mov [.ctx.winPath.pos.x], 15

                ; Browse
                mov [.ctx.winBrowse.pos.cy], 20+1+1
                mov [.ctx.winBrowse.pos.cx], 35
                lea eax, [edx-15-20-25-1]
                mov [.ctx.winBrowse.pos.y], eax
                lea eax, [ecx-68 + 20]
                mov [.ctx.winBrowse.pos.x], eax

                ; Enroll
                mov [.ctx.winEnroll.pos.cy], 24
                mov [.ctx.winEnroll.pos.cx], 74
                lea eax, [edx-20-15]
                mov [.ctx.winEnroll.pos.y], eax
                mov [.ctx.winEnroll.pos.x], 15

                ; Status
                add eax, 6
                mov [.ctx.winStatus.pos.x], 93
                mov [.ctx.winStatus.pos.y], eax
                lea eax, [ecx-94-10-5]
                mov [.ctx.winStatus.pos.cy], 14
                mov [.ctx.winStatus.pos.cx], eax

                ret

.status_bar_height:
                sub esp, sizeof.RECT
                virtual at esp
                  .rect RECT
                end virtual

                push esp
                push [ebx + CONTEXT.hwndStatus]
                call [GetWindowRect]

                test eax, eax
                jz .bad_statusbar
                mov eax, [.rect.bottom]
                sub eax, [.rect.top]
                jmp @f
.bad_statusbar: mov al, 20
@@:             add esp, sizeof.RECT
                retn

ResizeColumns:
                push LVSCW_AUTOSIZE_USEHEADER
                push 1
                push LVM_SETCOLUMNWIDTH
                push [ebx + CONTEXT.winList.hwnd]
                call [SendMessageA]
                ret

proc InitFingerList
                local column:LV_COLUMN
                local item:LV_ITEM

                push edi
                xor eax, eax
                lea ecx, [eax + sizeof.LV_COLUMN]
                lea edi, [column]
                rep stosb

                mov [column.mask], LVCF_TEXT+LVCF_WIDTH+LVCF_SUBITEM
                mov [column.pszText], gui.szFinger
                mov [column.cx], 52
.init_column:
                lea eax, [column]
                push eax
                push [column.iSubItem]
                push LVM_INSERTCOLUMN
                push [ebx + CONTEXT.winList.hwnd]
                call [SendMessageA]

                mov eax, gui.szCommand
                inc [column.iSubItem]
                cmp [column.pszText], eax
                mov [column.pszText], eax
                jnz .init_column

                xor eax, eax
                lea edi, [item]
                mov ecx, sizeof.LV_ITEM
                rep stosb

                xor edi, edi
                mov [item.mask], LVIF_TEXT+LVIF_STATE
                mov [item.state], LVIS_SELECTED
.init_item:     mov [item.iItem], edi
                mov [item.iSubItem], 0
                inc edi
                push edi
                call SubfactorText
                mov [item.pszText], eax

                lea eax, [item]
                push eax
                push 0
                push LVM_INSERTITEM
                push [ebx + CONTEXT.winList.hwnd]
                call [SendMessageA]

                imul eax, edi, MAX_PATH
                mov ecx, [ebx + CONTEXT.pSettings]
                lea eax, [eax + ecx + SETTINGS.cmd - MAX_PATH]
                mov [item.iSubItem], COLUMN_COMMAND
                mov [item.pszText], eax

                lea eax, [item]
                push eax
                push 0
                push LVM_SETITEM
                push [ebx + CONTEXT.winList.hwnd]
                call [SendMessageA]

                and [item.mask], -1 xor LVIF_STATE
                cmp edi, SUBFACTOR_MAX
                jb .init_item

                push LVSCW_AUTOSIZE_USEHEADER
                push 0
                push LVM_SETCOLUMNWIDTH
                push [ebx + CONTEXT.winList.hwnd]
                call [SendMessageA]

                call ResizeColumns

                pop edi
                ret
endp

proc SetCommand iItem:DWORD, szText:DWORD
                local item:LV_ITEM

                push edi
                xor eax, eax
                lea ecx, [eax + sizeof.LV_ITEM]
                lea edi, [item]
                rep stosb
                mov edi, [szText]
                push COLUMN_COMMAND
                push [iItem]
                push LVIF_TEXT
                mov [item.pszText], edi
                pop [item.mask]
                pop [item.iItem]
                pop [item.iSubItem]
                scasb
                pop edi

                lea eax, [item]
                push eax
                push 0
                push LVM_SETITEM
                push [ebx + CONTEXT.winList.hwnd]
                call [SendMessageA]

                call ResizeColumns
                ret
endp

; The return pointer should be freed with HeapFree(GetCurrenProcessHeap(), ...)
proc GetCommand iItem:DWORD
                local item:LV_ITEM

                push esi
                push edi

                xor eax, eax
                lea ecx, [eax + sizeof.LV_ITEM]
                lea edi, [item]
                push eax
                push COLUMN_COMMAND
                rep stosb
                pop [item.iSubItem]
                pop [item.pszText]

                mov edi, MAX_PATH * 2
                push edi
                push eax
                call [GetProcessHeap]
                push eax
                call [HeapAlloc]
                test eax, eax
                jz .oom
                mov esi, eax
                mov [item.pszText], eax
                mov [item.cchTextMax], edi

                lea eax, [item]
                push eax
                push [iItem]
                push LVM_GETITEMTEXT
                push [ebx + CONTEXT.winList.hwnd]
                call [SendMessageA]
                add [item.pszText], eax
                inc eax ; NULL

                push eax
                push esi
                push 0
                call [GetProcessHeap]
                push eax
                call [HeapReAlloc]
                test eax, eax
                jz .realloc_fail

.exit:          pop edi
                pop esi
                ret

.realloc_fail:  push esi
                push 0
                call [GetProcessHeap]
                push eax
                call [HeapFree]
.oom:           push MB_ICONERROR
                push gui.szError
                push gui.szOutOfMemoryError
                push [ebx + CONTEXT.win.hwnd]
                call [MessageBoxA]
                xor eax, eax
                jmp .exit
endp

proc RunSubFactorCommand dwSubFactor:DWORD
                push esi
                push edi

                mov eax, [dwSubFactor]
                dec eax
                js .exit
                cmp eax, 10
                jae .exit
                push eax
                call GetCommand
                test eax, eax
                jz .exit
                push eax ; for HeapFree

                mov esi, eax
                mov edi, eax
                xor eax, eax
                lea ecx, [eax-1] ; -1
                repnz scasb
                not ecx
                dec ecx ; strlen(esi)
                jz .cleanup

                ; Quotes?
                dec ecx
                lodsb
                mov edi, esi
                cmp al, '"'
                jz .quotes

                ; Unquoted
                dec esi
                mov al, ' '
                cmp byte [esi], al
                jz .no_params

                ; Quoted
.quotes:        repnz scasb
                jnz .no_params
                lea edx, [edi-1]
                mov al, ' '
                repz scasb
                jz .no_params
                dec edi
                jmp .execute

.no_params:     xor edi, edi

.execute:       mov cl, byte [edx]
                push ecx
                mov byte [edx], 0

                push SW_SHOWNORMAL
                push NULL
                push edi
                push esi
                push gui.szOpen
                push NULL
                call [ShellExecuteA]

                pop ecx
                mov byte [edx], cl

.cleanup:       ; push eax
                push 0
                call [GetProcessHeap]
                push eax
                call [HeapFree]
.exit:          pop edi
                pop esi
                ret
endp

proc UpdateSelection
                local iItem:DWORD

                push esi

                push LVNI_SELECTED
                push -1
                push LVM_GETNEXTITEM
                push [ebx + CONTEXT.winList.hwnd]
                call [SendMessageA]
                mov [iItem], eax
                inc eax
                push 0
                pop eax
                setnz al

                ; Enable/disable fingerprint managament controls
                lea esi, [ebx + CONTEXT.winPath]
                push 3 ; path, browse, enroll
                pop edx
                mov ecx, edx
@@:             push eax
                push [esi + WIN.hwnd]
                lea esi, [esi + sizeof.WIN]
                loop @b
                mov esi, [EnableWindow]
                call esi
                call esi
                call esi

                ; Clear enrollment status
                push gui.szEmpty
                push 0
                push WM_SETTEXT
                push [ebx + CONTEXT.winStatus.hwnd]
                call [SendMessageA]

                ; Set command text
.set_command:   mov eax, [iItem]
                test eax, eax
                mov esi, gui.szEmpty
                js .empty_text

                push eax
                call GetCommand
                test eax, eax
                cmovnz esi, eax

.empty_text:    push esi
                push [ebx + CONTEXT.winPath.hwnd]
                call [SetWindowTextA]
                cmp [iItem], 0
                jl @f
                push esi
                push 0
                call [GetProcessHeap]
                push eax
                call [HeapFree]
@@:
                xor esi, esi
                xor esi, [iItem]
                mov eax, gui.szEmpty
                js @f
                inc esi
                push esi
                call SubfactorFullText
@@:             push eax
                push [ebx + CONTEXT.winInfo.hwnd]
                call [SetWindowTextA]

.exit:          pop esi
                ret
endp

proc SetSelection dwFactor:DWORD
                local item:LV_ITEM

                push edi
                xor eax, eax
                mov ecx, sizeof.LV_ITEM
                lea edi, [item]
                push edi
                rep stosb
                pop edi

                mov al, LVIS_SELECTED+LVIS_FOCUSED
                mov edx, [dwFactor]
                dec edx
                mov [edi + LV_ITEM.mask], LVIF_STATE
                mov [edi + LV_ITEM.iItem], edx
                mov [edi + LV_ITEM.state], eax
                mov [edi + LV_ITEM.stateMask], eax

                mov eax, [ebx + CONTEXT.winList.hwnd]
                push eax

                push edi
                push ecx;0
                push LVM_SETITEM
                push eax
                call [SendMessageA]

                call [SetFocus]

                pop edi
                ret
endp

;proc SetStatus
;proc SetStatus szText:DWORD
SetStatus:
                virtual at esp
                  .szBuf db 128 dup (?)
                  .allocaSize = $ - $$
                  .retAddr dd ?
                  .arg0 dd ?
                  .arg1 dd ?
                end virtual

                sub esp, .allocaSize
                mov ecx, [.arg0]

                mov eax, gui.szStatusDefault
                dec ecx ; STATUS_DEFAULT (1)
                jz .set_text
                add eax, gui.szStatusEnrolled - gui.szStatusDefault
                dec ecx ; STATUS_ENROLLED (2)
                jz .set_text
                add eax, gui.szStatusEnrolling - gui.szStatusEnrolled ; unused
                dec ecx ; STATUS_ENROLLING (3)
                jz .enrolling
                add eax, gui.szStatusError - gui.szStatusEnrolling
                dec ecx ; STATUS_ERROR (4)
                jz .error
                add eax, gui.szStatusCommitting - gui.szStatusError
                dec ecx ; STATUS_COMMITTING (5)
                jz .set_text
                add eax, gui.szStatusDiscarding - gui.szStatusCommitting
                dec ecx ; STATUS_DISCARDING (6)
                jz .set_text
                add eax, gui.szStatusCommitted - gui.szStatusDiscarding
                dec ecx ; STATUS_COMMITTED (7)
                jz .set_text
                add eax, gui.szStatusDiscarded - gui.szStatusCommitted
                dec ecx ; STATUS_DISCARDED (8)
                jz .set_text
                add eax, gui.szStatusIdentifying - gui.szStatusDiscarded
                dec ecx ; STATUS_IDENTIFYING (10)
                jz .set_text
                mov eax, [.arg0]
                jmp .set_text

.enrolling:     mov eax, 'Enro'
                lea edi, [.szBuf]
                push edi
                stosd
                mov eax, 'llin'
                stosd
                mov ax, 'g '
                stosw
                movzx ecx, byte [gui.bEnrollCounter]
                inc ecx
                mov al, '.'
                mov ah, cl
                rep stosb
                mov [edi], cl
                cmp ah, 3
                jb @f
                xor ah, ah
@@:             mov byte [gui.bEnrollCounter], ah
                pop eax
                jmp .set_text

.error:         mov ecx, eax
                mov edx, [.arg1]
                push edx
                call WinBioErrorText ; clobbers only eax
                test eax, eax

                push ebp
                mov ebp, esp
                push edx
                jz .unknown_error

.winbio_error:  add ecx, gui.szStatusErrorText - gui.szStatusError
                push eax

.unknown_error: push ecx
                lea eax, [ebp+4] ; lea eax, [.szBuf]
                push eax
                call [wsprintfA]
                leave
                mov eax, esp

.set_text8:     push 8 ; 2 args
                jmp @f
.set_text:
.set_text4:     push 4 ; 1 arg
@@:             push eax
                push 0
                push WM_SETTEXT
                push [ebx + CONTEXT.winStatus.hwnd]
                call [SendMessageA]
                pop eax
                add esp, .allocaSize
                pop ecx
                add esp, eax
                jmp ecx

proc SaveSettings
                local rect:RECT
                local count:DWORD

                ; [GUI]
                lea eax, [rect]
                push eax
                push eax
                push [ebx + CONTEXT.win.hwnd]
                call [GetWindowRect]
                test eax, eax
                pop eax
                jz .exit
                push esi
                push edi
                mov edi, [ebx + CONTEXT.pSettings]

                push [eax + RECT.bottom]
                push [eax + RECT.right]
                push [eax + RECT.top]
                push [eax + RECT.left]
                pop [edi + SETTINGS.GUI.x]
                pop [edi + SETTINGS.GUI.y]
                pop [edi + SETTINGS.GUI.cx]
                pop [edi + SETTINGS.GUI.cy]

                ; [Devices]
                ; ...

                ; [Commands]
                mov [count], 0
                lea edi, [edi + SETTINGS.cmd]
.next_command:  push [count]
                call GetCommand
                test eax, eax
                mov ecx, MAX_PATH
                jnz .got_command
                add edi, ecx
                jmp .next
.got_command:   mov esi, eax

                push ecx
                push esi
                call [lstrlenA]
                pop ecx

                cmp eax, ecx
                jb @f
                lea eax, [ecx-1]
@@:             sub ecx, eax
                xchg eax, ecx

                push esi

                rep movsb
                xchg eax, ecx
                rep stosb

                push eax
                call [GetProcessHeap]
                push eax
                call [HeapFree]

.next:          inc [count]
                cmp [count], 10
                jb .next_command

.commands_done: pop edi
                pop esi
.exit:          ret
endp
