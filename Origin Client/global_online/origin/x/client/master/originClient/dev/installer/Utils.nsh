;--------------------------------
; EA Access installer utilities
;--------------------------------

;------------------------------------------------------------------------------
; Write a registry string for both x64 and WOW64
;------------------------------------------------------------------------------
!macro WriteRegStrX64Call _ROOT _SUBKEY _KEY _VALUE
    ${If} ${RunningX64}
        SetRegView 64
        WriteRegStr ${_ROOT} "${_SUBKEY}" "${_KEY}" "${_VALUE}"
        SetRegView 32
    ${EndIf}
    WriteRegStr ${_ROOT} "${_SUBKEY}" "${_KEY}" "${_VALUE}"

!macroend

!define WriteRegStrX64      '!insertmacro WriteRegStrX64Call'
!define un.WriteRegStrX64   '!insertmacro WriteRegStrX64Call'      

;------------------------------------------------------------------------------
; Delete a registry value for both x64 and WOW64
;------------------------------------------------------------------------------
!macro DeleteRegValueX64Call _ROOT _SUBKEY _KEY
    ${If} ${RunningX64}
        SetRegView 64
        DeleteRegValue ${_ROOT} "${_SUBKEY}" "${_KEY}"
        SetRegView 32
    ${EndIf}
    DeleteRegValue ${_ROOT} "${_SUBKEY}" "${_KEY}"

!macroend

!define DeleteRegValueX64      '!insertmacro DeleteRegValueX64Call'
!define un.DeleteRegValueX64   '!insertmacro DeleteRegValueX64Call'      

;------------------------------------------------------------------------------
; Delete a registry key for both x64 and WOW64
;------------------------------------------------------------------------------
!macro DeleteRegKeyX64Call _ROOT _SUBKEY
    ${If} ${RunningX64}
        SetRegView 64
        DeleteRegKey ${_ROOT} "${_SUBKEY}"
        SetRegView 32
    ${EndIf}
    DeleteRegKey ${_ROOT} "${_SUBKEY}"

!macroend

!define DeleteRegKeyX64      '!insertmacro DeleteRegKeyX64Call'
!define un.DeleteRegKeyX64   '!insertmacro DeleteRegKeyX64Call'     

;------------------------------------------------------------------------------
; Delete a registry key for both x64 and WOW64 only if its empty
;------------------------------------------------------------------------------
!macro DeleteRegKeyIfEmptyX64Call _ROOT _SUBKEY
    ${If} ${RunningX64}
        SetRegView 64
        DeleteRegKey /ifempty ${_ROOT} "${_SUBKEY}"
        SetRegView 32
    ${EndIf}
    DeleteRegKey /ifempty ${_ROOT} "${_SUBKEY}"

!macroend

!define DeleteRegKeyIfEmptyX64      '!insertmacro DeleteRegKeyIfEmptyX64Call'
!define un.DeleteRegKeyIfEmptyX64   '!insertmacro DeleteRegKeyIfEmptyX64Call'     

;------------------------------------------------------------------------------
; Get parent directory (FileFunc.nsh method is giving me problems)
;------------------------------------------------------------------------------
; The provided GetParent method was giving me troubles so we use our own
!ifdef GetParent
    !undef GetParent
!endif    

!macro FUNC_GetParent un
Function ${un}GetParent
  Exch $0 ; old $0 is on top of stack
  Push $1
  Push $2
  StrCpy $1 -1
  loop:
    StrCpy $2 $0 1 $1
    StrCmp $2 "" exit
    StrCmp $2 "\" exit
    IntOp $1 $1 - 1
  Goto loop
  exit:
    StrCpy $0 $0 $1
    Pop $2
    Pop $1
    Exch $0 ; put $0 on top of stack, restore $0 to original value
FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_GetParent ""
!insertmacro FUNC_GetParent "un."

;------------------------------------------------------------------------------
; VersionCompare
; Compare version numbers (http://nsis.sourceforge.net/VersionCompare).
; Syntax:
; ${VersionCompare} "[Version1]" "[Version2]" $var
; 
; Push VER1
; Push VER2
; Call VersionCompare
; Pop $var              ; Result:
;                       ;    $var=0  Versions are equal
;                       ;    $var=1  Version1 is newer
;                       ;    $var=2  Version2 is newer
;------------------------------------------------------------------------------
Function VersionCompare

    Exch $1
    Exch
    Exch $0
    Exch
    Push $2
    Push $3
    Push $4
    Push $5
    Push $6
    Push $7
 
begin:
    StrCpy $2 -1
    IntOp $2 $2 + 1
    StrCpy $3 $0 1 $2
    StrCmp $3 '' +2
    StrCmp $3 '.' 0 -3
    StrCpy $4 $0 $2
    IntOp $2 $2 + 1
    StrCpy $0 $0 '' $2
 
    StrCpy $2 -1
    IntOp $2 $2 + 1
    StrCpy $3 $1 1 $2
    StrCmp $3 '' +2
    StrCmp $3 '.' 0 -3
    StrCpy $5 $1 $2
    IntOp $2 $2 + 1
    StrCpy $1 $1 '' $2
 
    StrCmp $4$5 '' equal
 
    StrCpy $6 -1
    IntOp $6 $6 + 1
    StrCpy $3 $4 1 $6
    StrCmp $3 '0' -2
    StrCmp $3 '' 0 +2
    StrCpy $4 0
 
    StrCpy $7 -1
    IntOp $7 $7 + 1
    StrCpy $3 $5 1 $7
    StrCmp $3 '0' -2
    StrCmp $3 '' 0 +2
    StrCpy $5 0
 
    StrCmp $4 0 0 +2
    StrCmp $5 0 begin newer2
    StrCmp $5 0 newer1
    IntCmp $6 $7 0 newer1 newer2
 
    StrCpy $4 '1$4'
    StrCpy $5 '1$5'
    IntCmp $4 $5 begin newer2 newer1
 
equal:
    StrCpy $0 0
    goto end
    
newer1:
    StrCpy $0 1
    goto end
    
newer2:
    StrCpy $0 2
 
end:
    Pop $7
    Pop $6
    Pop $5
    Pop $4
    Pop $3
    Pop $2
    Pop $1
    Exch $0

FunctionEnd


;------------------------------------------------------------------------------
; GetVersionInfoString
; Get the file version information as a string
;
; Usage:
;   Push $INSTDIR\file.dll
;   Call GetVersionInfoString
;   Pop $R0   ; at this point $R0 is "1.0.0.0"
;------------------------------------------------------------------------------
!macro FUNC_GetVersionInfoString un
Function ${un}GetVersionInfoString

    Exch $0
    Push $R1
    Push $R2
    Push $R3
    Push $R4
    Push $R5

    ClearErrors
    GetDllVersion "$0" $R0 $R1
    IfErrors error
    
    IntOp $R2 $R0 / 0x00010000
    IntOp $R3 $R0 & 0x0000FFFF
    IntOp $R4 $R1 / 0x00010000
    IntOp $R5 $R1 & 0x0000FFFF
    StrCpy $0 "$R2.$R3.$R4.$R5"
    Goto done

error:
    StrCpy $0 ""

done:
    Pop $R5
    Pop $R4
    Pop $R3
    Pop $R2
    Pop $R1
    Exch $0
    
FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_GetVersionInfoString ""
!insertmacro FUNC_GetVersionInfoString "un."

!define LOGPIXELSX  '88'   ;88    /* Logical pixels/inch in X                 */
!define LOGPIXELSY  '90'   ;90    /* Logical pixels/inch in Y                 */

;------------------------------------------------------------------------------
; GetDPIs
; Returns current DPIs
;
; Usage:
;   Call GetDPIs
;   Pop $R0   ; at this point $R0 contains the DPIs of the system
;------------------------------------------------------------------------------
!macro FUNC_GetDPIs un
Function ${un}GetDPIs

    Push $R0
    Push $R1
    Push $R2
 
    ClearErrors

	System::Call 'user32::GetWindowDC(i $HWNDPARENT) i .r0'
	System::Call 'gdi32::GetDeviceCaps(i $0, i 88) i .r1 '
	StrCpy $R0 $1
		
	Pop $R2
	Pop $R1
	Exch $R0
FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_GetDPIs ""
!insertmacro FUNC_GetDPIs "un."


;------------------------------------------------------------------------------
; IsWindowsVista
; Checks if the current OS is Vista
;
; Usage:
;   Call IsWindowsVista
;   Pop $R0   ; at this point $R0 is "1" or "0"
;------------------------------------------------------------------------------
!macro FUNC_IsWindowsVista un
Function ${un}IsWindowsVista

    Push $R0
    Push $R1
    Push $R2
 
    ClearErrors
		; get OS
			;  DWORD dwOSVersionInfoSize;
			;  DWORD dwMajorVersion;
			;  DWORD dwMinorVersion;
			;  DWORD dwBuildNumber;
			;  DWORD dwPlatformId;
			;  TCHAR szCSDVersion[128];
			;  WORD  wServicePackMajor;
			;  WORD  wServicePackMinor;
			;  WORD  wSuiteMask;
			;  BYTE  wProductType;
			;  BYTE  wReservesd;
		
		System::Alloc 156
		Pop $0
		System::Call '*$0(i156)'
		System::Call kernel32::GetVersionExA(ir0)
		System::Call '*$0(i,&i4.r3,&i4.r4,i,i,&t128,&i2.r1,&i2.r2)'
		System::Free $0

		; Windows Server 2003 R2 / Windows Home Server / Windows Server 2003 / Windows XP Professional x64 Edition / Windows XP
		; major version 6
		
		IntCmp $3 6 MightBeVista Nah Nah
			
MightBeVista:
		; minor version 1
		IntCmp $4 0 ItIsVista Nah Nah
 
ItIsVista:
		StrCpy $R0 "1"
		Goto Done
Nah:
		StrCpy $R0 "0"
		Goto Done

Done:	
		Pop $R2
		Pop $R1
		Exch $R0
FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_IsWindowsVista ""
!insertmacro FUNC_IsWindowsVista "un."


;------------------------------------------------------------------------------
; IsUserAdmin
; Checks if the current user has admin privileges
;
; Usage:
;   Call IsUserAdmin
;   Pop $R0   ; at this point $R0 is "1" or "0"
;------------------------------------------------------------------------------
!macro FUNC_IsUserAdmin un
Function ${un}IsUserAdmin

    Push $R0
    Push $R1
    Push $R2
 
    ClearErrors
    UserInfo::GetName
    IfErrors Win9x
    Pop $R1
    UserInfo::GetAccountType
    Pop $R2
     
    StrCmp $R2 "Admin" 0 Continue
    ; Observation: I get here when running Win98SE. (Lilla)
    ; The functions UserInfo.dll looks for are there on Win98 too, 
    ; but just don't work. So UserInfo.dll, knowing that admin isn't required
    ; on Win98, returns admin anyway. (per kichik)
    StrCpy $R0 "1"
    Goto Done
 
Continue:
    ; You should still check for an empty string because the functions
    ; UserInfo.dll looks for may not be present on Windows 95. (per kichik)
    StrCmp $R2 "" Win9x
    StrCpy $R0 "0"
    Goto Done
 
Win9x:
    ; comment/message below is by UserInfo.nsi author:
    ; This one means you don't need to care about admin or
    ; not admin because Windows 9x doesn't either
    StrCpy $R0 "1"
 
Done:
    Pop $R2
    Pop $R1
    Exch $R0
FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_IsUserAdmin ""
!insertmacro FUNC_IsUserAdmin "un."

;------------------------------------------------------------------------------
; GetLocaleName
; Get the locale name for the current $LANGUAGE
;
; Usage:
;   Call GetLocaleName
;   Pop $R0   ; at this point $R0 is "en_US"
;------------------------------------------------------------------------------
!define LOCALE_SISO639LANGNAME   '0x00000059'   ; ISO abbreviated language name
!define LOCALE_SISO3166CTRYNAME  '0x0000005A'   ; ISO abbreviated country name

!macro FUNC_GetLocaleName un
Function ${un}GetLocaleName

    Push $0
     
    Push $LANGUAGE
    Call ${un}GetLanguageCodeFromLCID
    Pop $0
    
    Exch $0
    
FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_GetLocaleName ""
!insertmacro FUNC_GetLocaleName "un."

;------------------------------------------------------------------------------
; GetSystemDefaultLanguageCode
; Get the system default language code in the form en_US (ISO 639 lang name + ISO 3166 2 letter country name)
; Modifies no variables/registers
;
; Usage:
;   Call GetSystemDefaultLanguageCode
;   Pop $R0  ; $R0 now contains en_US
;------------------------------------------------------------------------------
!macro FUNC_GetSystemDefaultLanguageCode un
Function ${un}GetSystemDefaultLanguageCode

    Push $0

    StrCpy $0 ${LANG_ENGLISH} ; Set default language
    System::Call "Kernel32::GetSystemDefaultLCID() i .r0"
    
    Push $0
    Call ${un}GetLanguageCodeFromLCID
    Pop $0

done:
    Exch $0

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_GetSystemDefaultLanguageCode ""
!insertmacro FUNC_GetSystemDefaultLanguageCode "un."

;------------------------------------------------------------------------------
; GetLanguageCodeFromLCID
; Get a language code from an LCID
;
; Usage:
;   Push 1034
;   Call GetLanguageCodeFromLCID
;   Pop $R0  ; $R0 now contains en_US
;------------------------------------------------------------------------------
!macro FUNC_GetLanguageCodeFromLCID un
Function ${un}GetLanguageCodeFromLCID

    Exch $0
    Push $1
    Push $2

    StrCpy $1 ""
    StrCpy $2 ""

    System::Call "Kernel32::GetLocaleInfo(i r0, i ${LOCALE_SISO639LANGNAME}, t .r1, i ${NSIS_MAX_STRLEN}) i."
    System::Call "Kernel32::GetLocaleInfo(i r0, i ${LOCALE_SISO3166CTRYNAME}, t .r2, i ${NSIS_MAX_STRLEN}) i."

    StrCpy $0 "$1_$2"

done:
    Pop $2
    Pop $1
    Exch $0

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_GetLanguageCodeFromLCID ""
!insertmacro FUNC_GetLanguageCodeFromLCID "un."

;------------------------------------------------------------------------------
; CopyToClipboard
; Copy text to the clipboard
; Usage:
;   Push "some text"
;   Call CopyToClipboard
;------------------------------------------------------------------------------
!macro FUNC_CopyToClipboard un
Function ${un}CopyToClipboard

    Exch $0 ;input string
    Push $1
    Push $2
    System::Call 'user32::OpenClipboard(i 0)'
    System::Call 'user32::EmptyClipboard()'
    StrLen $1 $0
    IntOp $1 $1 + 1
    System::Call 'kernel32::GlobalAlloc(i 2, i r1) i.r1'
    System::Call 'kernel32::GlobalLock(i r1) i.r2'
    System::Call 'kernel32::lstrcpyA(i r2, t r0)'
    System::Call 'kernel32::GlobalUnlock(i r1)'
    System::Call 'user32::SetClipboardData(i 1, i r1)'
    System::Call 'user32::CloseClipboard()'
    Pop $2
    Pop $1
    Pop $0

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_CopyToClipboard ""
!insertmacro FUNC_CopyToClipboard "un."

;------------------------------------------------------------------------------
; Trim
; Removes leading & trailing whitespace from a string
; Usage:
;   Push 
;   Call Trim
;   Pop 
;------------------------------------------------------------------------------
!macro FUNC_Trim un
Function ${un}Trim

    Exch $R1 ; Original string
    Push $R2
 
Loop:
    StrCpy $R2 "$R1" 1
    StrCmp "$R2" " " TrimLeft
    StrCmp "$R2" "$\r" TrimLeft
    StrCmp "$R2" "$\n" TrimLeft
    StrCmp "$R2" "$\t" TrimLeft
    GoTo Loop2
    
TrimLeft:   
    StrCpy $R1 "$R1" "" 1
    Goto Loop
 
Loop2:
    StrCpy $R2 "$R1" 1 -1
    StrCmp "$R2" " " TrimRight
    StrCmp "$R2" "$\r" TrimRight
    StrCmp "$R2" "$\n" TrimRight
    StrCmp "$R2" "$\t" TrimRight
    GoTo done
    
TrimRight:  
    StrCpy $R1 "$R1" -1
    Goto Loop2
 
done:
    Pop $R2
    Exch $R1

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_Trim ""
!insertmacro FUNC_Trim "un."

;------------------------------------------------------------------------------
; StrStr
; input, top of stack = string to search for
;        top of stack-1 = string to search in
; output, top of stack (replaces with the portion of the string remaining)
; modifies no other variables.
;
; Usage:
;   Push "this is a long string"
;   Push "long"
;   Call StrStr
;   Pop $0
;  ($0 at this point is "long string")
;------------------------------------------------------------------------------
!macro FUNC_StrStr un
Function ${un}StrStr

    Exch $1 ; st=haystack,old$1, $1=needle
    Exch    ; st=old$1,haystack
    Exch $2 ; st=old$1,old$2, $2=haystack
    Push $3
    Push $4
    Push $5
    StrLen $3 $1
    StrCpy $4 0
    ; $1=needle
    ; $2=haystack
    ; $3=len(needle)
    ; $4=cnt
    ; $5=tmp
loop:
    StrCpy $5 $2 $3 $4
    StrCmp $5 $1 done
    StrCmp $5 "" done
    IntOp $4 $4 + 1
    Goto loop
done:
    StrCpy $1 $2 "" $4
    Pop $5
    Pop $4
    Pop $3
    Pop $2
    Exch $1
    
FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_StrStr ""
!insertmacro FUNC_StrStr "un."

;------------------------------------------------------------------------------
; EndsWith
; input, top of stack = string to search for
;        top of stack-1 = string to search in
; output, top of stack (1 or 0)
; modifies no other variables.
;
; Usage:
;   Push "this is a long string"
;   Push "string"
;   Call EndsWith
;   Pop $0
;  ($0 at this point is "1")
;------------------------------------------------------------------------------
!macro FUNC_EndsWith un
Function ${un}EndsWith

    Exch $1 ; st=haystack,old$1, $1=needle
    Exch    ; st=old$1,haystack
    Exch $2 ; st=old$1,old$2, $2=haystack
    Push $3
    
    Push $2
    Push $1
    Call ${un}StrStr
    Pop $3
    
    StrCmp $3 $1 yes no
    
yes:
    StrCpy $1 "1"
    Goto done

no:
    StrCpy $1 "0"
    Goto done
    
done:
    Pop $3
    Pop $2
    Exch $1

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_EndsWith ""
!insertmacro FUNC_EndsWith "un."

;------------------------------------------------------------------------------
; StrLower
; Converts the string on the stack to lowercase (in an ASCII sense)
; Usage:
; Push
; Call StrLower
; Pop
;------------------------------------------------------------------------------
!macro FUNC_StrLower un
Function ${un}StrLower
    Exch $0 ; Original string
    Push $1 ; Final string
    Push $2 ; Current character
    Push $3
    Push $4
    StrCpy $1 ""

Loop:
    StrCpy $2 $0 1 ; Get next character
    StrCmp $2 "" Done
    StrCpy $0 $0 "" 1
    StrCpy $3 122 ; 122 = ASCII code for z

Loop2:
    IntFmt $4 %c $3 ; Get character from current ASCII code
    StrCmp $2 $4 Match
    IntOp $3 $3 - 1
    StrCmp $3 91 NoMatch Loop2 ; 90 = ASCII code one beyond Z

Match:
    StrCpy $2 $4 ; It 'matches' (either case) so grab the lowercase version

NoMatch:
    StrCpy $1 $1$2 ; Append to the final string
    Goto Loop

Done:
    StrCpy $0 $1 ; Return the final string
    Pop $4
    Pop $3
    Pop $2
    Pop $1
    Exch $0
FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_StrLower ""
!insertmacro FUNC_StrLower "un."

;------------------------------------------------------------------------------
; DirState
; Check directory full, empty or not exist (http://nsis.sourceforge.net/DirState).
;
; Syntax:
;   Push "[path]"       ;Directory
;   Call DirState
;   Pop $res
; 
;   $res      ; Result:
;             ;    $res=0  (empty)
;             ;    $res=1  (full)
;             ;    $res=-1 (directory not found)
;------------------------------------------------------------------------------
!macro FUNC_DirState un
Function ${un}DirState

    Exch $0
    Push $1
    ClearErrors
 
    FindFirst $1 $0 '$0\*.*'
    IfErrors 0 +3
    StrCpy $0 -1
    goto end
    StrCmp $0 '.' 0 +4
    FindNext $1 $0
    StrCmp $0 '..' 0 +2
    FindNext $1 $0
    FindClose $1
    IfErrors 0 +3
    StrCpy $0 0
    goto end
    StrCpy $0 1
 
end:
    Pop $1
    Exch $0

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_DirState ""
!insertmacro FUNC_DirState "un."

;------------------------------------------------------------------------------
; GetProcessFileName
; Get the full executable path for a process given its pid
; Syntax:
; Push PID
; Call GetProcessFileName
; Pop $0
;
; $0 now contains the path to the executable that started the process
;------------------------------------------------------------------------------
!macro FUNC_GetProcessFileName un
Function ${un}GetProcessFileName

    # ( Pid -- ProcessName )
    Exch $2                     ; get Pid, save $2
    Push $0
    Push $1
    Push $3
    Push $R0
    
    StrCpy $R0 "0"
    StrCpy $3 "0"
        
    System::Call "Kernel32::OpenProcess(i 1040, i 0, i r2)i .r3"

    StrCpy $2 ""       ; set return value

    ; Check return value 
    IntCmp $3 0 done
  
    ; get hMod array
    System::Alloc 1024
    Pop $R0
 
    ; params: Pid, &hMod, sizeof(hMod), &cb
    System::Call "Psapi::EnumProcessModules(i r3, i R0, i 1024, *i .r1)i .r0"
 
    IntCmp $0 0 done
    
    ; get first hMod
    System::Call "*$R0(i .r0)"
 
    ; params: hProc, hMod, buffer, len
    System::Call "Psapi::GetModuleFileNameEx(i r3, i r0, t .r2, i 256)i .r0"
    
done:
    ; Free buffer
    System::Free $R0
    System::Call "kernel32::CloseHandle(i r3)"
 
    Pop $R0                     ; restore registers
    Pop $3
    Pop $1
    Pop $0
    Exch $2                     ; save process name

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_GetProcessFileName ""
!insertmacro FUNC_GetProcessFileName "un."

;------------------------------------------------------------------------------
; FindProcessWithPath
; Get the first pid for a process given the executable path
; Syntax:
; Push "c:/progrma.exe"
; Call FindProcessWithPath
; Pop $0
;
; $0 now contains a PID or 0 
;------------------------------------------------------------------------------
!macro FUNC_FindProcessWithPath un
Function ${un}FindProcessWithPath

    Exch $R5    ; File path
    Push $0     
    Push $1
    Push $2
    Push $R0
    Push $R1
    Push $R2
    Push $R3
    Push $R4
    Push $R6
 
    System::Alloc 4096
    Pop $R0                         ; process list buffer
    System::Alloc 2048
    Pop $R4                         ; full name buffer

    StrCpy $R1 "0"
    
     # get an array of all process ids
    System::Call "Psapi::EnumProcesses(i R0, i 4096, *i .R1)i .r0"

    IntCmp $0 0 notfound
 
    IntOp $R1 $R1 >> 2           ; Divide by sizeof(DWORD) to get $R1 process count
    IntCmp $R1 0 notfound
     
iterate:
    ; R3 will be the index 
    IntCmp $R3 $R1 notfound 0 notfound
    
    StrCpy $R2 $R3
    IntOp $0 $R2 << 2           ; Move to a certain DWORD position
    IntOp $0 $0 + $R0           ; Set 0 to point to R0 plus offset
    System::Call "*$0(i .r0)"   ; Get next PID
    
    Push $0
    Call ${un}GetProcessFileName
    Pop $2
    StrCpy $R6 $2			; temp. copy, so we can use it in the ::Call(...)
    StrCpy $R4 "0"		; clear our full name string

    ; check if we find it in the short name
    StrCmp $R6 $R5 found
   
    # convert the short name to the full name 
    System::Call "Kernel32::GetLongPathName(t R6, t.R4, i 1024)"

    ; check if we find it in the full name
    StrCmp $R4 $R5 found
    
    IntOp $R3 $R3 + 1
    goto iterate

notfound:
    StrCpy $R5 "0"
    Goto done
    
found:
    StrCpy $R5 $0

done:
    System::Free $R0          
		System::Free $R4             
 
    Pop $R6
    Pop $R4
    Pop $R3
    Pop $R2
    Pop $R1
    Pop $R0
    Pop $2
    Pop $1
    Pop $0
    Exch $R5
 
FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_FindProcessWithPath ""
!insertmacro FUNC_FindProcessWithPath "un."


;------------------------------------------------------------------------------
; FindProcessWithoutPath
; Get the first pid for a process given the executable path without a path!
; Syntax:
; Push "progrma.exe"
; Call FindProcessWithoutPath
; Pop $0
;
; $0 now contains a PID or 0 
;------------------------------------------------------------------------------
!macro FUNC_FindProcessWithoutPath un
Function ${un}FindProcessWithoutPath

    Exch $R5    ; File path
    Push $0     
    Push $1
    Push $2
    Push $R0
    Push $R1
    Push $R2
    Push $R3
    Push $R4
    Push $R6
 
    System::Alloc 4096
    Pop $R0                         ; process list buffer
    System::Alloc 2048
    Pop $R4                         ; full name buffer

    StrCpy $R1 "0"
    
     # get an array of all process ids
    System::Call "Psapi::EnumProcesses(i R0, i 4096, *i .R1)i .r0"

    IntCmp $0 0 notfound
 
    IntOp $R1 $R1 >> 2           ; Divide by sizeof(DWORD) to get $R1 process count
    IntCmp $R1 0 notfound
     
iterate:
    ; R3 will be the index 
    IntCmp $R3 $R1 notfound 0 notfound
    
    StrCpy $R2 $R3
    IntOp $0 $R2 << 2           ; Move to a certain DWORD position
    IntOp $0 $0 + $R0           ; Set 0 to point to R0 plus offset
    System::Call "*$0(i .r0)"   ; Get next PID
    
    Push $0
    Call ${un}GetProcessFileName
    Pop $2
    StrCpy $R6 $2			; temp. copy, so we can use it in the ::Call(...)
    StrCpy $R4 "0"		; clear our full name string

    ; check if we find it in the short name
    ;StrCmp $R6 $R5 found
    
		Push $R6
		Push $R5
		Call ${un}StrStr
		Pop $1
		StrCmp $1 "" 0 found
    
    # convert the short name to the full name 
    System::Call "Kernel32::GetLongPathName(t R6, t.R4, i 1024)"

    ; check if we find it in the full name
    ;StrCmp $R4 $R5 found

		Push $R4
		Push $R5
		Call ${un}StrStr
		Pop $1
		StrCmp $1 "" 0 found
   
    IntOp $R3 $R3 + 1
    goto iterate

notfound:
    StrCpy $R5 "0"
    Goto done
    
found:
    StrCpy $R5 $0

done:
    System::Free $R0          
		System::Free $R4             
 
    Pop $R6
    Pop $R4
    Pop $R3
    Pop $R2
    Pop $R1
    Pop $R0
    Pop $2
    Pop $1
    Pop $0
    Exch $R5
 
FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_FindProcessWithoutPath ""
!insertmacro FUNC_FindProcessWithoutPath "un."

;------------------------------------------------------------------------------
; KillProcess
; Kill a process given its PID
; Syntax:
; Push PID
; Call KillProcess
; Pop $0
;
; $0 now contains 1 or 0 
;------------------------------------------------------------------------------
!macro FUNC_KillProcess un
Function ${un}KillProcess

    Exch $R1    ; PID
    Push $0     
    Push $1
    Push $2

    StrCpy $2 "0" ; Default to failure
    System::Call "Kernel32::OpenProcess(i 1, i 0, i R1)i .r3"
    
    ; Check return value 
    IntCmp $3 0 done
   
    ;Kill it
    System::Call "Kernel32::TerminateProcess(i r3, i -1)i .r2"
    System::Call "kernel32::CloseHandle(i r3)"
    
done:
    StrCpy $R1 $2
    
    Pop $2
    Pop $1
    Pop $0   
    Exch $R1
    
FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_KillProcess ""
!insertmacro FUNC_KillProcess "un."

;------------------------------------------------------------------------------
; KillProcessWithPath
; Kill a process given its path
; Syntax:
; Push "c:\program files\electronic arts\core.exe"
; Call KillProcessWithPath
; Pop $0
;
; $0 now contains 1 or 0 
;------------------------------------------------------------------------------
!macro FUNC_KillProcessWithPath un
Function ${un}KillProcessWithPath

    Exch $0

    ; Find the process PID
    Push "$0"
    Call ${un}FindProcessWithPath
    Pop $0

    ; Found?
    IntCmp $0 0 done 
    Push $0

    ; Kill it!
    Call ${un}KillProcess
    Pop $0

done:
    Exch $0

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_KillProcessWithPath ""
!insertmacro FUNC_KillProcessWithPath "un."



;------------------------------------------------------------------------------
; KillAllProcessWithoutPath
; Kills all processes with a given name
; Syntax:
; Push "core.exe"
; Call KillProcessWithoutPath
;
;------------------------------------------------------------------------------
!macro FUNC_KillAllProcessWithoutPath un
Function ${un}KillAllProcessWithoutPath

    Pop $0
    Push $1

nextProcess:

    ; Find the process PID
    Push "$0"
    Call ${un}FindProcessWithoutPath
    Pop $1

    ; Found?
    IntCmp $1 0 done 
    Push $1

    ; Kill it!
    Call ${un}KillProcess
    Pop $1
    IntCmp $1 1 nextProcess

done:
	Pop $1
FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_KillAllProcessWithoutPath ""
!insertmacro FUNC_KillAllProcessWithoutPath "un."


;------------------------------------------------------------------------------
; HideAppFromRecentMenu
; Hide application from the recently run menu. 
; In Windows 7 it will also prevent the app from being pinned.
; Syntax:
; Push "Core.exe"
; Call HideAppFromRecentMenu
;------------------------------------------------------------------------------
!macro FUNC_HideAppFromRecentMenu un
Function ${un}HideAppFromRecentMenu

    Exch $0
    
    StrCmp $0 "" done

    WriteRegStr HKCR "Applications\$0" "NoStartPage" ""

done:
    Pop $0

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_HideAppFromRecentMenu ""
!insertmacro FUNC_HideAppFromRecentMenu "un."

;------------------------------------------------------------------------------
; RemoveRegisteredApp
; Remove registered application from the registry
; Syntax:
; Push "Core.exe"
; Call RemoveRegisteredApp
;------------------------------------------------------------------------------
!macro FUNC_RemoveRegisteredApp un
Function ${un}RemoveRegisteredApp

    Exch $0

    StrCmp $0 "" done
    
    DeleteRegKey HKCR "Applications\$0"

done:
    Pop $0

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_RemoveRegisteredApp ""
!insertmacro FUNC_RemoveRegisteredApp "un."

!define CSIDL_DESKTOP "0x0000"
!define CSIDL_INTERNET "0x0001"
!define CSIDL_PROGRAMS "0x0002"
!define CSIDL_CONTROLS "0x0003"
!define CSIDL_PRINTERS "0x0004"
!define CSIDL_PERSONAL "0x0005"
!define CSIDL_FAVORITES "0x0006"
!define CSIDL_STARTUP "0x0007"
!define CSIDL_RECENT "0x0008"
!define CSIDL_SENDTO "0x0009"
!define CSIDL_BITBUCKET "0x000A"
!define CSIDL_STARTMENU "0x000B"
!define CSIDL_MYDOCUMENTS "0x000C"
!define CSIDL_MYMUSIC "0x000D"
!define CSIDL_MYVIDEO "0x000E"
!define CSIDL_DIRECTORY "0x0010"
!define CSIDL_DRIVES "0x0011"
!define CSIDL_NETWORK "0x0012"
!define CSIDL_NETHOOD "0x0013"
!define CSIDL_FONTS "0x0014"
!define CSIDL_TEMPLATES "0x0015"
!define CSIDL_COMMON_STARTMENU "0x016"
!define CSIDL_COMMON_PROGRAMS "0x0017"
!define CSIDL_COMMON_STARTUP "0x0018"
!define CSIDL_COMMON_DESKTOPDIRECTORY "0x0019"
!define CSIDL_APPDATA "0x001A"
!define CSIDL_PRINTHOOD "0x001B"
!define CSIDL_LOCAL_APPDATA "0x001C"
!define CSIDL_ALTSTARTUP "0x001D"
!define CSIDL_COMMON_ALTSTARTUP "0x001E"
!define CSIDL_COMMON_FAVORITES "0x001F"
!define CSIDL_INTERNET_CACHE "0x0020"
!define CSIDL_COOKIES "0x0021"
!define CSIDL_HISTORY "0x0022"
!define CSIDL_COMMON_APPDATA "0x0023"
!define CSIDL_WINDOWS "0x0024"
!define CSIDL_SYSTEM "0x0025"
!define CSIDL_PROGRAM_FILES "0x0026"
!define CSIDL_MYPICTURES "0x0027"
!define CSIDL_PROFILE "0x0028"
!define CSIDL_SYSTEMX86 "0x0029"
!define CSIDL_PROGRAM_FILESX86 "0x002A"
!define CSIDL_PROGRAM_FILES_COMMON "0x002B"
!define CSIDL_PROGRAM_FILES_COMMONX86 "0x002C"
!define CSIDL_COMMON_TEMPLATES "0x002D"
!define CSIDL_COMMON_DOCUMENTS "0x002E"
!define CSIDL_COMMON_ADMINTOOLS "0x002F"
!define CSIDL_ADMINTOOLS "0x0030"
!define CSIDL_CONNECTIONS "0x0031"
!define CSIDL_COMMON_MUSIC "0x0035"
!define CSIDL_COMMON_PICTURES "0x0036"
!define CSIDL_COMMON_VIDEO "0x0037"
!define CSIDL_RESOURCES "0x0038"
!define CSIDL_RESOURCES_LOCALIZED "0x0039"
!define CSIDL_COMMON_OEM_LINKS "0x003A"
!define CSIDL_CDBURN_AREA "0x003B"
!define CSIDL_COMPUTERSNEARME "0x003D"

;------------------------------------------------------------------------------
; GetAirAppVersion
; Gets the version number for an installed air app
;
; Usage:
;   Push "C:\Program Files\MyAirAppName"
;   Call GetAirAppVersion
;   Pop $res
;------------------------------------------------------------------------------
!macro FUNC_GetAirAppVersion un
Function ${un}GetAirAppVersion

    Push $R1
    Exch
        
    ; Get the installation directory
    Exch $R0
    
    StrCpy $R1 ""
    
    ; Save registers
    Push $0
    Push $1
    Push $2
    Push $3
    Push $4    
    
    StrCpy $R0 "$R0\META-INF\AIR\application.xml"
    
    IfFileExists $R0 0 done
    
    ; Read file contents into $1
    ClearErrors
    FileOpen $0 $R0 r
    IfErrors done

ReadLoop:    
    FileRead $0 $1
    IfErrors DoneLoop
          
    Push $1
    Push "<version>"
    call ${un}StrStr
    Pop $1

    ; Read next line if not found
    StrCmp $1 "" ReadLoop 0
    
    ; Skip the <version> tag
    StrCpy $1 $1 1024 9
    
    ; $1 should contain something like "1.0.0.0</version>\n"
    
    StrLen $4 $1    ; $1 len    
    StrCpy $2 0     ; Current index

CopyLoop:
    ; Check if we are done    
    IntCmp $2 $4 DoneLoop 0 DoneLoop 
    
    StrCpy $3 $1 1 $2           ; Copy one char into $3
    IntOp $2 $2 + 1             ; Increase index
    StrCmp $3 "<"  0 CopyLoop   ; Loop while < is not found
    
    IntOp $2 $2 - 1             ; Skip last char
    StrCpy $1 $1 $2             ; Copy the version into $1
    
    ; Trim the resulting string
    Push $1
    call ${un}Trim
    Pop $R1
    
DoneLoop:
    FileClose $0

done:
    ; Restore registers    
    Pop $4
    Pop $3
    Pop $2
    Pop $1
    Pop $0
    
    Pop $R0
    Exch $R1    ; Save result and restore register value

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_GetAirAppVersion ""
!insertmacro FUNC_GetAirAppVersion "un."

;------------------------------------------------------------------------------
; GetUninstallStringBase
; Given an app name, find the uninstall string for that application
; The returned string is in HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\<program>\[UninstallString]
; Modifies no variables/registers
;
; Usage:
;   Push "EA Shared Game Component: PatchProgress"
;   Call GetUninstallStringBase
;   Pop $R0  ; $R0 now contains MsiExec.exe /X{FF66E9F6-83E7-3A3E-AF14-8DE9A809A6A4}
;------------------------------------------------------------------------------
!macro FUNC_GetUninstallStringBase un
Function ${un}GetUninstallStringBase

    Exch $R0
    Push $0
    Push $1
    Push $2

    StrCpy $0 0

loop:
    EnumRegKey $1 HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall" $0
    StrCmp $1 "" notfound
    IntOp $0 $0 + 1
    ReadRegStr $2 HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$1" "DisplayName"
    StrCmp $2 $R0 0 loop
    ; Skip if no UninstallString is present
    ReadRegStr $2 HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$1" "UninstallString"
    StrCmp $2 "" loop found

found:
    StrCpy $R0 $2
    Goto done

notfound:
    StrCpy $R0 ""
	
done:
    Pop $2
    Pop $1
    Pop $0
    Exch $R0

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_GetUninstallStringBase ""
!insertmacro FUNC_GetUninstallStringBase "un."

;------------------------------------------------------------------------------
; GetUninstallString
; Given an app name, find the uninstall string for that application
; The returned string is in HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\<program>\[UninstallString]
; For 64 bits systems the native 64 bits node is searched first and the virtualized Wow6432Node as second option
; Modifies no variables/registers
;
; Usage:
;   Push "EA Shared Game Component: PatchProgress"
;   Call GetUninstallString
;   Pop $R0  ; $R0 now contains MsiExec.exe /X{FF66E9F6-83E7-3A3E-AF14-8DE9A809A6A4}
;------------------------------------------------------------------------------
!macro FUNC_GetUninstallString un
Function ${un}GetUninstallString

    Exch $R0
    Push $0

    ${If} ${RunningX64}
        SetRegView 64
    ${EndIf}
	
	Push $R0
	Call ${un}GetUninstallStringBase
	Pop $0
	
	StrCmp $0 "" 0 found
	
	; Try the virtualized node
	${If} ${RunningX64}
        SetRegView 32
		Push $R0
		Call ${un}GetUninstallStringBase
		Pop $0
		StrCmp $0 "" 0 found		
    ${EndIf}
	
notfound:
    StrCpy $R0 ""
    Goto done

found:
    StrCpy $R0 $0
    Goto done

done:
    ${If} ${RunningX64}
        SetRegView 32
    ${EndIf}

    Pop $0
    Exch $R0

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_GetUninstallString ""
!insertmacro FUNC_GetUninstallString "un."


;------------------------------------------------------------------------------
; GetInstallGUID
; Given an app name, find the install information key and return the GUID (key name)
; The returned key is in HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Installer\UserData\S-1-5-18\Products\<GUID>
; Modifies no variables/registers
;
; Usage:
;   Push "EA Shared Game Component: PatchProgress"
;   Call GetInstallGUID
;   Pop $R0  ; $R0 now contains B0D844D25D026DC4487FBD8986BCF5C6
;------------------------------------------------------------------------------
!macro FUNC_GetInstallGUID un
Function ${un}GetInstallGUID

    Exch $R0
    Push $0
    Push $1
    Push $2
    
    StrCpy $0 0

loop:
    ${If} ${RunningX64}
        SetRegView 64
    ${EndIf}

    EnumRegKey $1 HKLM SOFTWARE\Microsoft\Windows\CurrentVersion\Installer\UserData\S-1-5-18\Products $0
    StrCmp $1 "" notfound
    IntOp $0 $0 + 1
    ReadRegStr $2 HKLM SOFTWARE\Microsoft\Windows\CurrentVersion\Installer\UserData\S-1-5-18\Products\$1\InstallProperties "DisplayName"
    StrCmp $2 $R0 found loop

found:
    StrCpy $R0 $1
    Goto done

notfound:
    StrCpy $R0 ""     
        
done:
    ${If} ${RunningX64}
        SetRegView 32
    ${EndIf}

    Pop $2
    Pop $1
    Pop $0
    Exch $R0

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_GetInstallGUID ""
!insertmacro FUNC_GetInstallGUID "un."

;------------------------------------------------------------------------------
; GetInstallRegKey
; Given an app name, find the install information key in the registry
; The returned key is in HKLM
; Modifies no variables/registers
;
; Usage:
;   Push "EA Shared Game Component: PatchProgress"
;   Call GetInstallRegKey
;   Pop $R0  ; $R0 now contains SOFTWARE\Microsoft\Windows\CurrentVersion\Installer\UserData\S-1-5-18\Products\815FEC512F479492E5BEBEDA8A3F3FB1
;------------------------------------------------------------------------------
!macro FUNC_GetInstallRegKey un
Function ${un}GetInstallRegKey

    Exch $R0
    Push $0
    
    Push $R0
    Call ${un}GetInstallGUID
    Pop $0
    
    StrCmp $0 "" notfound
    
    StrCpy $R0 "SOFTWARE\Microsoft\Windows\CurrentVersion\Installer\UserData\S-1-5-18\Products\$0"
    Goto done

notfound:
    StrCpy $R0 ""     
        
done:
    Pop $0
    Exch $R0

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_GetInstallRegKey ""
!insertmacro FUNC_GetInstallRegKey "un."

;------------------------------------------------------------------------------
; GetLCIDFromLanguageCode
; Given a language code in the form en_US (ISO 639 lang name + ISO 3166 2 letter ctry name
; get the windows locale ID
; Modifies no variables/registers
;
; Usage:
;   Push "en_US"
;   Call GetLCIDFromLanguageCode
;   Pop $R0  ; $R0 now contains 1033
;------------------------------------------------------------------------------
!macro FUNC_GetLCIDFromLanguageCode un
Function ${un}GetLCIDFromLanguageCode

    Exch $R0
    Push $0
    
	; I auto-generated this code with GetLocaleInfo.
	; Adding new languages should be fairly easy now
	
	; First check the full language code
	StrCmp $R0 "cs_CZ" 0 +3
	StrCpy $R0 "1029"
	Goto done
	StrCmp $R0 "da_DK" 0 +3
	StrCpy $R0 "1030"
	Goto done
	StrCmp $R0 "de_DE" 0 +3
	StrCpy $R0 "1031"
	Goto done
	StrCmp $R0 "el_GR" 0 +3
	StrCpy $R0 "1032"
	Goto done
	StrCmp $R0 "en_US" 0 +3
	StrCpy $R0 "1033"
	Goto done
	StrCmp $R0 "en_GB" 0 +3
	StrCpy $R0 "2057"
	Goto done
	StrCmp $R0 "es_ES" 0 +3
	StrCpy $R0 "1034"
	Goto done
	StrCmp $R0 "es_MX" 0 +3
	StrCpy $R0 "2058"
	Goto done
	StrCmp $R0 "fi_FI" 0 +3
	StrCpy $R0 "1035"
	Goto done
	StrCmp $R0 "fr_FR" 0 +3
	StrCpy $R0 "1036"
	Goto done
	StrCmp $R0 "hu_HU" 0 +3
	StrCpy $R0 "1038"
	Goto done
	StrCmp $R0 "it_IT" 0 +3
	StrCpy $R0 "1040"
	Goto done
	StrCmp $R0 "ja_JP" 0 +3
	StrCpy $R0 "1041"
	Goto done
	StrCmp $R0 "ko_KR" 0 +3
	StrCpy $R0 "1042"
	Goto done
	StrCmp $R0 "nl_NL" 0 +3
	StrCpy $R0 "1043"
	Goto done
	StrCmp $R0 "nb_NO" 0 +3
	StrCpy $R0 "1044"
	Goto done
	StrCmp $R0 "no_NO" 0 +3
	StrCpy $R0 "1044"
	Goto done
	StrCmp $R0 "pl_PL" 0 +3
	StrCpy $R0 "1045"
	Goto done
	StrCmp $R0 "pt_BR" 0 +3
	StrCpy $R0 "1046"
	Goto done
	StrCmp $R0 "pt_PT" 0 +3
	StrCpy $R0 "2070"
	Goto done
	StrCmp $R0 "sv_SE" 0 +3
	StrCpy $R0 "1053"
	Goto done
	StrCmp $R0 "zh_TW" 0 +3
	StrCpy $R0 "1028"
	Goto done
	StrCmp $R0 "zh_CN" 0 +3
	StrCpy $R0 "2052"
	Goto done
	StrCmp $R0 "th_TH" 0 +3
	StrCpy $R0 "1054"
	Goto done
	StrCmp $R0 "ru_RU" 0 +3
	StrCpy $R0 "1049"
	Goto done	
	
	; Cant find specific, then look for he two letter code
	StrCpy $R0 $R0 2
	
	StrCmp $R0 "cs" 0 +3
	StrCpy $R0 "1029"
	Goto done
	StrCmp $R0 "da" 0 +3
	StrCpy $R0 "1030"
	Goto done
	StrCmp $R0 "de" 0 +3
	StrCpy $R0 "1031"
	Goto done
	StrCmp $R0 "el" 0 +3
	StrCpy $R0 "1032"
	Goto done
	StrCmp $R0 "en" 0 +3
	StrCpy $R0 "1033"
	Goto done
	StrCmp $R0 "es" 0 +3
	StrCpy $R0 "1034"
	Goto done
	StrCmp $R0 "fi" 0 +3
	StrCpy $R0 "1035"
	Goto done
	StrCmp $R0 "fr" 0 +3
	StrCpy $R0 "1036"
	Goto done
	StrCmp $R0 "hu" 0 +3
	StrCpy $R0 "1038"
	Goto done
	StrCmp $R0 "it" 0 +3
	StrCpy $R0 "1040"
	Goto done
	StrCmp $R0 "ja" 0 +3
	StrCpy $R0 "1041"
	Goto done
	StrCmp $R0 "ko" 0 +3
	StrCpy $R0 "1042"
	Goto done
	StrCmp $R0 "nl" 0 +3
	StrCpy $R0 "1043"
	Goto done
	StrCmp $R0 "nb" 0 +3
	StrCpy $R0 "1044"
	Goto done
	StrCmp $R0 "pl" 0 +3
	StrCpy $R0 "1045"
	Goto done
	StrCmp $R0 "pt" 0 +3
	StrCpy $R0 "1046"
	Goto done
	StrCmp $R0 "sv" 0 +3
	StrCpy $R0 "1053"
	Goto done
	StrCmp $R0 "zh" 0 +3
	StrCpy $R0 "1028"
	Goto done
	StrCmp $R0 "zh" 0 +3
	StrCpy $R0 "2052"
	Goto done
	StrCmp $R0 "th" 0 +3
	StrCpy $R0 "1054"
	Goto done
	StrCmp $R0 "ru" 0 +3
	StrCpy $R0 "1049"
	Goto done
	
notfound:
    StrCpy $R0 ""     
        
done:
    Pop $0
    Exch $R0

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_GetLCIDFromLanguageCode ""
!insertmacro FUNC_GetLCIDFromLanguageCode "un."

;------------------------------------------------------------------------------
; CharToASCII
; Convert a character to its ascii value
;------------------------------------------------------------------------------
!macro FUNC_CharToASCII un
Function ${un}CharToASCII

    Exch $0 ; given character
    Push $1 ; current character
    Push $2 ; current Ascii Code

    StrCpy $2 1 ; right from start

Loop:
     IntFmt $1 %c $2 ; Get character from current ASCII code
     ${If} $1 S== $0 ; case sensitive string comparison
        StrCpy $0 $2
        Goto Done
     ${EndIf}
     IntOp $2 $2 + 1
     StrCmp $2 255 0 Loop ; ascii from 1 to 255
     StrCpy $0 0 ; ASCII code was'nt found -> return 0

Done:
     Pop $2
     Pop $1
     Exch $0

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_CharToASCII ""
!insertmacro FUNC_CharToASCII "un."

;------------------------------------------------------------------------------
; StrHash
; Returns the hash of a string (DJB hash)
;------------------------------------------------------------------------------
!macro FUNC_StrHash un
Function ${un}StrHash

    Exch $0
    Push $1
    Push $2
    Push $3

    StrCpy $2 "5381"

loop:
    StrCmp $0 "" done
    
    ; Extract first char
    StrCpy $1 $0 1
    StrCpy $0 $0 "" 1

    ; Convert to ASCII
    Push $1
    Call ${un}CharToASCII
    Pop $1

    IntOp $3 $2 << 5 ; hash << 5
    IntOp $2 $3 + $2 ; (hash << 5) + hash
    IntOp $2 $2 + $1 ; ((hash << 5) + hash) + *str
    Goto loop
    
done:
    StrCpy $0 $2
    Pop $3
    Pop $2
    Pop $1
    Exch $0

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_StrHash ""
!insertmacro FUNC_StrHash "un."

; BringWindowToForeground
; brings the main window into the foreground
; Usage:
;   Call BringWindowToForeground
;------------------------------------------------------------------------------
!macro FUNC_BringWindowToForeground un
Function ${un}BringWindowToForeground
    Push $R0
    Push $R1
   	Push $0
   	Push $1
   	Push $2
    
    StrCpy $1 "0"           ; HWND iterator
    StrLen $0 "$(^Name)"    ; Buffer len for window name
    IntOp $0 $0 + 1

loop:
    FindWindow $1 '#32770' '' 0 $1
    IntCmp $1 0 done
    System::Call "user32::GetWindowText(i r1, t .r2, i r0) i."
    StrCmp $2 "$(^Name)" 0 loop
    
    System::Call "user32::ShowWindow(i r1,i 9) i."         ; If minimized then restore (SW_RESTORE == 9)
    System::Call "user32::SetForegroundWindow(i r1) i."    ; Bring to front

done:
		
   	Pop $2
   	Pop $1
   	Pop $0
		Pop $R1
		Pop $R0

FunctionEnd    
!macroend 
   
; Insert function as an installer and uninstaller function.
!insertmacro FUNC_BringWindowToForeground ""
!insertmacro FUNC_BringWindowToForeground "un."

