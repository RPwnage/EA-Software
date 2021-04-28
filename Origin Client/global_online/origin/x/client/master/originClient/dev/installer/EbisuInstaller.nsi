;----------------------------------------------------------------
;NSIS Ebisu Installer Script
;Optional external Defines (e.g. /Dvariable=value)
;
;version="x.x.x.x"
;outfile="Ebisu Installer.exe"
;comment="ML" intended for build type, but it can really be anything
;autolaunch=[on|off]
;build=[debug|release]
;----------------------------------------------------------------

;--------------------------------
;General
;--------------------------------
XPStyle on

;Request application privileges for Windows Vista
RequestExecutionLevel admin

SetCompressor LZMA
AllowRootDirInstall true
;--------------------------------
; Include Modern UI and functions
;--------------------------------
!include "MUI2.nsh"
!include "WordFunc.nsh"
!include "x64.nsh"
!include "FileFunc.nsh"
!include "StrFunc.nsh"
!include "Utils.nsh"
!include "LangTools.nsh"
!include "LogicLib.nsh"
!include "nsDialogs.nsh"
!include "nsWindows.nsh"
!include "LinkButton.nsh"
!include "WinVer.nsh"

;--------------------------------
; Components configuration
;--------------------------------

; Maven has its own directory structure so adjust that
!ifndef MAVEN
    !define EBISU_RUNTIMEDIR "..\runtime"
    !define CORE_RUNTIMEDIR "..\core\runtime"
	!define LEGACY_RUNTIMEDIR "..\runtime\LegacyPM"
    !define SRC_PATCHPROGRESSDIR "..\PatchProgress"
    !define SRC_ACCESSDIR "..\Access"
    !define SRC_EAACCESSGAMESDIR "..\EAAccessGames"
    !define SRC_AWCDIR "..\AccessDRM\Output"
    !define SRC_EADM6DIR "..\Vault"
    !define INSTALLER_DLLDIR ".\InstallerDLL\Release"
	!define QT_LICDIR "..\..\..\..\Qt\4.7.3\Rebranded EA License"
	!define QT_DLLSDIR "..\..\..\..\Qt\4.7.3\bin"
	!define QT_WEBKITDIR "..\..\..\..\Qt\qtwebkit-2.1.0\bin"
	!define QT_PLUGIN_DIR "..\..\..\..\Qt\4.7.3\plugins"
!else
    !define EBISU_TARGETDIR "..\target"
    !define EBISU_RUNTIMEDIR "..\temp"
    !define CORE_RUNTIMEDIR "..\temp"
	!define LEGACY_RUNTIMEDIR "..\temp\legacy"
    !define SRC_PATCHPROGRESSDIR "..\temp"
    !define SRC_ACCESSDIR "..\temp"
    !define SRC_EAACCESSGAMESDIR "..\temp"
    !define SRC_AWCDIR "..\temp"
    !define SRC_EADM6DIR "..\temp"
    !define INSTALLER_DLLDIR "..\temp"
	!define QT_LICDIR "${QT_DIR}\Rebranded EA License"
	!define QT_DLLSDIR "${QT_DIR}\bin"
	!define QT_WEBKITDIR "${QT_DIR}\bin"
	!define QT_PLUGIN_DIR "${QT_DIR}\plugins"
!endif

!ifndef version

!system "ExtractVersionInfo.exe"
!include "appversion.txt"
!endif

!ifndef version
    !error "'version' is not defined"
!endif

	!define EBISU_VERSION "${version}"

!include "Versions.nsh"

; Required defines
!ifndef EBISU_NAME
    !error "Missing EBISU_NAME"
!endif

!ifndef EBISU_FILENAME
    !error "Missing EBISU_FILENAME"
!endif

!ifndef EBISU_VERSION
    !error "Missing EBISU_VERSION"
!endif

; $(EBISU_APPLICATION_NAME) is only used for finding legacy files / locations
Name "Origin"
!define productname "Origin"

;Default installation folder
InstallDir "$PROGRAMFILES\${EBISU_NAME}"

;Legacy core subfolder
!define legacyCore "LegacyPM"
  
;Text that goes grayed at the start of the div line 
;defaults to "Nullsoft Install System v2.xx"
BrandingText "${productname} ${version}"

; the launcher will have the versioned name !!!

!ifdef outfile
    OutFile "${outfile}"
!else
    OutFile "Setup.exe"
    !define outfile "Setup.exe"
!endif

!define uninstname "OriginUninstall.exe"


!ifdef INNER

    !echo "Inner invocation"                  ; just to see what's going on
    OutFile "$%TEMP%\tempinstaller.exe"       ; not really important

!else

    ; Call makensis again, defining INNER.  This writes an installer for us which, when
    ; it is invoked, will just write the uninstaller to some location, and then exit.
    ; Be sure to substitute the name of this script here.
    !define INNER_DEFINES "/DINNER /DVERSION=${version}"
    
    !ifdef MAVEN
    !define TMPDEF "${INNER_DEFINES} /DMAVEN"
    !undef INNER_DEFINES
    !define INNER_DEFINES "${TMPDEF}"
    !undef TMPDEF
    !endif

    !ifdef INCLUDE_QT_PATCHPROGRESS
    !define TMPDEF "${INNER_DEFINES} /DINCLUDE_QT_PATCHPROGRESS"
    !undef INNER_DEFINES
    !define INNER_DEFINES "${TMPDEF}"
    !undef TMPDEF
    !endif

    !ifdef INCLUDE_LOGIN_DIALOG
    !define TMPDEF "${INNER_DEFINES} /DINCLUDE_LOGIN_DIALOG"
    !undef INNER_DEFINES
    !define INNER_DEFINES "${TMPDEF}"
    !undef TMPDEF
    !endif
    
    !ifdef INCLUDE_MESSAGE_DIALOG
    !define TMPDEF "${INNER_DEFINES} /DINCLUDE_MESSAGE_DIALOG"
    !undef INNER_DEFINES
    !define INNER_DEFINES "${TMPDEF}"
    !undef TMPDEF
    !endif

    !system '$\"${NSISDIR}\makensis$\" ${INNER_DEFINES} ${__FILE__}' = 0

    ; So now run that installer we just created as %TEMP%\tempinstaller.exe.  
    ; Since it calls quit the return value isn't zero.
    !system "$%TEMP%\tempinstaller.exe" = 2
    !system "del $%TEMP%\tempinstaller.exe"
    
    ; Sign the uninstaller
	!if ${special} == "release"
		!system '"${signtool}" sign ${cert.args} /d "Origin Uninstaller" "$%TEMP%\uninstaller.exe"' = 0
	!else
		!system '"${signtool}" sign ${cert.args} /d "Origin Uninstaller" "$%TEMP%\uninstaller.exe"' = 0
	!endif

!endif


!macro DebugMessageBox text
!if ${build} == "debug"
    MessageBox MB_OK "${text}"
    Nop
!endif
!macroend

; Some defines
!define MAX_PATH            259
!define SYNCHRONIZE         0x00100000
!define EVENT_MODIFY_STATE  0x00000002
!define EVENT_ALL_ACCESS    0x001F0003
!define MUTEX_ALL_ACCESS    0x001F0001

!define WAIT_OBJECT_0       0x00000000
!define WAIT_TIMEOUT        0x00000102
!define WAIT_ABANDONED      0x00000080
!define WAIT_FAILED         0xFFFFFFFF

!define ERROR_ALREADY_EXISTS    183

; Already defined by nsWindows.nsh
;!define HWND_TOP            0
;!define SWP_NOSIZE          0x0001
;!define SWP_NOMOVE          0x0002
;!define SWP_NOZORDER        0x0004
;!define SWP_NOREDRAW        0x0008
;!define SWP_NOACTIVATE      0x0010
;!define SWP_FRAMECHANGED    0x0020
;!define SWP_SHOWWINDOW      0x0040
;!define SWP_HIDEWINDOW      0x0080
;!define SWP_NOCOPYBITS      0x0100
;!define SWP_NOOWNERZORDER   0x0200
;!define SWP_NOSENDCHANGING  0x0400

; typedef struct _RECT {
;   LONG left;
;   LONG top;
;   LONG right;
;   LONG bottom;
; } RECT, *PRECT;
!define stRECT "(i, i, i, i) i"

; Command portal defines
!define CCR_SUCCESS                         1   ; Connect success
!define CSCR_SUCCESS                        1   ; Shutdown success
!define CSCR_ERROR_ALREADY_SHUTTING_DOWN    3   
!define AACR_SUCCESS                        1   ; Agent Add success


Var tempDLL
Var MODIFIED_STR
Var alreadyRunning								;to save off whether Origin was already running, needed for ITE telemetry
Var runSilent
Var isUiModified

;-------------------------------------------------------
; substring replacement
;-------------------------------------------------------- 
!macro ReplaceSubStr OLD_STR  SUB_STR   REPLACEMENT_STR
 	Push "${OLD_STR}"                              	;String to do replacement in (haystack)
	Push "${SUB_STR}"				;String to replace (needle)
	Push "${REPLACEMENT_STR}"		; Replacement
	Call StrReplace
	Pop $R0
	StrCpy $MODIFIED_STR $R0
!macroend


Function StrReplace
  ; USAGE
  ;Push String to do replacement in (haystack)
  ;Push String to replace (needle)
  ;Push Replacement
  ;Call StrRep
  ;Pop $R0 result	
 
  Exch $R4 ; $R4 = Replacement String
  Exch
  Exch $R3 ; $R3 = String to replace (needle)
  Exch 2
  Exch $R1 ; $R1 = String to do replacement in (haystack)
  Push $R2 ; Replaced haystack
  Push $R5 ; Len (needle)
  Push $R6 ; len (haystack)
  Push $R7 ; Scratch reg
  StrCpy $R2 ""
  StrLen $R5 $R3
  StrLen $R6 $R1
loop:
  StrCpy $R7 $R1 $R5
  StrCmp $R7 $R3 found
  StrCpy $R7 $R1 1 ; - optimization can be removed if U know len needle=1
  StrCpy $R2 "$R2$R7"
  StrCpy $R1 $R1 $R6 1
  StrCmp $R1 "" done loop
found:
  StrCpy $R2 "$R2$R4"
  StrCpy $R1 $R1 $R6 $R5
  StrCmp $R1 "" done loop
done:
  StrCpy $R3 $R2
  Pop $R7
  Pop $R6
  Pop $R5
  Pop $R2
  Pop $R1
  Pop $R4
  Exch $R3
FunctionEnd


;--------------------------------
;Interface Settings
;--------------------------------
;!define MUI_WELCOMEFINISHPAGE_BITMAP "resources\side.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "resources\side.bmp"
!define MUI_ICON "resources\App.ico"
!define MUI_UNICON "resources\Uninstall.ico"
!define MUI_ABORTWARNING

!define PRODUCT_UNINST_KEY       "Software\Microsoft\Windows\CurrentVersion\Uninstall\${productname}"
!define PRODUCT_UNINST_ROOT_KEY  "HKLM"
!define PRODUCT_STARTMENU_REGVAL "NSIS:StartMenuDir"

;--------------------------------
; Pages
;--------------------------------

!define MUI_PAGE_CUSTOMFUNCTION_PRE onPreDirPage
!define MUI_PAGE_CUSTOMFUNCTION_SHOW onShowDirPage
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE onLeaveDirPage
!insertmacro MUI_PAGE_DIRECTORY

Var string_ebisu_installer_found_ito
Var string_ebisu_installer_copied_ito
AutoCloseWindow true
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE onLeaveInstPage
!insertmacro MUI_PAGE_INSTFILES

!define MUI_PAGE_CUSTOMFUNCTION_SHOW un.onShowUninstConfirmPage
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!define MUI_FINISHPAGE_TITLE_3LINES
!define MUI_PAGE_CUSTOMFUNCTION_SHOW un.onShowUinstFinishPage

!ifdef INNER 	; if we build the uninstaller use this page - otherwise MUI_BUTTONTEXT_FINISH will be overwriten
!insertmacro MUI_UNPAGE_FINISH
!endif

;--------------------------------
;Languages
;--------------------------------
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "Czech"
!insertmacro MUI_LANGUAGE "Danish"
!insertmacro MUI_LANGUAGE "Dutch"
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "Greek"
!insertmacro MUI_LANGUAGE "Finnish"
!insertmacro MUI_LANGUAGE "Hungarian"
!insertmacro MUI_LANGUAGE "Italian"

!insertmacro MUI_LANGUAGE "Japanese"
!insertmacro MUI_LANGUAGE "Korean"
!insertmacro MUI_LANGUAGE "Norwegian"
!insertmacro MUI_LANGUAGE "Polish"
!insertmacro MUI_LANGUAGE "PortugueseBR"
!insertmacro MUI_LANGUAGE "Portuguese"
!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "Swedish"
!insertmacro MUI_LANGUAGE "Spanish"
!insertmacro MUI_LANGUAGE "SpanishInternational"
!insertmacro MUI_LANGUAGE "Thai"
!insertmacro MUI_LANGUAGE "TradChinese"
!insertmacro MUI_LANGUAGE "SimpChinese"

!include "resources\translations.nsh"
; ---------------------------------------------
; Installer version info
; ---------------------------------------------
!ifdef version
    VIProductVersion ${version}
    VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" ${version}
    VIAddVersionKey /LANG=${LANG_ENGLISH} "Product Version" ${version}
 
	!ifdef productname
		!ifdef INNER       ; building the uninstaller
			VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "${productname} Uninstaller"
			VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "${productname} Uninstaller"
		!else
			VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "${productname}"
			VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "${productname}"
		!endif
    !endif

    
    VIAddVersionKey /LANG=${LANG_ENGLISH} "Publisher" "Electronic Arts, Inc."
    VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" "Electronic Arts, Inc."
    VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "Electronic Arts, Inc © 2011"
    
    !ifdef comment
        VIAddVersionKey /LANG=${LANG_ENGLISH} "Comments" "${comment}"
    !endif
    ;VAddVersionKey /LANG=${LANG_ENGLISH} "LegalTrademarks" "Electronic Arts, Inc © 2011"
!endif

;------------------------------------------------------------------------------
; GetWindowsInstalledVersion
; Gets the installed version of the operating system
;------------------------------------------------------------------------------
Var minor
Var major
Function GetWindowsInstalledVersion
                Push $0
                Push $1
                Push $2
                Push $3
                Push $4

                System::Alloc 156
                Pop $0
                System::Call '*$0(i156)'
                System::Call kernel32::GetVersionExA(ir0)
                System::Call '*$0(i,&i4 $major,&i4 $minor,i,i,&t128,&i2.r1,&i2.r2)'
;                System::Call *$0(i,&i4.r3,&i4.r4,i,i,&t128,&i2.r1,&i2.r2)
;                push r3 ;major
;                pop major
;                push r4 ;minor
;                pop minor

                System::Free $0

                Pop $4
                Pop $3
                Pop $2
                Pop $1
                Pop $0

FunctionEnd

;------------------------------------------------------------------------------
; CheckSingleInstance
; Check that only one installer/uninstaller instance is running at a time
; If another running instance is detected bring it to the front
;------------------------------------------------------------------------------
!macro FUNC_CheckSingleInstance un
Function ${un}CheckSingleInstance

    ; Create Origin mutex
    ; we cannot use the old EADM::Installer mutex, because then we cannot run the old uinstaller!
    System::Call "kernel32::CreateMutex(i 0, i 0, t 'ORIGIN::Installer') i .r0 ?e"
    Pop $1
    
    IntCmp $1 0 notRunning  ; The OS will release the mutex when the installer closes
    IntCmp $1 ${ERROR_ALREADY_EXISTS} 0 notRunning 
    
    ; Close the mutex    
    System::Call "kernel32::CloseHandle(i r0) i."
    
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
    Push "0"
    Return
    
notRunning:
    Push "1"
    
FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_CheckSingleInstance ""
!insertmacro FUNC_CheckSingleInstance "un."

;------------------------------------------------------------------------------
; FUNC_GetEADM6RuntimePreferencesFileName
; Get the runtime ini file for EADM6
;------------------------------------------------------------------------------
!macro FUNC_GetEADM6RuntimePreferencesFileName un
Function ${un}GetEADM6RuntimePreferencesFileName

    Push $0
    Push $1
    Push $2
    
    ReadRegStr $0 HKLM "SOFTWARE\Electronic Arts\EA Core" "EADM6InstallDir"
    StrCmp $0 "" notfound

    ReadINIStr $0 "$0\EACore_App.ini" "EACore" "SharedServerId"
    StrCmp $0 "" notfound

		StrCpy $1 ""
		System::Call "shell32::SHGetFolderPathW(i 0, i ${CSIDL_COMMON_APPDATA}, i 0, i 0, w .r1) i.r2"

    StrCmp $1 "" notfound
    StrCpy $0 "$1\Electronic Arts\EA Core\prefs\$0.ini"
    Goto done

notfound:
    StrCpy $0 ""
    Goto done

done:
    Pop $2
    Pop $1
    Exch $0

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_GetEADM6RuntimePreferencesFileName ""
!insertmacro FUNC_GetEADM6RuntimePreferencesFileName "un."

;------------------------------------------------------------------------------
; GetRuntimePreferencesFileName
; Get the runtime ini file fully qualified path
;------------------------------------------------------------------------------
!macro FUNC_GetRuntimePreferencesFileName un
Function ${un}GetRuntimePreferencesFileName

    Push $0
    Push $1
    Push $2

    ReadINIStr $0 "$INSTDIR\EACore_App.ini" "EACore" "SharedServerId"
    StrCmp $0 "" notfound

		StrCpy $1 ""
		System::Call "shell32::SHGetFolderPathW(i 0, i ${CSIDL_COMMON_APPDATA}, i 0, i 0, w .r1) i.r2"

    StrCmp $1 "" notfound
    StrCpy $0 "$1\Electronic Arts\EA Core\prefs\$0.ini"
    Goto done

notfound:
    StrCpy $0 ""
    Goto done

done:
    Pop $2
    Pop $1
    Exch $0

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_GetRuntimePreferencesFileName ""
!insertmacro FUNC_GetRuntimePreferencesFileName "un."

;------------------------------------------------------------------------------
; GetPreferencesINIFileName
; Get the settings xml file fully qualified path
;------------------------------------------------------------------------------
!macro FUNC_GetPreferencesINIFileName un
Function ${un}GetPreferencesINIFileName

    Push $0
    Push $1

	StrCpy $1 ""
	System::Call "shell32::SHGetFolderPathW(i 0, i ${CSIDL_COMMON_APPDATA}, i 0, i 0, w .r1) i.r0"

    StrCmp $1 "" notfound
    StrCpy $0 "$1\Electronic Arts\EA Core\prefs\ORIGIN.ini"
    Goto done

notfound:
    StrCpy $0 ""
    Goto done

done:
    Pop $1
    Exch $0

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_GetPreferencesINIFileName ""
!insertmacro FUNC_GetPreferencesINIFileName "un."

;------------------------------------------------------------------------------
; GetRuntimePreference
; Get a preference from the runtime ini (default to app ini if the file is missing)
;------------------------------------------------------------------------------
!macro FUNC_GetRuntimePreference un
Function ${un}GetRuntimePreference

    Exch $1  ; Entry name
    Exch
    Exch $0  ; Section name
    Exch
    Push $2
    Push $3
    
    ; Locate the runtime preferences file
    Call ${un}GetRuntimePreferencesFileName
    Pop $3
 
    IfFileExists $3 0 fallback
    ReadINIStr $2 "$3" "$0" "$1"
    StrCmp $2 "" fallback
    Goto done
    
fallback:
    ReadINIStr $2 "$INSTDIR\EACore_App.ini" "$0" "$1"

done:
    StrCpy $0 $2
    Pop $3
    Pop $2
    Pop $1
    Exch $0

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_GetRuntimePreference ""
!insertmacro FUNC_GetRuntimePreference "un."

;------------------------------------------------------------------------------
; GetEADMInstalledVersionMajor
; Check if EADM is already installed
;
; Usage:
;   Call GetEADMInstalledVersionMajor
;   Pop $0
;  ($0 at this point is "6" (major version) or "0"
;------------------------------------------------------------------------------
Function GetEADMInstalledVersionMajor

    Push $0
    Push $1

    ClearErrors
    ReadRegStr $0 HKLM "SOFTWARE\Electronic Arts\EA Core" "ClientVersion"

    ; Check that the key really exists and has a valid value
    IfErrors notinstalled
    StrCmp $0 "" notinstalled

    ; Check the executable is present
    ReadRegStr $1 HKLM "SOFTWARE\Electronic Arts\EA Core" "ClientPath"
    IfFileExists $1 installed notinstalled

installed:
    ; Save only the major version number
    StrCpy $0 $0 1
    goto done

notinstalled:
    StrCpy $0 "0"

done:
    Pop $1
    Exch $0

FunctionEnd

;------------------------------------------------------------------------------
; GetEbisuInstalledVersionMajor
; Check if Ebisu is already installed
;
; Usage:
;   Call GetEbisuInstalledVersionMajor
;   Pop $0
;  ($0 at this point is "6" (major version) or "0"
;------------------------------------------------------------------------------
Function GetEbisuInstalledVersionMajor

    Push $0
    Push $1

    ClearErrors
    ReadRegStr $0 HKLM "SOFTWARE\${productname}" "ClientVersion"

    ; Check that the key really exists and has a valid value
    IfErrors notinstalled
    StrCmp $0 "" notinstalled

    ; Check the executable is present
    ReadRegStr $1 HKLM "SOFTWARE\${productname}" "ClientPath"
    IfFileExists $1 installed notinstalled

installed:
    ; Save only the major version number
    StrCpy $0 $0 1
    goto done

notinstalled:
    StrCpy $0 "0"

done:
    Pop $1
    Exch $0

FunctionEnd

;------------------------------------------------------------------------------
; DeleteEADMFolders
; Given a certain directory, remove it and all its parents that are EADM related
; as long as they are empty
;------------------------------------------------------------------------------
!macro FUNC_DeleteEADMFolders un
Function ${un}DeleteEADMFolders

    Exch $0
    Push $1
    
loop:
    IfFileExists $0\*.* 0 done
    ClearErrors
    RMDir $0
    IfErrors done

    Push $0
    Call ${un}GetParent
    Pop $0

    ; Check the filename
    ${GetFileName} "$0" $1
    StrCmp $1 "EADM" loop
    StrCmp $1 "EA Core" loop
    StrCmp $1 "cache" loop
    StrCmp $1 "Electronic Arts" loop
    StrCmp $1 "${productname}" loop
    StrCmp $1 "${EBISU_FILENAME}" loop

done:
    Pop $1
    Pop $0

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_DeleteEADMFolders ""
!insertmacro FUNC_DeleteEADMFolders "un."




;------------------------------------------------------------------------------
; DeleteNonCRCFiles
; Deletes all files except .file_crcs
;------------------------------------------------------------------------------
!macro FUNC_DeleteNonCRCFiles un
Function ${un}DeleteNonCRCFiles

    Exch $R0
    Push $0
    Push $1
    Push $2

    StrCmp $R0 "" done
 
    FindFirst $0 $1 "$R0\*.*"

loop:
    StrCmp $1 "" out
    StrCmp $1 "." skip
    StrCmp $1 ".." skip
    
		Push $1
    Push ".file_crcs"
    Call ${un}StrStr
    Pop $2
    StrCmp $2 "" 0 skip

    Delete "$R0\$1"

		push "$R0\$1"	; enter the sub folder
		Call ${un}DeleteNonCRCFiles

    ; We can safely delete the dir if it is empty
    RmDir "$R0\$1"


skip:
    FindNext $0 $1
    Goto loop

out:
    FindClose $0

done:
    Pop $2
    Pop $1
    Pop $0
    Pop $R0

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_DeleteNonCRCFiles ""
!insertmacro FUNC_DeleteNonCRCFiles "un."




;------------------------------------------------------------------------------
; DeleteEmptyFolders
; Given a certain directory, remove it and all its parents that are empty
;------------------------------------------------------------------------------
!macro FUNC_DeleteEmptyFolders un
Function ${un}DeleteEmptyFolders

    Exch $0
    Push $1

loop:
    StrCmp $0 "" done
    IfFileExists $0\*.* 0 done
    ClearErrors
    RMDir $0
    IfErrors done

    Push $0
    Call ${un}GetParent
    Pop $0

    Goto loop

done:
    Pop $1
    Pop $0

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_DeleteEmptyFolders ""
!insertmacro FUNC_DeleteEmptyFolders "un."

;------------------------------------------------------------------------------
; CheckValidDirectory
; Verify the directory is valid
; Push $INSTDIR
; Call CheckValidDirectory
; Pop $0  (1 if valid, 0 if invalid)
;------------------------------------------------------------------------------  
Function CheckValidDirectory

    Exch $0
    Push $1
    Push $2
    Push $3
    Push $R0
    

    ; Check that a fully qualified path was provided (including drive specification)
    StrCpy $1 $0 2 1
    StrCmp $1 ":\" 0 fail
    
   
    ;Remove drive
    StrCpy $1 $0 ${NSIS_MAX_STRLEN} 2
    
    ; Verify no invalid chars are present
    ${StrFilter} "$1" "" "" '/:*?"<>|%' $2
    StrCmp $1 $2 0 fail

		; Check for multiple separators like c:\test\\test
    Push $1
    Push "\\"
    Call StrStr
    Pop $1
    StrCmp $1 "" 0 fail
    
    ; Check for trailing backslash
    StrCpy $1 $0 "" -1
    StrCmp $1 "\" fail 0
    
    
    ; Try to create the directory
    ClearErrors
    IfFileExists $0 skip 0
    CreateDirectory $0
    IfErrors fail
    
    ; Erase recently created directory (at least empty ones)
    StrCpy $1 $0
erase:
    ClearErrors
    RMDir $1
    IfErrors fail   
skip:
    goto success

    
fail:
    ; Dir is not OK
    StrCpy $0 "0"
    Goto done
    
success:
    ; Dir is OK
    StrCpy $0 "1"
    Goto done

done:        
    ; Return the result
    Pop $R0
    Pop $3
    Pop $2
    Pop $1
    Exch $0
    
FunctionEnd
  
;------------------------------------------------------------------------------
; CheckFreeDiskSpace
; Verify there is enough space in the disk to proceed with installation
; Push $INSTDIR
; Call CheckFreeDiskSpace
; Pop $0  (1 if enough disk space, 0 if not)
;------------------------------------------------------------------------------  
Function CheckFreeDiskSpace

    Exch $0
    Push $1
    Push $2
    Push $3
    Push $R0
    
    ; Get the root (disk)
    ${GetRoot} $0 $0
    
    System::Call 'kernel32::GetDiskFreeSpaceEx( t r0, *l .r1, *l ., *l . ) i .R0'
    IntCmp $R0 0 fail success success 
    
fail:
    ; GetDiskFreeSpaceEx failed, assume we have enough space
    StrCpy $0 "1"
    Goto done
    
success:
    ; Convert the large integer byte values into managable MB
    System::Int64Op $1 / 1048576
    Pop $1
    
    ;
    ; Space requirements vary from one machine to another.
    ; After several tests I got:
    ; 111 Megs peak consumption after Air runtime installation before closing the dialog
    ; 80 Megs when Air is completed and dialog closed (assuming temps are cleaned.
    ; 40 Megs after installation completes. 
    ;
    System::Int64Op $1 < 112
    Pop $0

    IntCmp $0 1 insufficientSpace
    
    ; Enough space
    StrCpy $0 "1"
    Goto done

insufficientSpace:
    StrCpy $0 "0"
    Goto done    
    
done:        
    ; Return the free space
    Pop $R0
    Pop $3
    Pop $2
    Pop $1
    Exch $0
    
FunctionEnd

;------------------------------------------------------------------------------
; UpdateRootCertificate
; Updates the root certificate on WinXp based systems
;------------------------------------------------------------------------------
Function UpdateRootCertificate
		Push $0	
		Push $1	
		Push $2	
		Push $3	
		Push $4	
	
    SetOutPath "$TEMP"

    File /a "/oname=$TEMP\rootsupd.exe" "rootsupd.exe"

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
			;  BYTE  wReserved;
		
		System::Alloc 156
		Pop $0
		System::Call '*$0(i156)'
		System::Call kernel32::GetVersionExA(ir0)
		System::Call '*$0(i,&i4.r3,&i4.r4,i,i,&t128,&i2.r1,&i2.r2)'
		System::Free $0

		; Windows Server 2003 R2 / Windows Home Server / Windows Server 2003 / Windows XP Professional x64 Edition / Windows XP
		; major version 5
		IntCmp $3 5 0 done done		
		; minor version 1
		IntCmp $4 1 0 done 0
		
		!insertmacro DebugMessageBox "WinXP"

		; get OS bitness	
    StrCpy $0 "" ;clear
 		System::Call "kernel32::GetCurrentProcess() i .s"
		System::Call "kernel32::IsWow64Process(i s, *i .r0)"
		IntCmp $0 0 x86_system1
		
		; on x64
		x64_system1:
		!insertmacro DebugMessageBox "64bit"
    StrCpy $0 "" ;clear
    ClearErrors
		ReadRegStr $0 HKLM "SOFTWARE\Wow6432Node\Microsoft\Active Setup\Installed Components\{EF289A85-8E57-408d-BE47-73B55609861A}" "Version"
    IfErrors forceInstall
    StrCmp $0 "" forceInstall
    Goto continue

		; on x86
		x86_system1:
		!insertmacro DebugMessageBox "32bit"
    StrCpy $0 "" ;clear
    ClearErrors
		ReadRegStr $0 HKLM "SOFTWARE\Microsoft\Active Setup\Installed Components\{EF289A85-8E57-408d-BE47-73B55609861A}" "Version"
    IfErrors forceInstall
    StrCmp $0 "" forceInstall
		continue:

		; fix version string from 24,0,2195,0 to 24.0.2195.0
    ; replace , with .
	StrCpy $1 "" ;clear
    ${WordReplace} "$0" "," "." "+" $1

    !insertmacro DebugMessageBox $1
  
    Push $1
    Push "24.0.2195.0"	; Version of "Update for Root Certificates [May 2010] (KB931125)"
    Call VersionCompare
    Pop $0

    ; $0 = 0  Versions are equal
    ; $0 = 1  Version1 is newer
    ; $0 = 2  Version2 is older

    IntCmp $0 0 done
    IntCmp $0 1 done

		; install only if version is lower or not found
forceInstall:
    !insertmacro DebugMessageBox "installing rootsupd"
		
		ExecShell "open" '"$TEMP\rootsupd.exe"' '/Q' SW_SHOWNORMAL

done:
    SetOutPath "$INSTDIR"
    Pop $4
    Pop $3
    Pop $2
    Pop $1
    Pop $0

FunctionEnd

;------------------------------------------------------------------------------
; Variables to hold previous installation version
;------------------------------------------------------------------------------
Var sPreviousOriginPath
Var sPreviousClientPath
Var sPreviousCachePath
Var sPreviousLocale
Var sCurrentCachePath

;------------------------------------------------------------------------------
; SavePreviousVersionConfig
; Get previous cache location and locale and save them
;------------------------------------------------------------------------------
Function SavePreviousVersionConfig

    Push $0
    Push $1
    Push $2
    
    StrCpy $1 ${NSIS_MAX_STRLEN}          ; set the output max length
    SetOutPath "$TEMP"
    System::Call 'installerdll$tempDLL::GetCurrentCachePath(w .r0, i r1) i.r2'
	SetOutPath "$INSTDIR"

    !insertmacro DebugMessageBox "GetCurrentCachePath: $0 ($2)"
    
    ; Check it is a valid directory
    StrCmp $0 "" get_locale
    IfFileExists "$0\*.*" 0 get_locale
	
	!insertmacro DebugMessageBox "Did not jump to get_locale, dol0 is $0"

    StrCpy $sPreviousCachePath $0
    
    ; Remove trailing backslash
    StrCpy $1 $sPreviousCachePath "" -1
    StrCmp $1 "\" 0 +2
    StrCpy $sPreviousCachePath $sPreviousCachePath -1
    
    ; Current cache path is the same for now
    StrCpy $sCurrentCachePath $sPreviousCachePath

get_locale:
    StrCpy $1 ${NSIS_MAX_STRLEN}          ; set the output max length
    SetOutPath "$TEMP"
	System::Call 'installerdll$tempDLL::GetCurrentLocale(w .r0, i r1) i.r2'
   	SetOutPath "$INSTDIR"

    !insertmacro DebugMessageBox "GetCurrentLocale: $0 ($2)"

    StrCmp $0 "" done
    StrCpy $sPreviousLocale $0
    
done:
    SetOutPath "$INSTDIR"
    !insertmacro DebugMessageBox "SavePreviousVersionConfig:$\ncache: $sPreviousCachePath$\nlocale: $sPreviousLocale"
 !insertmacro DebugMessageBox "GetCurrentCachePath: $sCurrentCachePath"
    Pop $2
    Pop $1
    Pop $0

FunctionEnd

;------------------------------------------------------------------------------
; RelativizeManifestPaths
; Make all manifest files contents relative to the manifest directory
;------------------------------------------------------------------------------
Function RelativizeManifestPaths

    Push $0
    Push $1
    Push $2

    StrCmp $sCurrentCachePath "" done

    StrCpy $0 $sCurrentCachePath     ; Current cache folder to scan
    StrCpy $1 $sPreviousCachePath    ; Original cache location (relativize against this)
    
    !insertmacro DebugMessageBox "RelativizeManifestPaths:$\ncache: $0$\noriginal path: $1"

    SetOutPath "$TEMP"
    System::Call 'installerdll$tempDLL::RelativizeManifestPaths(w r0, w r1) i.r2'
    SetOutPath "$INSTDIR"
    
    !insertmacro DebugMessageBox "RelativizeManifestPaths done: $2"

done:
    Pop $2
    Pop $1
    Pop $0
    
FunctionEnd
;------------------------------------------------------------------------------
; DeleteOldMyPrefsFromCache
; Delete MyPrefs.ead files within cache folders
;------------------------------------------------------------------------------
Function DeleteOldMyPrefsFromCache

    Exch $R0
    Push $0
    Push $1

    StrCmp $R0 "" done
    
    FindFirst $0 $1 "$R0\*.*"

loop:
    StrCmp $1 "" out
    StrCmp $1 "." skip
    StrCmp $1 ".." skip

    IfFileExists "$R0\$1\MyPrefs.ead" 0 skip
    Delete "$R0\$1\MyPrefs.ead"

    ; We can safely delete the dir if it is empty
    RmDir "$R0\$1"

skip:
    FindNext $0 $1
    Goto loop

out:
    FindClose $0

done:
    Pop $1
    Pop $0
    Pop $R0

FunctionEnd

;------------------------------------------------------------------------------
; MoveCacheToTempLocation
; Uninstall previous EADM version. Maintain the cache and locale
;------------------------------------------------------------------------------
Function MoveCacheToTempLocation

    Push $0

    StrCmp $sCurrentCachePath "" done

    StrCpy $0 "0"

loop:
    ; Remove empty dirs just in case
    RMDir "$sCurrentCachePath_tmp$0"
    ClearErrors
    ; Try to move
    Rename $sCurrentCachePath "$sCurrentCachePath_tmp$0"
    IfErrors next
    ; Success
    StrCpy $sCurrentCachePath "$sCurrentCachePath_tmp$0"
    !insertmacro DebugMessageBox "Moved cache to $sCurrentCachePath"
    Goto done
next:
    ; Try next
    IntOp $0 $0 + 1
    ; Retry 10 times at most
    IntCmp $0 10 0 0 error
    Sleep 500
    Goto loop

error:
    !insertmacro DebugMessageBox "Failed to move cache to temp location:$\n$sCurrentCachePath_tmp$0"
    Goto done

done:
    Pop $0

FunctionEnd

;------------------------------------------------------------------------------
; HideCache
; Erase preferences so the cache path will be unknown to the currently installed version
;------------------------------------------------------------------------------
Function HideCache

    Push $0
    Push $1

    StrCmp $sCurrentCachePath "" done
    
    ; Find a previous version
    Call GetEADMInstalledVersionMajor
    Pop $0

    StrCmp $0 "4" EADM4
    StrCmp $0 "5" EADM5
    StrCmp $0 "6" EADM6
    Goto done
    
EADM4:
EADM5:
    ReadRegStr $0 HKLM "SOFTWARE\Electronic Arts\EA Core" "CachePath"
    StrCmp $0 "" +2
    WriteRegStr HKLM "SOFTWARE\Electronic Arts\EA Core" "CachePath" ""

    ReadRegStr $0 HKLM "SOFTWARE\Electronic Arts\EAL" "CachePath"
    StrCmp $0 "" +2
    WriteRegStr HKLM "SOFTWARE\Electronic Arts\EAL" "CachePath" ""

    IfFileExists "$INSTDIR\Prefs.ead" 0 appdata
    ClearErrors
    FileOpen $0 "$INSTDIR\Prefs.ead" w
    IfErrors done
    FileWrite $0 ""
    FileClose $0
    
appdata:
		StrCpy $1 ""
		System::Call "shell32::SHGetFolderPathW(i 0, i ${CSIDL_COMMON_APPDATA}, i 0, i 0, w .r1) i.r0"
    
    StrCmp $1 "" done
    StrCpy $1 "$1\Electronic Arts\eadm_app.prefs"
    IfFileExists "$1" 0 done
    ClearErrors
    FileOpen $0 "$1" w
    IfErrors done
    FileWrite $0 ""
    FileClose $0
    Goto done

EADM6:
    ; Locate the runtime preferences file
    Call GetEADM6RuntimePreferencesFileName
    Pop $1
    
    DeleteINIStr "$1" "EACore" "base_cache_dir"
    DeleteINIStr "$1" "EACoreSession" "full_base_cache_dir"

done:
    !insertmacro DebugMessageBox "HideCache: cache was hidden from previous installation"
    
    Pop $1
    Pop $0

FunctionEnd

;------------------------------------------------------------------------------
; CreateDestinationDirectory
; Create an empty destination directory suitable for moving the current cache
;------------------------------------------------------------------------------
Function CreateDestinationDirectory

    Exch $R0
    Push $0
    
    IfFileExists $R0 0 found
    
    StrCpy $0 "0"

loop:
    IntOp $0 $0 + 1
    IfFileExists "$R0\$0" loop
    StrCpy $R0 "$R0\$0"
    
found:
    ; Got it, create the parent directory
    Push $R0
    Call GetParent
    Pop $0
    IfFileExists $0 +2
    CreateDirectory $0

done:
    Pop $0
    Exch $R0

FunctionEnd


;------------------------------------------------------------------------------
; FixPreviousCachePath
; Fix previous cache path to a name that is compatible with EADM 6.0
; The cache path should end with "EA Core\cache"
;------------------------------------------------------------------------------
Function FixPreviousCachePath

    Push $0
    Push $1
    Push $2
    Push $R0
    
    StrCmp $sCurrentCachePath "" done
    StrCmp $sPreviousCachePath "" done
    
    ; Copy previous cache path to R0
    StrCpy $R0 $sPreviousCachePath
    
    ; Check if it contains "EA Core\cache", which is valid
    Push $R0
    Push "\EA Core\cache"
    Call StrStr
    Pop $0
    StrCmp $0 "" 0 done
    
    ; If it ends with "EADM\cache", change that to "EA Core\cache"
    Push $R0
    Push "\EADM\cache"
    Call StrStr
    Pop $0
    StrCmp $0 "\EADM\cache" 0 +4
    StrCpy $R0 $R0 -11
    StrCpy $R0 "$R0\EA Core\cache"
    Goto fix
    
    ; End with "EA Core" ? Just append cache
    Push $R0
    Push "\EA Core"
    Call StrStr
    Pop $0
    StrCmp $0 "\EA Core" 0 +3
    StrCpy $R0 "$R0\cache"
    Goto fix

    StrCpy $R0 "$R0\EA Core\cache"
    Goto fix

fix:
    !insertmacro DebugMessageBox "Fixed cache path is: $R0"

    ; Check if something changed
    StrCmp $sCurrentCachePath $R0 done

    ; We can only move to a non existant directory or otherwise we will fail
    Push $R0
    Call CreateDestinationDirectory
    Pop $R0

    !insertmacro DebugMessageBox "CreateDestinationDirectory $R0"
    
    ClearErrors
    Rename $sCurrentCachePath $R0
    IfErrors error

    !insertmacro DebugMessageBox "Renamed $sCurrentCachePath -> $R0"

    ; Delete the old cache folder chain,
    ; the move may have erased the last component so try the parent too
    Push $sCurrentCachePath
    Call DeleteEADMFolders
    Push $sCurrentCachePath
    Call GetParent
    Call DeleteEADMFolders

    ; Save new cache path
    StrCpy $sCurrentCachePath $R0
    
    !insertmacro DebugMessageBox "FixPreviousCachePath: $sCurrentCachePath"
    Goto done

error:
   !insertmacro DebugMessageBox "Failed to rename $sCurrentCachePath -> $R0"
   
done:
    Pop $R0
    Pop $2
    Pop $1
    Pop $2
    
FunctionEnd


;------------------------------------------------------------------------------
; DeletePreviousInstallDir
; Manually delete previous installation directory
;------------------------------------------------------------------------------
Function DeletePreviousInstallDir

    Push $0

    StrCmp $sPreviousClientPath "" done
    IfFileExists "$sPreviousClientPath\*.*" 0 done
    
    ; Just to be safe only erase directories containing EADM
    Push $sPreviousClientPath
    Push "EADM"
    Call StrStr
    Pop $0
    StrCmp $0 "" done
    
    ; Finally, erase
	!insertmacro DebugMessageBox "DeletePreviousInstallDir: About to remove $sPreviousClientPath"
    RmDir /r $sPreviousClientPath

done:
    Pop $0
    
FunctionEnd

;------------------------------------------------------------------------------
; UseEADM_7_8_Uninstaller
; Uninstall EADM version 7.x and 8.0.x.x through the their own uninstaller
;------------------------------------------------------------------------------
Function UseEADM_7_8_Uninstaller
    Push $0
    Push $1
    Push $2
    
		; uninstall old EADM

		; first try
    ExecWait '"$INSTDIR\EADMUninstall.exe" /S _?=$INSTDIR'
    IfFileExists "$INSTDIR\EADMUninstall.exe"  fileFound2 fileNotFound2
    fileFound2:
    Delete "$INSTDIR\EADMUninstall.exe"
		fileNotFound2:
    
    ; Get the installation directory
    ReadRegStr $0 HKLM "SOFTWARE\Electronic Arts\EADM" "ClientPath"
   
    ; Get the installation directory (ClientPath points to the executable)
    Push $0
    Call GetParent
    Pop $0
    
		; second try
    ExecWait '"$0\EADMUninstall.exe" /S _?=$0'
    IfFileExists "$0\EADMUninstall.exe" fileFound1 fileNotFound1
	  fileFound1:
    Delete "$0\EADMUninstall.exe"    
    fileNotFound1:

    Pop $2
    Pop $1
    Pop $0

FunctionEnd

;------------------------------------------------------------------------------
; UninstallEADM_7_8_Version
; Uninstall EADM version 7.x and 8.0.x.x - Maintain the cache, settings and locale
;------------------------------------------------------------------------------
Function UninstallEADM_7_8_Version
    Push $0
    Push $1
    Push $2
    Push $3
    Push $4

    ; if an ORIGIN.ini is already present, exit the migration code, because we already have an Origin client installed
		Call GetPreferencesINIFileName
		Pop $1
		StrCmp $1 "" continueMigration
		IfFileExists $1 done

continueMigration:	

    ; Find a previous version of EADM 7,8
    ReadRegStr $0 HKLM "SOFTWARE\Electronic Arts\EADM" "ClientVersion"
    ; Check that the key really exists and has a valid value
    IfErrors done
    StrCmp $0 "" done


    ; find old parent "Electronic Arts" folder - use $3 exclusively!!!
    ReadRegStr $3 HKLM "SOFTWARE\Electronic Arts\EADM" "ClientPath"   
    Push $3
    Call GetParent
    Pop $4	; $4 contains the install folder
    StrCmp $4 "" done	; no folder?
    Push $4
    Call GetParent
    Pop $3	; $3 contains the parent of the install folder
    Push $0
    Push ${version}	; Origin version number
    Call VersionCompare
    Pop $1	; use $1, we need $0 with the version number a second time

    ; $0 = 0  Versions are equal
    ; $0 = 1  Version1 is newer
    ; $0 = 2  Version2 is older

    IntCmp $1 0 done ; last EADM is already a version of Origin
    IntCmp $1 1 done ; do nothing.... maybe a future version...

    Push $0	; second usage of $0 with the version number
    Push "7.0.0.0"	; EADM newer than 6
    Call VersionCompare
    Pop $1
    IntCmp $1 2 done ; EADM is older than EADM 6 - do nothing....
     
	
		; start migrating EADM 7 and 8
		; save settings & download cache   
		; modify settings and eacore.ini
		; todo: check upgrade path from EADM 5 and 6 !!!
        
    SetOutPath "$TEMP"	; req. for the installer dll
    System::Call 'installerdll$tempDLL::MigrateCacheAndSettings() i.r0'
   	SetOutPath "$INSTDIR"
    
    IntCmp $0 1 doNotRemoveOldInstallFolder ; because it is used as the DIP folder (egde case)
    IntCmp $0 2 doNotRemoveOldDataFolder ; because it is used as the DIP folder (egde case)
	
		Call UseEADM_7_8_Uninstaller		   
    RMDir $4
    
		; delete old parent folder, if it is "Electronic Arts" and if it is empty     
    ${GetFileName} $3 $1
    StrCmp $1 "Electronic Arts" deleteParent1
    goto done
deleteParent1:
    RMDir $3 
    
		goto done

doNotRemoveOldInstallFolder:
		
		Call UseEADM_7_8_Uninstaller		
    goto done
    
doNotRemoveOldDataFolder:
		;rename the data folder to prevent deletion
    ; ${CSIDL_COMMON_APPDATA}\Electronic Arts\EADM
		StrCpy $1 ""
		StrCpy $2 ""
		System::Call "shell32::SHGetFolderPathW(i 0, i ${CSIDL_COMMON_APPDATA}, i 0, i 0, w .r1) i.r0"
    StrCmp $1 "" nothingThere
    StrCpy $2 $1   
    StrCpy $1 "$1\Electronic Arts\EADM"   
    StrCpy $2 "$2\Electronic Arts\tmpEADM"   
    IfFileExists "$1\*.*" 0 nothingThere     ; Check that the folder exists
    
    Rename $1 $2
		Call UseEADM_7_8_Uninstaller	; just uninstall and then restore the folder
		;restore it
    Rename $2 $1
		goto deleteFolder

nothingThere:    
		Call UseEADM_7_8_Uninstaller	; just uninstall

deleteFolder:		
    
    ; delete old folder
    RMDir $4

		; delete old parent folder, if it is "Electronic Arts" and if it is empty     
    ${GetFileName} $3 $1   
    StrCmp $1 "Electronic Arts" deleteParent2
    goto done
 deleteParent2:
    RMDir $3

    
done:
    Pop $4
    Pop $3
    Pop $2
    Pop $1
    Pop $0

FunctionEnd

;------------------------------------------------------------------------------
; UninstallPreviousEADMVersion
; Uninstall previous EADM version. Maintain the cache and locale
;------------------------------------------------------------------------------
Function UninstallPreviousEADMVersion

    Push $0
    Push $1
    Push $2

    !insertmacro DebugMessageBox "UninstallPreviousEADMVersion"

    ; Do a little housekeeping, remove a registry key that is left behind
    ; by EADM 5.x uninstaller (32 bits only)
	DeleteRegValue HKLM "SOFTWARE\Electronic Arts\EAL" "CachePath"
	DeleteRegKey /ifempty HKLM "SOFTWARE\Electronic Arts\EAL"
    
    ; remove the AIR UI compponent before doing a check for an existing installed EADM 6, because most
    ; traces would have been removed by a previous uninstall, except the "EA Download Manager UI"
    ; Get the current uninstaller for the AIR UI component
    Push "EA Download Manager UI"
    Call GetUninstallString
    Pop $2
    ; Add silent uninstallation
    StrCpy $2 "$2 /quiet"
    
    ; Finally, execute the uninstaller
	SetOutPath "$INSTDIR"
    ExecWait $2 $0
    !insertmacro DebugMessageBox "Previous version uninstalled: $0"


    ; Find a previous version
    Call GetEADMInstalledVersionMajor
    Pop $0

    StrCmp $0 "" done
    StrCmp $0 "0" done
    
    !insertmacro DebugMessageBox "Detected previous version $0"

    ; Save previous version installation path
    ReadRegStr $sPreviousClientPath HKLM "SOFTWARE\Electronic Arts\EA Core" "ClientPath"

    ; Get the installation directory (ClientPath points to the executable)
    Push $sPreviousClientPath
    Call GetParent
    Pop $sPreviousClientPath
    
    !insertmacro DebugMessageBox "PreviousClientPath: $sPreviousClientPath"

    StrCmp $0 "4" EADM4
    StrCmp $0 "5" EADM5
    StrCmp $0 "6" EADM6
    Goto done
    
EADM4:
EADM5:
EADM6:
    ; Get current config
    Call SavePreviousVersionConfig

    ; Hide the cache so the uninstaller will not erase it
    Call HideCache

    ; Fix all manifests to make them relative
    Call RelativizeManifestPaths

    ; Remove old per-user preferences
    Push $sCurrentCachePath
    Call DeleteOldMyPrefsFromCache
    
    ; Delete the logs
    RmDir /r "$sCurrentCachePath\logs"

    ; Delete the old preferences
    Delete "$sCurrentCachePath\Prefs.ead"
    
    ; Delete the EADM 4.x Anonymous user folder
    RMDir /r "$sCurrentCachePath\{ Anonymous }"
    
douninstall:
		; try english application name first
    ; Get the current uninstaller
    Push "EA Download Manager"
    Call GetUninstallString
    Pop $2
    
    ; Add silent uninstallation
    StrCpy $2 "$2 /S"
    
    Push $2
    Push "IDriver.exe"
    Call StrStr
    Pop $1
    StrCmp $1 "" +2 0

    ; Add /uninst to Install Shield uninstaller
    StrCpy $2 "$2 /uninst"
    !insertmacro DebugMessageBox "About to uninstall previous version:$\n$2"
    
    ; Finally, execute the uninstaller
    ExecWait $2 $0
    !insertmacro DebugMessageBox "Previous version uninstalled: $0"       



    ; try localized application name - seems to be req. for 2052,1028,1038,1041,1042    
    ; Get the current uninstaller
    Push "$(EADM_APPLICATION_NAME)"
    Call GetUninstallString
    Pop $2
    
    ; Add silent uninstallation
    StrCpy $2 "$2 /S"
    
    Push $2
    Push "IDriver.exe"
    Call StrStr
    Pop $1
    StrCmp $1 "" +2 0

    ; Add /uninst to Install Shield uninstaller
    StrCpy $2 "$2 /uninst"
    !insertmacro DebugMessageBox "About to uninstall previous version:$\n$2"       
    
    ; Finally, execute the uninstaller
    ExecWait $2 $0
    !insertmacro DebugMessageBox "Previous version uninstalled: $0"       
        
    ; It seems that EADM 4.x uninstaller misses some files probably due
    ; to the /uninst flag not executing certain scripts portions
    ; We manually delete the installation directory in that case
    Call DeletePreviousInstallDir

    ; Erase orphan EADM6 Start Menu entries
    SetShellVarContext all
    Push "$SMPROGRAMS"
    Call RemoveOrphanSMEntries

    ; Erase EADM4 Start Menu entry entry (just in case)
    IfFileExists "$SMPROGRAMS\Electronic Arts\EA Download Manager.lnk" 0 +3
    Delete "$SMPROGRAMS\Electronic Arts\EA Download Manager.lnk"
    RMDir "$SMPROGRAMS\Electronic Arts"
    SetShellVarContext current
    
done:
    Pop $2
    Pop $1
    Pop $0

FunctionEnd

;------------------------------------------------------------------------------
; RemoveOrphanSMEntries
; Remove orpan start menu entries (probably due to an EADM 5 downgrade)
;------------------------------------------------------------------------------
Function RemoveOrphanSMEntries

    Exch $R0
    Push $0
    Push $1

    FindFirst $0 $1 $R0\*.*

    loop:
        StrCmp $1 "" out
        StrCmp $1 "." skip
        StrCmp $1 ".." skip

        IfFileExists "$R0\$1\*.*" isdir test

    isdir:
        ; Recurse
        Push "$R0\$1"
        Call RemoveOrphanSMEntries

    test:
        IfFileExists "$R0\$1\$(EADM_MENU_UNINSTALL).lnk" 0 skip
        IfFileExists "$R0\$1\$(EADM_MENU_HELP).url" 0 skip
        IfFileExists "$R0\$1\$(EBISU_APPLICATION_NAME).lnk" 0 skip
        IfFileExists "$RO\$1\$(EADM_MENU_OER).lnk" 0 skip
        
        ; Found one, remove it!
        Delete "$R0\$1\$(EADM_MENU_UNINSTALL).lnk"
        Delete "$R0\$1\$(EADM_MENU_HELP).url"
        Delete "$R0\$1\$(EBISU_APPLICATION_NAME).lnk"
        Delete "$RO\$1\$(EADM_MENU_OER).lnk"
        Push "$R0\$1"
        Call DeleteEADMFolders

    skip:
        FindNext $0 $1
        Goto loop

    out:
        FindClose $0

done:
    Pop $1
    Pop $0
    Pop $R0
    
FunctionEnd

;------------------------------------------------------------------------------
; StrTok
; String tokenizer
;------------------------------------------------------------------------------
Function StrTok
  Exch $R1
  Exch 1
  Exch $R0
  Push $R2
  Push $R3
  Push $R4
  Push $R5
 
  ;R0 fullstring
  ;R1 tokens
  ;R2 len of fullstring
  ;R3 len of tokens
  ;R4 char from string
  ;R5 testchar
 
  StrLen $R2 $R0
  IntOp $R2 $R2 + 1
 
  loop1:
    IntOp $R2 $R2 - 1
    IntCmp $R2 0 exit
 
    StrCpy $R4 $R0 1 -$R2
 
    StrLen $R3 $R1
    IntOp $R3 $R3 + 1
 
    loop2:
      IntOp $R3 $R3 - 1
      IntCmp $R3 0 loop1
 
      StrCpy $R5 $R1 1 -$R3
 
      StrCmp $R4 $R5 Found
    Goto loop2
  Goto loop1
 
  exit:
  ;Not found!!!
  StrCpy $R1 ""
  StrCpy $R0 ""
  Goto Cleanup
 
  Found:
  StrLen $R3 $R0
  IntOp $R3 $R3 - $R2
  StrCpy $R1 $R0 $R3
 
  IntOp $R2 $R2 - 1
  IntOp $R3 $R3 + 1
  StrCpy $R0 $R0 $R2 $R3
 
  Cleanup:
  Pop $R5
  Pop $R4
  Pop $R3
  Pop $R2
  Exch $R0
  Exch 1
  Exch $R1
 
FunctionEnd

;--------------------------------
; Parse the command line for backward compat
;--------------------------------
Function .onInit
!ifdef INNER
 
    ; If INNER is defined, then we aren't supposed to do anything except write out
    ; the uninstaller.  This is better than processing a command line option as it means
    ; this entire code path is not present in the final (real) installer.
    WriteUninstaller "$%TEMP%\uninstaller.exe"
    Quit  ; just bail out quickly when running the "inner" installer
!endif

	SectionSetSize 0 ${addsize}

	;initialize whether there's an instance running or not
	StrCpy $alreadyRunning ""

		; generate random part for the dll name	
		StrCpy $0 ""
		System::Call 'kernel32::GetTickCount()i .r0'
		StrCpy $tempDLL $0
		
    SetShellVarContext all
		
    ; Extract our plugin dll
    File /a "/oname=$TEMP\installerdll$tempDLL.dll" "${INSTALLER_DLLDIR}\installerdll.dll"

    ${GetParameters} $0
		; /S (handled by NSIS)
		IfSilent endSilent
    
    ; /s option (uppercase is handled by NSIS)
    ClearErrors
    ${GetOptions} $0 "/s" $1
    IfErrors endSilent 0
    SetSilent silent

endSilent:
    ; /dir option
    ClearErrors
    ${GetOptions} $0 "/dir" $1
    IfErrors endDir
    StrCmp $1 "" endDir

    Push $1
		Call CheckValidDirectory
 		Pop $0  ; (1 if valid, 0 if invalid)
    IntCmp $0 1 validFolder
    MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONSTOP $(EADM_INSTALL_PATH_INVALID)
    Abort
    
validFolder:
    
    Push $1
    Call fixInstallationDir
    Pop $1
    StrCpy $INSTDIR $1

    ; Remove trailing backslash
    StrCpy $0 $INSTDIR "" -1
    StrCmp $0 "\" 0 +2
    StrCpy $INSTDIR $INSTDIR -1

endDir:
	; is our !define defined?
    !ifndef MUI_LANGDLL_LANGUAGES
        !error "MUI_LANGDLL_LANGUAGES not defined (should be in Localization.nsh)"
    !endif
    ; MUI_LANGDLL_LANGUAGES has all the supported languages (through the use of MUI_LANGUAGE) 
    ; The format is as follows:
    ; 'Italiano' '1040' 'Deutsch' '1031' 'Français' '1036' 'Nederlands' '1043' 'Dansk' '1030' 'English' '1033'
	
	; Check if a particular language was indicated
		${GetParameters} $0
    ClearErrors
    ${GetOptions} $0 "/locale " $1
    IfErrors endLangSelection 0
		Push $1
		Call GetLCIDFromLanguageCode
		Pop $1
		
		StrCmp $1 "" endLangSelection
		StrCpy $LANGUAGE $1

endLangSelection:

	Call LocaleLanguageSelection
    Pop $LANGUAGE
	
	; the NSIS package will use this to localize buttons for message boxes, if the UI language is installed!!!
	StrCpy $R0 $LANGUAGE
	System::Call 'Kernel32::SetEnvironmentVariableW(w "ORIGIN_INSTALLER_LANGUAGE", w R0).r2'

	!insertmacro DebugMessageBox "STEP 3: JRIVERO: LANGUAGE:  $LANGUAGE"
    
	;*****	This has to be the first "real" step! ****
    ; Check the user has admin privileges
    Call IsUserAdmin
    Pop $1
    IntCmp $1 1 endCheckAdmin
    
    MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONSTOP $(EADM_INSTALL_ADMIN_REQUIRED)
    Abort

endCheckAdmin:

	; Check there we are the only running instance    
    Call CheckSingleInstance
    Pop $0
    IntCmp $0 "1" +2
    Abort

		; See if the parent process is belonging to EADM, then we run as an update and won't show some warnings 
		StrCpy $1 "0"	; no update
    
  	${GetParameters} $0
		ClearErrors
  	StrCpy $1 "0" ; clear update variable
    ${GetOptions} $0 "/update" $2
    IfErrors noUpdate 0    
		StrCpy $1 "1" ; update	
noUpdate:
FunctionEnd


; -------------------------------------------------------------------------------
; GetInstallerAssets: exctracts the welcome / finish BMP files
; -------------------------------------------------------------------------------
Function GetInstallerAssets

	${Switch} $LANGUAGE
	${Case} ${LANG_ENGLISH}
	File /oname=$TEMP\welcome.bmp "resources\English-installscreen.bmp"
	${Break}
	${Case} ${LANG_ENGLISHBR}
	File /oname=$TEMP\welcome.bmp "resources\English-installscreen.bmp"
	${Break}
	${Case} ${LANG_CZECH}	
	File /oname=$TEMP\welcome.bmp "resources\Czech-installscreen.bmp"
	${Break}
	${Case} ${LANG_DANISH}
	File /oname=$TEMP\welcome.bmp "resources\Danish-installscreen.bmp"
	${Break}
	${Case} ${LANG_DUTCH}	
	File /oname=$TEMP\welcome.bmp "resources\Dutch-installscreen.bmp"
	${Break}
	${Case} ${LANG_FRENCH}
	File /oname=$TEMP\welcome.bmp "resources\French-installscreen.bmp"
	${Break}
	${Case} ${LANG_GERMAN}
	File /oname=$TEMP\welcome.bmp "resources\German-installscreen.bmp"
	${Break}
	${Case} ${LANG_GREEK}	
	File /oname=$TEMP\welcome.bmp "resources\Greek-installscreen.bmp"
	${Break}
	${Case} ${LANG_FINNISH}
	File /oname=$TEMP\welcome.bmp "resources\Finnish-installscreen.bmp"
	${Break}
	${Case} ${LANG_HUNGARIAN}
	File /oname=$TEMP\welcome.bmp "resources\Hungarian-installscreen.bmp"
	${Break}
	${Case} ${LANG_ITALIAN}
	File /oname=$TEMP\welcome.bmp "resources\Italian-installscreen.bmp"
	${Break}
	${Case} ${LANG_JAPANESE}
	File /oname=$TEMP\welcome.bmp "resources\Japanese-installscreen.bmp"
	${Break}
	${Case} ${LANG_KOREAN}
	File /oname=$TEMP\welcome.bmp "resources\Korean-installscreen.bmp"
	${Break}
	${Case} ${LANG_NORWEGIAN}
	File /oname=$TEMP\welcome.bmp "resources\Norwegian-installscreen.bmp"
	${Break}
	${Case} ${LANG_POLISH}
	File /oname=$TEMP\welcome.bmp "resources\Polish-installscreen.bmp"
	${Break}
	${Case} ${LANG_PORTUGUESEBR}
	File /oname=$TEMP\welcome.bmp "resources\Brazilian-Port-installscreen.bmp"
	${Break}
	${Case} ${LANG_PORTUGUESE}
	File /oname=$TEMP\welcome.bmp "resources\Portuguese-installscreen.bmp"
	${Break}
	${Case} ${LANG_RUSSIAN}
	File /oname=$TEMP\welcome.bmp "resources\Russian-installscreen.bmp"
	${Break}
	${Case} ${LANG_SIMPCHINESE}
	File /oname=$TEMP\welcome.bmp "resources\Chinese-S-installscreen.bmp"
	${Break}
	${Case} ${LANG_SWEDISH}
	File /oname=$TEMP\welcome.bmp "resources\Swedish-installscreen.bmp"
	${Break}
	${Case} ${LANG_SPANISH}
	File /oname=$TEMP\welcome.bmp "resources\Spanish-installscreen.bmp"
	${Break}
	${Case} ${LANG_SPANISHINTERNATIONAL}
	File /oname=$TEMP\welcome.bmp "resources\MexSpa-installscreen.bmp"
	${Break}
	${Case} ${LANG_THAI}
	File /oname=$TEMP\welcome.bmp "resources\Thai-installscreen.bmp"
	${Break}
	${Case} ${LANG_TRADCHINESE}
	File /oname=$TEMP\welcome.bmp "resources\Chinese-T-installscreen.bmp"
	${Break}
	${Default}
	File /oname=$TEMP\welcome.bmp "resources\English-installscreen.bmp"
	${EndSwitch}

FunctionEnd

; -------------------------------------------------------------------------------
; onPreDirPage: This callback will be called before the directory page is shown
;               If this is an update or reinstall, the installation directory remains unchanged
;               so we skip this page
; -------------------------------------------------------------------------------
Var ShowDirPage

Function onPreDirPage

    Push $0
    Push $R0

    Call GetEbisuInstalledVersionMajor
    Pop $R0

	; set 'ShowDirPage' to be used later
	StrCpy $ShowDirPage "false"
    IntCmp $R0 0 0 writeShowDirPage writeShowDirPage
	StrCpy $ShowDirPage "true"
	
writeShowDirPage:
	; communicate whether the user options page was shown
	WriteRegStr HKLM "SOFTWARE\${productname}" "ShowDirPage" $ShowDirPage
	
begin:
    !insertmacro DebugMessageBox "onPreDirPage Ebisu version is $R0"
    IntCmp $R0 0 done

    ;
    ; Get the installation directory
    ;
    ReadRegStr $0 HKLM "SOFTWARE\${productname}" "ClientPath"
    StrCmp $0 "" done

    ; Get the installation directory (ClientPath points to the executable)
    Push $0
    Call GetParent
    Pop $0
    IfFileExists $0\*.* 0 skip
    
    StrCpy $INSTDIR $0
    
    
skip:
    ;
    ; A version is already installed. Skip directory selection
    ;
    Abort

done:
    Pop $R0
    Pop $0

FunctionEnd

!define SHCNE_ASSOCCHANGED 0x08000000
!define SHCNF_IDLIST 0
 
Function RefreshShellIcons
  System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v \
  (${SHCNE_ASSOCCHANGED}, ${SHCNF_IDLIST}, 0, 0)'
FunctionEnd


; -------------------------------------------------------------------------------
; onShowDirPage: This callback will be called when the directory page is shown
;                If Ebisu is already installed (Update or Repair) then the user
;                does not get to choose the destination directory
; -------------------------------------------------------------------------------
!define IDC_DESKTOPCHECK      1500
!define IDC_STARTCHECK	      1501
!define IDC_AUTORUNCHECK      1502
!define IDC_CRASHCHECK	      1503
!define IDC_AUTOUPDATE	      1504
!define IDC_TELEMOO			  1505

Var DesktopCheck
Var DesktopCheckResult
Var StartCheck
Var StartCheckResult
Var AutostartCheck
Var AutostartCheckResult
Var AutoupdateCheck
Var AutoupdateCheckResult
Var TelemOOCheck
Var TelemOOCheckResult
Var temporaryControlID
Var isOSVista
Var DPIsValue
Var isRequireUICorrection
Var UICorrectionOffset
Var UICheckBoxOffset
Var UICheckBoxOffsetInitial
Var UILinkOffset
Var myResult
Var myResult2
Var syslink
Var SysLinkSubProc
Var ConfirmReadLabel
Var UserIsInCanada
Var LinkFont
Var LinkTemplate
Var LinkCtlType
Function onShowDirPage

	StrCpy $isRequireUICorrection "0"
	StrCpy $UICorrectionOffset "173"
	StrCpy $UICheckBoxOffset "20"
	StrCpy $UICheckBoxOffsetInitial "0"
    StrCpy $UILinkOffset "3"
    
    Call IsWindowsVista
    Pop $isOSVista
	IntCmp $isOSVista 1 isOSVista notOSVista notOSVista
	
isOSVista:	
	Call GetDPIs
	Pop $DPIsValue

	;                     equal      less       more
	IntCmp $DPIsValue 96  notOSVista notOSVista correctUIForVistaDPIs 
	
correctUIForVistaDPIs:	
	StrCpy $isRequireUICorrection "1"
	; make $UICorrectionOffset = $UICorrectionOffset + (($DPIsValue - 96) * 1.5)
	; Example with 144
    IntOp $myResult $DPIsValue - 96 ; 144 - 96 = 48
	IntOp $myResult2 $myResult / 2 ; 48 / 2 = 24
	IntOp $myResult $myResult2 + $myResult ; 24 + 48 = 72
	StrCpy $UICheckBoxOffsetInitial $myResult
	IntOp $UICorrectionOffset $UICorrectionOffset + $myResult
	!insertmacro DebugMessageBox "DPIs: $DPIsValue, correction: [$UICorrectionOffset] UICheckBoxOffsetInitial [$UICheckBoxOffsetInitial]"	

notOSVista:
	
    Push $R0
    Push $R1
	
    Call GetEbisuInstalledVersionMajor
    Pop $R0

   !insertmacro DebugMessageBox "onShowDirPage Ebisu version is $R0"
    IntCmp $R0 0 adjustWindow
	
disable:
    ;
    ; A version is already installed. Disable directory selection
    ;
    FindWindow $R0 "#32770" "" $HWNDPARENT
    GetDlgItem $R1 $R0 1019 ; Line edit
    EnableWindow $R1 0
    GetDlgItem $R1 $R0 1001 ; Browse button
    EnableWindow $R1 0
	Goto done
	
adjustWindow:
	
	; Hide the static text at the top
	GetDlgItem $temporaryControlID $mui.DirectoryPage 1006
	System::Call 'user32::ShowWindow(i $temporaryControlID, i ${SW_HIDE})'
	
	; reposition the Space Required label
	GetDlgItem $temporaryControlID $mui.DirectoryPage 1023
    ; Get the size of the window.
    System::Call "*${stRECT} .R0"

    ; Get the window rect
    System::Call "user32::GetWindowRect(i $temporaryControlID, i R0)"
    ; convert position into window space
		System::Call 'user32::MapWindowPoints(i 0, i $HWNDPARENT ,i R0, i 2)'
    System::Call "*$R0${stRECT} (.R2, .R3, .R4, .R5)"

    ; move widget
    IntOp $R3 $R3 - $UICorrectionOffset
    
    System::Call 'kernel32::SetLastError(i 0)'
    System::Call 'user32::SetWindowPos(i $temporaryControlID, i 0, i 0, i R3, i 0, i 0, i ${SWP_NOSIZE}|${SWP_NOZORDER}) i.R6'
	
	System::Free $R0
	
	; reposition the Space Available label
	GetDlgItem $temporaryControlID $mui.DirectoryPage 1024
    ; Get the size of the window.
    System::Call "*${stRECT} .R0"

    ; Get the window rect
    System::Call "user32::GetWindowRect(i $temporaryControlID, i R0)"
    ; convert position into window space
		System::Call 'user32::MapWindowPoints(i 0, i $HWNDPARENT ,i R0, i 2)'
    System::Call "*$R0${stRECT} (.R7, .R3, .R4, .R5)"

    ; move widget
    IntOp $R3 $R3 - $UICorrectionOffset
	
	System::Call 'kernel32::SetLastError(i 0)'
    System::Call 'user32::SetWindowPos(i $temporaryControlID, i 0, i 0, i R3, i 0, i 0, i ${SWP_NOSIZE}|${SWP_NOZORDER}) i.R6'
	
	System::Free $R0
	
	; reposition the Directory
	GetDlgItem $temporaryControlID $mui.DirectoryPage 1019
    ; Get the size of the window.
    System::Call "*${stRECT} .R0"

    ; Get the window rect
    System::Call "user32::GetWindowRect(i $temporaryControlID, i R0)"
    ; convert position into window space
		System::Call 'user32::MapWindowPoints(i 0, i $HWNDPARENT ,i R0, i 2)'
    System::Call "*$R0${stRECT} (.R2, .R3, .R4, .R5)"

    ; move widget
    IntOp $R3 $R3 - $UICorrectionOffset
	IntOp $R2 $R2 - $R7
	
	System::Call 'kernel32::SetLastError(i 0)'
    System::Call 'user32::SetWindowPos(i $temporaryControlID, i 0, i R2, i R3, i 0, i 0, i ${SWP_NOSIZE}|${SWP_NOZORDER}) i.R6'
	System::Free $R0
	
	; reposition the Browse button
	GetDlgItem $temporaryControlID $mui.DirectoryPage 1001
    ; Get the size of the window.
    System::Call "*${stRECT} .R0"

    ; Get the window rect
    System::Call "user32::GetWindowRect(i $temporaryControlID, i R0)"
    ; convert position into window space
		System::Call 'user32::MapWindowPoints(i 0, i $HWNDPARENT ,i R0, i 2)'
    System::Call "*$R0${stRECT} (.R2, .R3, .R4, .R5)"

    ; move widget
    IntOp $R3 $R3 - $UICorrectionOffset
	IntOp $R2 $R2 - $R7
    
    System::Call 'kernel32::SetLastError(i 0)'
    System::Call 'user32::SetWindowPos(i $temporaryControlID, i 0, i R2, i R3, i 0, i 0, i ${SWP_NOSIZE}|${SWP_NOZORDER}) i.R6'
	System::Free $R0
	
	; reposition the Directory box
	GetDlgItem $temporaryControlID $mui.DirectoryPage 1020
    ; Get the size of the window.
    System::Call "*${stRECT} .R0"

    ; Get the window rect
    System::Call "user32::GetWindowRect(i $temporaryControlID, i R0)"
    ; convert position into window space
		System::Call 'user32::MapWindowPoints(i 0, i $HWNDPARENT ,i R0, i 2)'
    System::Call "*$R0${stRECT} (.R2, .R3, .R4, .R5)"

    ; move widget
    IntOp $R3 $R3 - $UICorrectionOffset
	IntOp $R2 $R2 - $R7
    
    System::Call 'kernel32::SetLastError(i 0)'
    System::Call 'user32::SetWindowPos(i $temporaryControlID, i 0, i R2, i R3, i 0, i 0, i ${SWP_NOSIZE}|${SWP_NOZORDER}) i.R6'

	System::Free $R0
	
	; Need to adjust window size for Japanese, Korean, and both Chinese
	IntOp $R4 140 - 0
	StrCmp $LANGUAGE ${LANG_JAPANESE} 0 Korean
	IntOp $R4 120 - 0
	Goto addCheckboxes
Korean:
	StrCmp $LANGUAGE ${LANG_KOREAN} 0 Traditional
	IntOp $R4 120 - 0
	Goto addCheckboxes
Traditional:
	StrCmp $LANGUAGE ${LANG_TRADCHINESE} 0 Simplified
	IntOp $R4 120 - 0
	Goto addCheckboxes
Simplified:
	StrCmp $LANGUAGE ${LANG_SIMPCHINESE} 0 addCheckboxes
	IntOp $R4 120 - 0
	
addCheckboxes:

	IntCmp $isRequireUICorrection 0 continueCheckBoxes
	IntOp $R4 $R4 + $UICheckBoxOffsetInitial
	
continueCheckBoxes:

	; Determine if user is in Canada for CASL
	System::Call 'installerdll$tempDLL::IsUserInCanada() i.s'
	Pop $UserIsInCanada
	
	;Add Create Desktop Shortcut checkbox
	StrCpy $0 $(installer_desktop_shortcut)
	System::Call 'user32::CreateWindowEx(i 0, t "BUTTON", t r0, i ${BS_AUTOCHECKBOX}|${WS_CHILD}|${WS_VISIBLE}|${WS_TABSTOP}, i 0, i R4, i 500, i 20, i $mui.DirectoryPage, i IDC_DESKTOPCHECK, i 0, i 0) i.s'
	Pop $DesktopCheck
	System::Call 'user32::SetWindowPos(i $DesktopCheck, i ${HWND_TOP}, i 0, i 0, i 0, i 0, i ${SWP_NOSIZE}|${SWP_NOMOVE})'
	SendMessage $mui.DirectoryPage ${WM_GETFONT} 0 0 $0
	SendMessage $DesktopCheck ${WM_SETFONT} $0 1
	SendMessage $DesktopCheck ${BM_SETCHECK} 1 0
	
	;Add Create Start Menu Shortcut checkbox
	IntOp $R4 $R4 + $UICheckBoxOffset
	StrCpy $0 $(installer_start_menu_shortcut)
	System::Call 'user32::CreateWindowEx(i 0, t "BUTTON", t r0, i ${BS_AUTOCHECKBOX}|${WS_CHILD}|${WS_VISIBLE}|${WS_TABSTOP}, i 0, i R4, i 500, i 20, i $mui.DirectoryPage, i IDC_STARTCHECK, i 0, i 0) i.s'
	Pop $StartCheck
	System::Call 'user32::SetWindowPos(i $StartCheck, i ${HWND_TOP}, i 0, i 0, i 0, i 0, i ${SWP_NOSIZE}|${SWP_NOMOVE})'
	SendMessage $mui.DirectoryPage ${WM_GETFONT} 0 0 $0
	SendMessage $StartCheck ${WM_SETFONT} $0 1
	SendMessage $StartCheck ${BM_SETCHECK} 1 0
	
	;Add Autorun checkbox
	IntOp $R4 $R4 + $UICheckBoxOffset
	StrCpy $0 $(installer_autorun_windows_start)
	System::Call 'user32::CreateWindowEx(i 0, t "BUTTON", t r0, i ${BS_AUTOCHECKBOX}|${WS_CHILD}|${WS_VISIBLE}|${WS_TABSTOP}, i 0, i R4, i 500, i 20, i $mui.DirectoryPage, i IDC_AUTORUNCHECK, i 0, i 0) i.s'
	Pop $AutostartCheck
	System::Call 'user32::SetWindowPos(i $AutostartCheck, i ${HWND_TOP}, i 0, i 0, i 0, i 0, i ${SWP_NOSIZE}|${SWP_NOMOVE})'
	SendMessage $mui.DirectoryPage ${WM_GETFONT} 0 0 $0
	SendMessage $AutostartCheck ${WM_SETFONT} $0 1
	SendMessage $AutostartCheck ${BM_SETCHECK} 1 0
	
	;Add Autoupdate checkbox	
	IntOp $R4 $R4 + $UICheckBoxOffset
	StrCpy $0 $(origin_bootstrap_auto_update_client_and_games)
	Push $0
	Push "%1"
	Push "Origin"
	Call StrReplace
	Pop $0
	System::Call 'user32::CreateWindowEx(i 0, t "BUTTON", t r0, i ${BS_AUTOCHECKBOX}|${WS_CHILD}|${WS_VISIBLE}|${WS_TABSTOP}, i 0, i R4, i 345, i 20, i $mui.DirectoryPage, i IDC_AUTOUPDATE, i 0, i 0) i.s'
	Pop $AutoupdateCheck
	System::Call 'user32::SetWindowPos(i $AutoupdateCheck, i ${HWND_TOP}, i 0, i 0, i 0, i 0, i ${SWP_NOSIZE}|${SWP_NOMOVE})'
	SendMessage $mui.DirectoryPage ${WM_GETFONT} 0 0 $0
	SendMessage $AutoupdateCheck ${WM_SETFONT} $0 1
	
	; Don't auto-check for Canadian users
	IntCmp $UserIsInCanada 1 skipCheckboxForCanada
	
	SendMessage $AutoupdateCheck ${BM_SETCHECK} 1 0
skipCheckboxForCanada:

	; Use lookalike links for XP
	${If} ${IsWinXP}
		; Make a crappy XP 'link font'
		;MessageBox MB_OK "Test IS XP"
		CreateFont $LinkFont "$(^Font)" "$(^FontSize)" "400" /UNDERLINE
		StrCpy $LinkTemplate "%1"
		StrCpy $LinkCtlType "static"
	${Else}
		;MessageBox MB_OK "Test >XP"
		StrCpy $LinkTemplate "<A>%1</A>"
		StrCpy $LinkCtlType "SysLink"
	${EndIf}
	StrCpy $1 $LinkCtlType
	
    IntOp $R5 $R4 + $UILinkOffset
	StrCpy $0 "$(ebisu_client_explain_this_link)"
	Push $LinkTemplate
	Push "%1"
	Push $0
	Call StrReplace
	Pop $0
    System::Call 'user32::CreateWindowEx(i 0, t r1, t r0, i ${WS_CHILD}|${WS_VISIBLE}|${WS_TABSTOP}, i 350, i R5, i 100, i 20, i $mui.DirectoryPage, i 0, i 0, i 0) i.s'
	Pop $syslink
    System::Call 'user32::SetWindowPos(i $syslink, i ${HWND_TOP}, i 0, i 0, i 0, i 0, i ${SWP_NOSIZE}|${SWP_NOMOVE})'	
	${If} ${IsWinXP}
		SendMessage $syslink ${WM_SETFONT} $LinkFont 0
		SetCtlColors $syslink 0x0000FF "transparent"
	${Else}
		SendMessage $mui.DirectoryPage ${WM_GETFONT} 0 0 $0
		SendMessage $syslink ${WM_SETFONT} $0 1
	${EndIf}
    ${LinkButton_SetCallback} $syslink AutoUpdateLinkProc $SysLinkSubProc $SysLinkSubProc
    
	;Add Telemetry opt out checkbox
	IntOp $R4 $R4 + $UICheckBoxOffset
	StrCpy $0 $(ebisu_client_help_improve_checkbox)
	Push $0
	Push "%1"
	Push "Origin"
	Call StrReplace
	Pop $0
	System::Call 'user32::CreateWindowEx(i 0, t "BUTTON", t r0, i ${BS_AUTOCHECKBOX}|${WS_CHILD}|${WS_VISIBLE}|${WS_TABSTOP}, i 0, i R4, i 345, i 20, i $mui.DirectoryPage, i IDC_TELEMOO, i 0, i 0) i.s'
	Pop $TelemOOCheck
	System::Call 'user32::SetWindowPos(i $TelemOOCheck, i ${HWND_TOP}, i 0, i 0, i 0, i 0, i ${SWP_NOSIZE}|${SWP_NOMOVE})'	
	SendMessage $mui.DirectoryPage ${WM_GETFONT} 0 0 $0
	SendMessage $TelemOOCheck ${WM_SETFONT} $0 1
	SendMessage $TelemOOCheck ${BM_SETCHECK} 1 0
    
    IntOp $R5 $R4 + $UILinkOffset
	StrCpy $0 "$(ebisu_client_explain_this_link)"
	Push $LinkTemplate
	Push "%1"
	Push $0
	Call StrReplace
	Pop $0
    System::Call 'user32::CreateWindowEx(i 0, t r1, t r0, i ${WS_CHILD}|${WS_VISIBLE}|${WS_TABSTOP}, i 350, i R5, i 100, i 20, i $mui.DirectoryPage, i 0, i 0, i 0) i.s'
	Pop $syslink
    System::Call 'user32::SetWindowPos(i $syslink, i ${HWND_TOP}, i 0, i 0, i 0, i 0, i ${SWP_NOSIZE}|${SWP_NOMOVE})'	
	${If} ${IsWinXP}
		SendMessage $syslink ${WM_SETFONT} $LinkFont 0
		SetCtlColors $syslink 0x0000FF "transparent"
	${Else}
		SendMessage $mui.DirectoryPage ${WM_GETFONT} 0 0 $0
		SendMessage $syslink ${WM_SETFONT} $0 1
	${EndIf}
    ${LinkButton_SetCallback} $syslink TelemOOLinkProc $SysLinkSubProc $SysLinkSubProc

    
    ;ConfirmReadLabel
    IntOp $R5 $R5 + 20
	StrCpy $0 $(ebisu_client_confirm_user_read_explanations)
    System::Call 'user32::CreateWindowEx(i 0, t "static", t r0, i ${WS_CHILD}|${WS_VISIBLE}|${SS_LEFT}, i 15, i 355, i 300, i 30, i $HWNDPARENT, i 0, i 0, i 0) i.s'
	Pop $ConfirmReadLabel
    SendMessage $mui.DirectoryPage ${WM_GETFONT} 0 0 $0
	SendMessage $ConfirmReadLabel ${WM_SETFONT} $0 1
	
	;Since we're skipping our other windows, we don't get focus
	BringToFront
	
done:
    Pop $R1
    Pop $R0

FunctionEnd

!macro UpdateDirDialog
    FindWindow $R0 "#32770" "" $HWNDPARENT
    GetDlgItem $R1 $R0 1019 ; Line edit
    SendMessage $R1 ${WM_SETTEXT} 0 "STR:$INSTDIR"
!macroend

Var AutoUpdateWindow
Var READTEXT
Var CLOSEBTN
Var TEXTFONT
Var AUTOUPDATEMSG
Function AutoUpdateLinkProc
    ${NSW_CreateWindow} $AutoUpdateWindow "Origin Setup" 1018
    
	StrCpy $0 $(ebisu_client_automatic_update_legal_explanation)
    StrCpy $AUTOUPDATEMSG $0
    
    System::Call 'user32::CreateWindowEx(i 0, t "EDIT", t "Test", i ${DEFAULT_STYLES}|${WS_BORDER}|${WS_CHILD}|${WS_VISIBLE}|${WS_TABSTOP}|${WS_VSCROLL}|${ES_MULTILINE}|${ES_READONLY}, i 10, i 10, i 320, i 170, i $AutoUpdateWindow, i 0, i 0, i 0) i.s'
	Pop $READTEXT
    System::Call 'user32::SetWindowPos(i $READTEXT, i ${HWND_TOP}, i 0, i 0, i 0, i 0, i ${SWP_NOSIZE}|${SWP_NOMOVE})'	
    SetCtlColors $READTEXT "" 0xffffff
    CreateFont $TEXTFONT "$(^Font)" "$(^FontSize)" "400"
    SendMessage $READTEXT ${WM_SETFONT} $TEXTFONT 0
    SendMessage $READTEXT ${WM_SETTEXT} 0 "STR:$AUTOUPDATEMSG"
    
    ${NSW_CreateButton} 170u 115u 50u 15u "Close"
    Pop $CLOSEBTN
    ${NSW_OnClick} $CLOSEBTN AutoUpdateCloseBtnAction
    
    System::Call 'user32::SetWindowPos(i $AutoUpdateWindow, i ${HWND_TOP}, i 0, i 0, i 350, i 250, i ${SWP_NOMOVE})'	
    
    ${NSW_Show}
FunctionEnd

Var TelemOOWindow
Var MSGLABEL
Function TelemOOLinkProc
    ${NSW_CreateWindow} $TelemOOWindow "Origin Setup" 1018
    
	StrCpy $0 $(ebisu_client_experience_reporting_text)
	
    ${NSW_CreateLabel} 15u 15u 200u 70u $0
    Pop $MSGLABEL
    
    ${NSW_CreateButton} 170u 85u 50u 15u "Close"
    Pop $CLOSEBTN
    ${NSW_OnClick} $CLOSEBTN TelemOOCloseBtnAction
    
    System::Call 'user32::SetWindowPos(i $TelemOOWindow, i ${HWND_TOP}, i 0, i 0, i 350, i 200, i ${SWP_NOMOVE})'
    
    ${NSW_Show}
FunctionEnd

Function AutoUpdateCloseBtnAction
    SendMessage $AutoUpdateWindow ${WM_CLOSE} 0 0
FunctionEnd

Function TelemOOCloseBtnAction
    SendMessage $TelemOOWindow ${WM_CLOSE} 0 0
FunctionEnd

Var WINRESULT
; -------------------------------------------------------------------------------
; onLeaveDirPage: This callback will be called when the directory page is left
; -------------------------------------------------------------------------------
Function onLeaveDirPage
    ; Remove the extra label we added
    System::Call 'user32::DestroyWindow(i $ConfirmReadLabel) i.s'
    Pop $WINRESULT
	
	; Close the two extra windows
	${If} $AutoUpdateWindow != 0
		SendMessage $AutoUpdateWindow ${WM_CLOSE} 0 0
	${EndIf}
	${If} $TelemOOWindow != 0
		SendMessage $TelemOOWindow ${WM_CLOSE} 0 0
	${EndIf}
    
    ; If a version is already installed. Disable directory checking
    Call GetEbisuInstalledVersionMajor
    Pop $R0
    IntCmp $R0 0 0 done done

    ; Remove percents, like ? and * they are removed automagically
    ; ${WordReplace} $INSTDIR "%" "" "+" $R0

    ; Something replaced there?
    ; StrCmp $INSTDIR $R0 lenCheck

    ; When NSIS leaves the page it copies the line edit back to INSTDIR
    ; so we must also modify the line edit text
    ; StrCpy $INSTDIR $R0

    ; !insertmacro UpdateDirDialog
	SendMessage $StartCheck ${BM_GETCHECK} 0 0 $StartCheckResult
	SendMessage $DesktopCheck ${BM_GETCHECK} 0 0 $DesktopCheckResult
	SendMessage $AutostartCheck ${BM_GETCHECK} 0 0 $AutostartCheckResult
	SendMessage $AutoupdateCheck ${BM_GETCHECK} 0 0 $AutoupdateCheckResult
	SendMessage $TelemOOCheck ${BM_GETCHECK} 0 0 $TelemOOCheckResult
	
lenCheck:

    ; Remove trailing backslash
    StrCpy $0 $INSTDIR "" -1
    StrCmp $0 "\" 0 +2
    StrCpy $INSTDIR $INSTDIR -1

    Push $INSTDIR
		Call CheckValidDirectory
 		Pop $0  ; (1 if valid, 0 if invalid)
    IntCmp $0 1 validFolder 
   
    MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONSTOP $(EADM_INSTALL_PATH_INVALID)
    Abort
    
validFolder:

    ; Check installation dir length
    StrLen $0 "$INSTDIR\legacyPM\lang\CoreStrings_xx_XX.xml"
    IntCmp $0 ${MAX_PATH} doneLenCheck doneLenCheck warnLen

warnLen:
    MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND $(EADM_INSTALL_PATH_TOO_LONG)
    Abort

doneLenCheck:
    ; We are allowing unicode chars in the path
    ; Comment the following line if we need to prevent this
    Goto doneUnicodeCheck

    StrCpy $0 "0"

CheckChar:
    ; check each character of the install dir for illegal entries
    StrCpy $1 $INSTDIR 1 $0
    StrCmp $1 "" doneUnicodeCheck

    ; Find $1 is in the legal character set
    ; The character (`) has been removed from valid characters
    Push "$$abcdefghijklmnopqrstuvwxyz1234567890 ~!@#%^&()_'+=-[]{};,.:\"
    Push $1
    Call StrStr
    Pop $R2

    ; If $1 is not found in the legal character set
    StrCmp $R2 "" WarnAndReset

    ; check the next char
    IntOp $0 $0 + 1
    goto CheckChar

WarnAndReset:

    StrCpy $INSTDIR "$PROGRAMFILES\${EBISU_FILENAME}"

    !insertmacro UpdateDirDialog
    
    MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND $(EADM_INSTALLDIR_WARNING)
    Abort

doneUnicodeCheck:
    ; Check Free disk space on destination dir
    Push $INSTDIR
    Call CheckFreeDiskSpace
    Pop $1
    IntCmp $1 0 0 endDiskSpace endDiskSpace
    MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONSTOP $(EADM_INSTALL_NODISKSPACE)
    Abort
    
endDiskSpace:
    ; Check if we are installing in the current users desktop
    SetShellVarContext current
		Call RefreshShellIcons

    ; Check if it is a subdir of the users desktop
    StrLen $0 $DESKTOP
    StrCpy $1 $INSTDIR $0
    StrCmp $1 $DESKTOP 0 endUsersDesktop
		; remove old instdir + parents on the current users desktop, because we re-create it on the desktop for all users
		RMDir $INSTDIR
    StrCpy $R0 $INSTDIR

nextParent:    	
    Push $R0
    StrCpy $R1 $R0	; save parent
    Call GetParent
    Pop $R0
		RMDir $R0		
    StrCmp $R0 $R1 0 nextParent
		Call RefreshShellIcons
    ; Replace by All users desktop
    SetShellVarContext all
		Call RefreshShellIcons

    StrCpy $1 $INSTDIR "" $0
    StrCpy $INSTDIR "$DESKTOP$1"

    !insertmacro UpdateDirDialog
    
    SetShellVarContext current

endUsersDesktop:
    ; If the destination directory is NOT empty then append EADownloadManager
    ; unless it is already named EADownloadManager
    ${GetFileName} "$INSTDIR" $0
    StrCmp $0 ${EBISU_FILENAME} endEmptyCheck
    
    Push $INSTDIR
    Call DirState
    Pop $0
    
    ; 1 means full (otherwise empty or unexistant)
    IntCmp $0 1 +2
    Goto endEmptyCheck
    StrCpy $INSTDIR "$INSTDIR\${EBISU_FILENAME}"
		!insertmacro UpdateDirDialog
    
    ; Check that the directory is not root, otherwise append Origin
    StrCpy $0 $INSTDIR
    StrLen $1 "$0"
    IntCmp $1 2 0 done next			; for example c:
    StrCpy $INSTDIR "$0\${EBISU_FILENAME}"
    !insertmacro UpdateDirDialog
    goto done
next:
    IntCmp $1 3 0 done done		; for example c:\
    StrCpy $INSTDIR "$0${EBISU_FILENAME}"
    !insertmacro UpdateDirDialog
    
endEmptyCheck:
done:

FunctionEnd

; -------------------------------------------------------------------------------
; onLeaveInstPage: This callback will be called when the install page has been left
; -------------------------------------------------------------------------------
Function onLeaveInstPage

    ; If dialog containing user adjustable options was shown,
    ; then use user selected 'autostart' setting
    StrCmp $ShowDirPage "true" checkAutostart done
	
checkAutostart:
	IntCmp $AutostartCheckResult 0 noAutoRun
	Call RunEADMAfterBootComputer
	Goto done
	
noAutoRun:
	Call DontRunEADMAfterBootComputer
	
done:
	; We always want to run after install now
	Call RunAfterInstallation
	
FunctionEnd

; -------------------------------------------------------------------------------
; fixInstallationDir: Adjust installation dir to be a proper
; -------------------------------------------------------------------------------
Function fixInstallationDir

    Exch $R0
    Push $0
    Push $1
    Push $R1

    ; Remove percents, ?, *, etc
    ;${WordReplace} $R0 "%" "" "+" $R0
    ;${WordReplace} $R0 "?" "" "+" $R0
    ;${WordReplace} $R0 "*" "" "+" $R0
    ;${WordReplace} $R0 ">" "" "+" $R0
    ;${WordReplace} $R0 "<" "" "+" $R0
    
    ; Check if we are installing in the current users desktop
    SetShellVarContext current
		Call RefreshShellIcons

    ; Check if it is a subdir of the users desktop
    StrLen $0 $DESKTOP
    StrCpy $1 $R0 $0
    StrCmp $1 $DESKTOP 0 endUsersDesktop
		; remove old instdir + parent on the current users desktop, because we re-create it on the desktop for all users
		RMDir $R0
    
nextParent:    	
    Push $R0
    StrCpy $R1 $R0	; save parent
    Call GetParent
    Pop $R0
		RMDir $R0		
    StrCmp $R0 $R1 0 nextParent
        
		Call RefreshShellIcons

    ; Replace by All users desktop
    SetShellVarContext all
		Call RefreshShellIcons

    StrCpy $1 $R0 "" $0
    StrCpy $R0 "$DESKTOP$1"

    SetShellVarContext current

endUsersDesktop:
    ; If the destination directory is NOT empty then append Origin
    ; unless it is already named Origin
    ${GetFileName} "$R0" $0
    StrCmp $0 ${EBISU_FILENAME} endEmptyCheck

    Push $R0
    Call DirState
    Pop $0

    ; 1 means full (otherwise empty or unexistant)
    IntCmp $0 1 +2
    Goto endEmptyCheck

    StrCpy $R0 "$R0\${EBISU_FILENAME}"

endEmptyCheck:

    ; Check that the directory is not root, otherwise append Origin
    StrLen $1 "$R0"
    IntCmp $1 2 0 done next
    StrCpy $R0 "$R0\${EBISU_FILENAME}"
    goto done
next:
    IntCmp $1 3 0 done done
    StrCpy $R0 "$R0${EBISU_FILENAME}"

done:
		Pop $R1
    Pop $1
    Pop $0
    Exch $R0

FunctionEnd

;------------------------------------------------------------------------------
; FakeEADM4Installation
; Create the appropriate keys to simulate EADM 4.x is installed
; Modifies no registers/variables
;------------------------------------------------------------------------------
Function FakeEADM4Installation

    ${If} ${RunningX64}
        SetRegView 64
    ${EndIf}
	WriteRegStr HKLM "SOFTWARE\Classes\Installer\Products\D139E7FE48CDB174D86B8A3385904547\SourceList" "" ""
	${If} ${RunningX64}
        SetRegView 32
    ${EndIf}

FunctionEnd

;------------------------------------------------------------------------------
; RemoveFakeEADM4Installation
; Remove keys and files that were created during installation to fake
; that an EADM 4.x installation was present
;------------------------------------------------------------------------------
Function un.RemoveFakeEADM4Installation

    ${If} ${RunningX64}
        SetRegView 64
    ${EndIf}
    DeleteRegKey HKLM "SOFTWARE\Classes\Installer\Products\D139E7FE48CDB174D86B8A3385904547"
    ${If} ${RunningX64}
        SetRegView 32
    ${EndIf}

FunctionEnd

;------------------------------------------------------------------------------
; MigrateCache
; Save the cache path to the new config file
;------------------------------------------------------------------------------
Function MigrateCache

    Push $0
    Push $1
	
    ; Get the runtime preferences into $1
    Call GetRuntimePreferencesFileName
    Pop $1
    StrCmp $1 "" 0 +2
    StrCpy $1 "$INSTDIR\EACore_App.ini" ; default to EACore_App.ini if an error occured

    ; Create the runtime preferences if needed
    IfFileExists $1 save
	
    Push $1
    Call GetParent
    Pop $0
    CreateDirectory $0	
	
save:
    StrCmp $sCurrentCachePath "" locale

    ; Adjust so the cache matches EADM 8 requirements
    ; The folder will be moved too
    Call FixPreviousCachePath

    ; Ensure everyone has access to the cache folder
		SetOutPath "$TEMP"
		StrCpy $0 "$sCurrentCachePath"
		System::Call 'installerdll$tempDLL::GrantEveryoneAccessToFile(w r0)'
		SetOutPath "$INSTDIR"
    
    ; Precreate an UTF16 file.
    ClearErrors
    FileOpen $0 "$1" w
    IfErrors dowrite
    FileWriteUTF16LE $0 "; Config"
    FileClose $0

dowrite:
    WriteINIStr "$1" "EACore" "base_cache_dir" "$sCurrentCachePath"
    WriteINIStr "$1" "EACoreSession" "full_base_cache_dir" "$sCurrentCachePath"
    
locale:
    StrCmp $sPreviousLocale "" done
    WriteINIStr "$1" "Vault" "locale" "$sPreviousLocale"
    
done:
	Call UpdatePreferencesINI
    Pop $1
    Pop $0

FunctionEnd

;------------------------------------------------------------------------------
; UpdatePreferencesINI
; Save the cache path to the settings file
;------------------------------------------------------------------------------
Function UpdatePreferencesINI
	
	Push $0
	Push $1
	
	!insertmacro DebugMessageBox "Updating settings cache, cache is $sCurrentCachePath"
	StrCmp $sCurrentCachePath "" done
	
	; Get the settings xml file into $1
	Call GetPreferencesINIFileName
	Pop $1
	StrCmp $1 "" done
	
	IfFileExists $1 createPreferences
	Goto writePreferences	
	
createPreferences:
	Push $1
	Call GetParent
	Pop $0
	CreateDirectory $0
	
writePreferences:
	; Precreate an UTF16 file.
    ClearErrors
    FileOpen $0 "$1" w
    IfErrors dowrite
    FileWriteUTF16LE $0 "; Config"
    FileClose $0
	
dowrite:
	WriteINIStr "$1" "EACore" "base_cache_dir" "$sCurrentCachePath"
	WriteINIStr "$1" "EACoreSession" "full_base_cache_dir" "$sCurrentCachePath"
	
done:
	Pop $1
	Pop $0

FunctionEnd

;------------------------------------------------------------------------------
; ModifyAccessRights
; Change access rights for some EADM files and folders
;------------------------------------------------------------------------------
Function ModifyAccessRights

    Push $0
    Push $1

    ; Logs and runtime preferences folder
    ; Application Data\Electronic Arts\EA Core\prefs
    ; Application Data\Electronic Arts\EA Core\logs
		StrCpy $1 ""
		System::Call "shell32::SHGetFolderPathW(i 0, i ${CSIDL_COMMON_APPDATA}, i 0, i 0, w .r1) i.r0"
    StrCmp $1 "" err1
    StrCpy $0 "$1\Electronic Arts\EA Core"
    CreateDirectory $0
	SetOutPath "$TEMP"
	System::Call 'installerdll$tempDLL::GrantEveryoneAccessToFile(w r0)'
	SetOutPath "$INSTDIR"
   
err1:
    Pop $1
    Pop $0

FunctionEnd

; -------------------------------------------------------------------------------
; RegisterAgentsManually: Register agents with writing to the ini file
; -------------------------------------------------------------------------------
Function RegisterAgentsManually

    Push $0
    Push $R0

    ; Counter
    StrCpy $R0 "0"

    ; Register Vault for launch
    StrCpy $0 '"$INSTDIR\${EBISU_FILENAME}\${EBISU_FILENAME}.exe" /locale={locale} /patchLocale={patchLocale} /corePort={CPPort}'
    WriteINIStr "$INSTDIR\EACore_App.ini" "EACore" "core.agents.$R0.id" "AGENT_VAULT"
    WriteINIStr "$INSTDIR\EACore_App.ini" "EACore" "core.agents.$R0.path" "$0"
    WriteINIStr "$INSTDIR\EACore_App.ini" "EACore" "core.agents.$R0.supportedtasks" "TASK_LAUNCH_VAULT, TASK_SET_FILTER, TASK_ADD_DOWNLOAD_JOB, TASK_SET_UI_MODE"
    WriteINIStr "$INSTDIR\EACore_App.ini" "EACore" "core.agents.$R0.handlertype" "AIR"
    IntOp $R0 $R0 + 1

!ifdef INCLUDE_QT_PATCHPROGRESS

    StrCpy $0 '"$INSTDIR\PatchProgress.exe" /locale={locale} /patchLocale={patchLocale} /guid={clientGUID}'
    WriteINIStr "$INSTDIR\EACore_App.ini" "EACore" "core.agents.$R0.id" "AGENT_PATCH_DL_PROGRESS"
    WriteINIStr "$INSTDIR\EACore_App.ini" "EACore" "core.agents.$R0.path" "$0"
    WriteINIStr "$INSTDIR\EACore_App.ini" "EACore" "core.agents.$R0.supportedtasks" "TASK_PATCH_DL_PROGRESS"
    WriteINIStr "$INSTDIR\EACore_App.ini" "EACore" "core.agents.$R0.handlertype" "QT"
    IntOp $R0 $R0 + 1

!endif  ;INCLUDE_QT_PATCHPROGRESS

!ifdef INCLUDE_LOGIN_DIALOG

    StrCpy $0 '"$INSTDIR\Login.exe" /locale={locale} /patchLocale={patchLocale} /gameName="{data5}" /guid={clientGUID}'
    WriteINIStr "$INSTDIR\EACore_App.ini" "EACore" "core.agents.$R0.id" "AGENT_LOGIN"
    WriteINIStr "$INSTDIR\EACore_App.ini" "EACore" "core.agents.$R0.path" "$0"
    WriteINIStr "$INSTDIR\EACore_App.ini" "EACore" "core.agents.$R0.supportedtasks" "TASK_LOGIN_USER"
    WriteINIStr "$INSTDIR\EACore_App.ini" "EACore" "core.agents.$R0.handlertype" "QT"
    IntOp $R0 $R0 + 1

!endif  ;INCLUDE_LOGIN_DIALOG

!ifdef INCLUDE_MESSAGE_DIALOG

    StrCpy $0 '"$INSTDIR\MessageDlg.exe" /locale={locale}'
    WriteINIStr "$INSTDIR\EACore_App.ini" "EACore" "core.agents.$R0.id" "AGENT_MESSAGE"
    WriteINIStr "$INSTDIR\EACore_App.ini" "EACore" "core.agents.$R0.path" "$0"
    WriteINIStr "$INSTDIR\EACore_App.ini" "EACore" "core.agents.$R0.supportedtasks" "TASK_DISPLAY_MESSAGE"
    WriteINIStr "$INSTDIR\EACore_App.ini" "EACore" "core.agents.$R0.handlertype" "QT"
    IntOp $R0 $R0 + 1

!endif  ;INCLUDE_MESSAGE_DIALOG

end:
    Pop $R0
    Pop $0

FunctionEnd

; -------------------------------------------------------------------------------
; onVerifyInstDir: This callback will be called every time the user changes 
; the install directory
; -------------------------------------------------------------------------------
Function .onVerifyInstDir

FunctionEnd

; -------------------------------------------------------------------------------
; This callback is called when the installer terminates due to failure
; -------------------------------------------------------------------------------
Function .onInstFailed

	; Update the registry that we failed our last install so the launcher knows to not launch the client
	WriteRegStr HKLM "SOFTWARE\${productname}" "InstallSuccesfull" "false"

FunctionEnd

; -------------------------------------------------------------------------------
; This callback is called when the installer terminates due to success
; -------------------------------------------------------------------------------
Function .onInstSuccess

	; Update the registry that we failed our last install so the launcher knows to launch the client
	WriteRegStr HKLM "SOFTWARE\${productname}" "InstallSuccesfull" "true"

FunctionEnd


;------------------------------------------------------------------------------
; CheckForLockedIGOFiles
; check if IGO files are locked and display a dialog
; Usage:
;		call CheckForLockedIGOFiles
;------------------------------------------------------------------------------
!macro FUNC_CheckForLockedIGOFiles un
Function ${un}CheckForLockedIGOFiles

IGOstillLocked:

	SetOutPath "$TEMP"
    StrCpy $0 ""
	System::Call 'installerdll$tempDLL::GetLockingProcessList(w .r0, i ${NSIS_MAX_STRLEN}) i.r1'
	!insertmacro DebugMessageBox "GetLockingProcessList: $0 ($1)"
	SetOutPath "$INSTDIR"

	IntCmp $1 0 IGOisNotLocked 0 0
		
		; repeat the check after the user hits okay...
	StrCpy $1 $(EADM_WARNING_LOCKED_FILES)
    MessageBox MB_RETRYCANCEL|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONEXCLAMATION|MB_DEFBUTTON1 "$1$\r$\n$0" IDRETRY IGOstillLocked IDCANCEL stop

    stop:
    Quit
    

IGOisNotLocked:		

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_CheckForLockedIGOFiles ""
!insertmacro FUNC_CheckForLockedIGOFiles "un."

;------------------------------------------------------------------------------
; CheckForRunningInstances
; check if eacoreserver or eadmui are running and show a warning
; Usage:
;		Push "1"	// 1 means running silent - 0 means show warning messages
;		call CheckForRunningInstances
;------------------------------------------------------------------------------
!macro FUNC_CheckForRunningInstances un
Function ${un}CheckForRunningInstances

    ReadRegStr $sPreviousClientPath HKLM "SOFTWARE\Electronic Arts\EA Core" "ClientPath"

    ; Get the installation directory (ClientPath points to the executable)
    Push $sPreviousClientPath
    Call ${un}GetParent
    Pop $sPreviousClientPath

    ; Remove trailing backslash
    StrCpy $0 $sPreviousClientPath "" -1
    StrCmp $0 "\" 0 +2
    StrCpy $sPreviousClientPath $sPreviousClientPath -1

	SetOutPath "$TEMP"
		; req. to enumerate full process paths
    System::Call 'installerdll$tempDLL::GrabDBGPrivilege() i.r1'
	

		; If we run as an update, skip the messages and run silently
		; MY: but for ITE metrics, we actually need to know whether it is running or not, regardless of whether it's silent
		;Pop $1
    ;IntCmp $1 1 NothingFound
		Pop $runSilent

		; Core 4
    System::Call 'installerdll$tempDLL::FindProcess(w "$INSTDIR\Core.exe", i 1) i.r0'
    ;Push "$INSTDIR\Core.exe"
	;	Call ${un}FindProcessWithPath
	;	Pop $0
    IntCmp $0 0 Core4NotFound1
		;set var to indicate already running, and if silent, skip the rest
		StrCpy $alreadyRunning "1"
    IntCmp $runSilent 1 NothingFound
    MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONEXCLAMATION $(EADM_UNINSTALL_WARNING_INSTANCE_DETECTED)
		Goto NothingFound

Core4NotFound1:    

    System::Call 'installerdll$tempDLL::FindProcess(w "$sPreviousClientPath\Core.exe", i 1) i.r0'
;    Push "$sPreviousClientPath\Core.exe"
;		Call ${un}FindProcessWithPath
;		Pop $0
    IntCmp $0 0 Core4NotFound2
		;set var to indicate already running, and if silent, skip the rest
		StrCpy $alreadyRunning "1"
    IntCmp $runSilent 1 NothingFound
    MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONEXCLAMATION $(EADM_UNINSTALL_WARNING_INSTANCE_DETECTED)
		Goto NothingFound

Core4NotFound2:    

		; Core 5 & 6 & 7
    System::Call 'installerdll$tempDLL::FindProcess(w "$INSTDIR\EACoreServer.exe", i 1) i.r0'
;    Push "$INSTDIR\EACoreServer.exe"
;		Call ${un}FindProcessWithPath
;		Pop $0
    IntCmp $0 0 CoreNewNotFound1
		;set var to indicate already running, and if silent, skip the rest
		StrCpy $alreadyRunning "1"
    IntCmp $runSilent 1 NothingFound
    MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONEXCLAMATION $(EADM_UNINSTALL_WARNING_INSTANCE_DETECTED)
		Goto NothingFound

CoreNewNotFound1:

    System::Call 'installerdll$tempDLL::FindProcess(w "$sPreviousClientPath\EACoreServer.exe", i 1) i.r0'
;    Push "$sPreviousClientPath\EACoreServer.exe"
;		Call ${un}FindProcessWithPath
;		Pop $0
    IntCmp $0 0 CoreNewNotFound2
		;set var to indicate already running, and if silent, skip the rest
		StrCpy $alreadyRunning "1"
    IntCmp $runSilent 1 NothingFound
    MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONEXCLAMATION $(EADM_UNINSTALL_WARNING_INSTANCE_DETECTED)
		Goto NothingFound

CoreNewNotFound2:    

    ; Ebisu
    System::Call 'installerdll$tempDLL::FindProcess(w "$INSTDIR\${EBISU_FILENAME}.exe", i 1) i.r0'
;    Push "$INSTDIR\${EBISU_FILENAME}.exe"	
;		Call ${un}FindProcessWithPath
;		Pop $0
    IntCmp $0 0 EbisuFound1
		;set var to indicate already running, and if silent, skip the rest
		StrCpy $alreadyRunning "1"
    IntCmp $runSilent 1 NothingFound
    MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONEXCLAMATION $(EADM_UNINSTALL_WARNING_INSTANCE_DETECTED)
		Goto NothingFound

EbisuFound1:

    ; Ebisu
    System::Call 'installerdll$tempDLL::FindProcess(w "$sPreviousClientPath\${EBISU_FILENAME}.exe", i 1) i.r0'
;    Push "$sPreviousClientPath\${EBISU_FILENAME}.exe"
;		Call ${un}FindProcessWithPath
;		Pop $0
    IntCmp $0 0 EADMFound1
		;set var to indicate already running, and if silent, skip the rest
		StrCpy $alreadyRunning "1"
    IntCmp $runSilent 1 NothingFound
    MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONEXCLAMATION $(EADM_UNINSTALL_WARNING_INSTANCE_DETECTED)


EADMFound1:

    ; Ebisu
    System::Call 'installerdll$tempDLL::FindProcess(w "$INSTDIR\EADMUI.exe", i 1) i.r0'
;    Push "$INSTDIR\EADMUI.exe"	
;		Call ${un}FindProcessWithPath
;		Pop $0
    IntCmp $0 0 EADMFound2
		;set var to indicate already running, and if silent, skip the rest
		StrCpy $alreadyRunning "1"
    IntCmp $runSilent 1 NothingFound
    MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONEXCLAMATION $(EADM_UNINSTALL_WARNING_INSTANCE_DETECTED)
		Goto NothingFound
		
EADMFound2:

    ; Ebisu
    System::Call 'installerdll$tempDLL::FindProcess(w "$sPreviousClientPath\EADMUI.exe", i 1) i.r0'
;    Push "$sPreviousClientPath\EADMUI.exe"	
;		Call ${un}FindProcessWithPath
;		Pop $0
    IntCmp $0 0 Legacy1
		;set var to indicate already running, and if silent, skip the rest
		StrCpy $alreadyRunning "1"
    IntCmp $runSilent 1 NothingFound
    MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONEXCLAMATION $(EADM_UNINSTALL_WARNING_INSTANCE_DETECTED)
		Goto NothingFound

Legacy1:
		; check if legacy processes are running
    System::Call 'installerdll$tempDLL::FindProcess(w "$INSTDIR\${legacyCore}\EAProxyInstaller.exe", i 1) i.r0'
;    Push "$INSTDIR\${legacyCore}\EAProxyInstaller.exe"	
;		Call ${un}FindProcessWithPath
;		Pop $0
    IntCmp $0 0 Legacy2
		;set var to indicate already running, and if silent, skip the rest
		StrCpy $alreadyRunning "1"
    IntCmp $runSilent 1 NothingFound
    MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONEXCLAMATION $(EADM_UNINSTALL_WARNING_INSTANCE_DETECTED)
		Goto NothingFound

Legacy2:
    System::Call 'installerdll$tempDLL::FindProcess(w "$INSTDIR\${legacyCore}\OriginClientService.exe", i 1) i.r0'
;    Push "$INSTDIR\${legacyCore}\OriginClientService.exe"	
;		Call ${un}FindProcessWithPath
;		Pop $0
    IntCmp $0 0 Legacy3
		;set var to indicate already running, and if silent, skip the rest
		StrCpy $alreadyRunning "1"
    IntCmp $runSilent 1 NothingFound
    MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONEXCLAMATION $(EADM_UNINSTALL_WARNING_INSTANCE_DETECTED)
		Goto NothingFound

Legacy3:
    System::Call 'installerdll$tempDLL::FindProcess(w "$INSTDIR\${legacyCore}\OriginLegacyCLI.exe", i 1) i.r0'
;    Push "$INSTDIR\${legacyCore}\OriginLegacyCLI.exe"	
;		Call ${un}FindProcessWithPath
;		Pop $0
    IntCmp $0 0 Legacy4
		;set var to indicate already running, and if silent, skip the rest
		StrCpy $alreadyRunning "1"
    IntCmp $runSilent 1 NothingFound
    MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONEXCLAMATION $(EADM_UNINSTALL_WARNING_INSTANCE_DETECTED)
		Goto NothingFound

Legacy4:
    System::Call 'installerdll$tempDLL::FindProcess(w "$INSTDIR\${legacyCore}\EACoreServer.exe", i 1) i.r0'
;    Push "$INSTDIR\${legacyCore}\EACoreServer.exe"	
;		Call ${un}FindProcessWithPath
;		Pop $0
    IntCmp $0 0 Legacy5
		;set var to indicate already running, and if silent, skip the rest
		StrCpy $alreadyRunning "1"
    IntCmp $runSilent 1 NothingFound
    MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONEXCLAMATION $(EADM_UNINSTALL_WARNING_INSTANCE_DETECTED)
		Goto NothingFound

Legacy5:
    System::Call 'installerdll$tempDLL::FindProcess(w "$INSTDIR\${legacyCore}\OriginUninstall.exe", i 1) i.r0'
;    Push "$INSTDIR\${legacyCore}\OriginUninstall.exe"	
;		Call ${un}FindProcessWithPath
;		Pop $0
    IntCmp $0 0 Legacy6
		;set var to indicate already running, and if silent, skip the rest
		StrCpy $alreadyRunning "1"
    IntCmp $runSilent 1 NothingFound
    MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONEXCLAMATION $(EADM_UNINSTALL_WARNING_INSTANCE_DETECTED)
		Goto NothingFound

Legacy6:
    System::Call 'installerdll$tempDLL::FindProcess(w "$INSTDIR\${legacyCore}\PatchProgress.exe", i 1) i.r0'
;    Push "$INSTDIR\${legacyCore}\PatchProgress.exe"	
;		Call ${un}FindProcessWithPath
;		Pop $0
    IntCmp $0 0 Legacy7
		;set var to indicate already running, and if silent, skip the rest
		StrCpy $alreadyRunning "1"
    IntCmp $runSilent 1 NothingFound
    MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONEXCLAMATION $(EADM_UNINSTALL_WARNING_INSTANCE_DETECTED)
		Goto NothingFound

Legacy7:
    System::Call 'installerdll$tempDLL::FindProcess(w "$INSTDIR\${legacyCore}\Login.exe", i 1) i.r0'
;    Push "$INSTDIR\${legacyCore}\Login.exe"	
;		Call ${un}FindProcessWithPath
;		Pop $0
    IntCmp $0 0 Legacy8
		;set var to indicate already running, and if silent, skip the rest
		StrCpy $alreadyRunning "1"
    IntCmp $runSilent 1 NothingFound
    MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONEXCLAMATION $(EADM_UNINSTALL_WARNING_INSTANCE_DETECTED)
		Goto NothingFound

Legacy8:
    System::Call 'installerdll$tempDLL::FindProcess(w "$INSTDIR\${legacyCore}\MessageDlg.exe", i 1) i.r0'
;    Push "$INSTDIR\${legacyCore}\MessageDlg.exe"	
;		Call ${un}FindProcessWithPath
;		Pop $0
    IntCmp $0 0 NothingFound
		;set var to indicate already running, and if silent, skip the rest
		StrCpy $alreadyRunning "1"
    IntCmp $runSilent 1 NothingFound
    MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONEXCLAMATION $(EADM_UNINSTALL_WARNING_INSTANCE_DETECTED)
		Goto NothingFound

NothingFound:
	SetOutPath "$INSTDIR"
FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_CheckForRunningInstances ""
!insertmacro FUNC_CheckForRunningInstances "un."

;------------------------------------------------------------------------------
; KillRunningCoreServer
; Kill Core in destination directory
;------------------------------------------------------------------------------
!macro FUNC_KillRunningCoreServer un
Function ${un}KillRunningCoreServer

    ReadRegStr $sPreviousClientPath HKLM "SOFTWARE\Electronic Arts\EA Core" "ClientPath"
	ReadRegStr $sPreviousOriginPath HKLM "SOFTWARE\Origin" "ClientPath"
    ; Get the installation directory (ClientPath points to the executable)
    Push $sPreviousClientPath
    Call ${un}GetParent
    Pop $sPreviousClientPath
	
    Push $sPreviousOriginPath
    Call ${un}GetParent
    Pop $sPreviousOriginPath	

    ; Remove trailing backslash
    StrCpy $0 $sPreviousClientPath "" -1
    StrCmp $0 "\" 0 +2
    StrCpy $sPreviousClientPath $sPreviousClientPath -1
	
    StrCpy $0 $sPreviousOriginPath "" -1
    StrCmp $0 "\" 0 +2
    StrCpy $sPreviousOriginPath $sPreviousOriginPath -1
    
	SetOutPath "$TEMP"
		; req. to enumerate full process paths
    System::Call 'installerdll$tempDLL::GrabDBGPrivilege() i.r1'

    ; Sandboxed & Legacy Core
    System::Call 'installerdll$tempDLL::KillProcess(w "$INSTDIR\${legacyCore}\EACoreServer.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousClientPath\EACoreServer.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$INSTDIR\EACoreServer.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousOriginPath\EACoreServer.exe", i 1)'
    

    System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousClientPath\EACoreCLI.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$INSTDIR\EACoreCLI.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousOriginPath\EACoreCLI.exe", i 1)'
    

    System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousClientPath\EADMLegacyCLI.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$INSTDIR\EADMLegacyCLI.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousOriginPath\EADMLegacyCLI.exe", i 1)'
    

    System::Call 'installerdll$tempDLL::KillProcess(w "$INSTDIR\${legacyCore}\OriginLegacyCLI.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousClientPath\OriginLegacyCLI.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$INSTDIR\OriginLegacyCLI.exe", i 1)'
    
	System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousOriginPath\OriginLegacyCLI.exe", i 1)'
    

    System::Call 'installerdll$tempDLL::KillProcess(w "$INSTDIR\${legacyCore}\OriginClientService.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousClientPath\OriginClientService.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$INSTDIR\OriginClientService.exe", i 1)'
    
	System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousOriginPath\OriginClientService.exe", i 1)'
    
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$INSTDIR\${legacyCore}\OriginWebHelperService.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousClientPath\OriginWebHelperService.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$INSTDIR\OriginWebHelperService.exe", i 1)'
    
	System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousOriginPath\OriginWebHelperService.exe", i 1)'
    

    System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousClientPath\EADMClientService.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$INSTDIR\EADMClientService.exe", i 1)'
    
	System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousOriginPath\EADMClientService.exe", i 1)'
    

    System::Call 'installerdll$tempDLL::KillProcess(w "$INSTDIR\${legacyCore}\EAProxyInstaller.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousClientPath\EAProxyInstaller.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$INSTDIR\EAProxyInstaller.exe", i 1)'
    
	System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousOriginPath\EAProxyInstaller.exe", i 1)'
    

    System::Call 'installerdll$tempDLL::KillProcess(w "$INSTDIR\${legacyCore}\OriginUninstall.exe", i 1)'
    
	System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousOriginPath\OriginUninstall.exe", i 1)'
    

    ; Login dialog
    System::Call 'installerdll$tempDLL::KillProcess(w "$INSTDIR\${legacyCore}\Login.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousClientPath\Login.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$INSTDIR\Login.exe", i 1)'
    
	System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousOriginPath\Login.exe", i 1)'
    

    ; Message dialog
    System::Call 'installerdll$tempDLL::KillProcess(w "$INSTDIR\${legacyCore}\MessageDlg.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousClientPath\MessageDlg.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$INSTDIR\MessageDlg.exe", i 1)'
    
	System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousOriginPath\MessageDlg.exe", i 1)'
    

    System::Call 'installerdll$tempDLL::KillProcess(w "$INSTDIR\${legacyCore}\PatchProgress.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousClientPath\PatchProgress.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$INSTDIR\PatchProgress.exe", i 1)'
    
	System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousOriginPath\PatchProgress.exe", i 1)'
    

    ; EADM 5.x
    System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousClientPath\Core.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$INSTDIR\Core.exe", i 1)'
    
	System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousOriginPath\Core.exe", i 1)'
    

    ; EADM 6.x
    System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousClientPath\EADownloadManager\EADownloadManager.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$INSTDIR\EADownloadManager\EADownloadManager.exe", i 1)'
    
	System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousOriginPath\EADownloadManager.exe", i 1)'
    

    ; EADM 7.x / 8.x
    System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousClientPath\EADMUI.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$INSTDIR\EADMUI.exe", i 1)'
    
	System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousOriginPath\EADMUI.exe", i 1)'
    
    
    ; Ebisu
    System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousClientPath\${EBISU_FILENAME}.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$INSTDIR\${EBISU_FILENAME}.exe", i 1)'
    
	System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousOriginPath\${EBISU_FILENAME}.exe", i 1)'
    

    ; old Middleware
    System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousClientPath\EADM.exe", i 1)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "$INSTDIR\EADM.exe", i 1)'
    
	System::Call 'installerdll$tempDLL::KillProcess(w "$sPreviousOriginPath\EADM.exe", i 1)'
    

	; kill all processes with a given name, so we have to be carefull about the names
	
	System::Call 'installerdll$tempDLL::KillProcess(w "EADM.exe", i 0)'
    
	System::Call 'installerdll$tempDLL::KillProcess(w "${EBISU_FILENAME}.exe", i 0)'
    
	System::Call 'installerdll$tempDLL::KillProcess(w "EADMUI.exe", i 0)'
    
	System::Call 'installerdll$tempDLL::KillProcess(w "EADownloadManager.exe", i 0)'
    
	System::Call 'installerdll$tempDLL::KillProcess(w "EAProxyInstaller.exe", i 0)'
    
	System::Call 'installerdll$tempDLL::KillProcess(w "EACoreCLI.exe", i 0)'
    
	System::Call 'installerdll$tempDLL::KillProcess(w "EADMLegacyCLI.exe", i 0)'
    
	System::Call 'installerdll$tempDLL::KillProcess(w "OriginLegacyCLI.exe", i 0)'
    
	System::Call 'installerdll$tempDLL::KillProcess(w "OriginClientService.exe", i 0)'
    
    System::Call 'installerdll$tempDLL::KillProcess(w "OriginWebHelperService.exe", i 0)'
    
	System::Call 'installerdll$tempDLL::KillProcess(w "EADMClientService.exe", i 0)'
    
	System::Call 'installerdll$tempDLL::KillProcess(w "EACoreServer.exe", i 0)'
    
	
  	SetOutPath "$INSTDIR"
    
FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_KillRunningCoreServer ""
!insertmacro FUNC_KillRunningCoreServer "un."




;------------------------------------------------------------------------------
; IsFileLocked
; check if a file is locked
; Usage:
;		Push "myfile.dll"
;		call IsFileLocked
;		Pop $1
;		StrCmp $1 "locked" label_the_file_is_locked
;------------------------------------------------------------------------------

!define GENERIC_WRITE 0x40000000
!define GENERIC_READ 0x80000000
!define FILE_GENERIC_READ 0x00120089
!define FILE_GENERIC_WRITE 0x00120116
!define FILE_ATTRIBUTE_NORMAL 0x00000080
!define FILE_SHARE_READ 0x00000001  
!define OPEN_ALWAYS 4
!define OPEN_EXISTING 3
!define INVALID_HANDLE_VALUE -1
!define ERROR_SHARING_VIOLATION     32
!macro FUNC_IsFileLocked un
Function ${un}IsFileLocked
	Pop $0

  System::Call "kernel32::SetLastError(i 0)"
	System::Call "kernel32.dll::CreateFile(t '$0', i ${GENERIC_READ}, i 0, i 0, i ${OPEN_EXISTING}, i 0, i 0) i .r2" 
	;System::Call "kernel32.dll::GetLastError( ) i .r3"
	IntCmp $2 ${INVALID_HANDLE_VALUE} file_does_not_exist 
	System::Call "kernel32::CloseHandle(i $2) i .r1"

  System::Call "kernel32::SetLastError(i 0)"
  System::Call "kernel32.dll::CreateFile(t '$0', i ${GENERIC_WRITE}, i 0, i 0, i ${OPEN_ALWAYS}, i 0, i 0) i .r2" 
	System::Call "kernel32.dll::GetLastError( ) i .r3"
	IntCmp $2 ${INVALID_HANDLE_VALUE} check_last_error 
	System::Call "kernel32::CloseHandle(i $2) i .r1"

	IntCmp $3 ${ERROR_SHARING_VIOLATION} check_last_error


	goto file_does_not_exist

check_last_error:
 push "locked"
 goto done

file_does_not_exist:
 push "no"
done:
FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_IsFileLocked ""
!insertmacro FUNC_IsFileLocked "un."


Function ModifyMessageString
!insertmacro ReplaceSubStr "$(ebisu_installer_found_ito)" "%1" "Origin config for ItO.ini"
StrCpy $string_ebisu_installer_found_ito $MODIFIED_STR

!insertmacro ReplaceSubStr "$(ebisu_installer_copied_ito)" "%1" "Origin config for ItO.ini"
!insertmacro ReplaceSubStr "$MODIFIED_STR" "%2" "Origin Developer Tool"
StrCpy $string_ebisu_installer_copied_ito $MODIFIED_STR
FunctionEnd

Var originMajor
Var originMinor
Var originPoint
Var originBuild
;--------------------------------
; Installer Section
;--------------------------------
Section "Main Program" SecMain

!ifndef INNER

	; See if the parent process is belonging to EADM, then we run as an update and won't show some warnings 
  	${GetParameters} $0
		ClearErrors
  	StrCpy $1 "0" ; clear update variable
    ${GetOptions} $0 "/update" $2
    IfErrors noUpdate 0  
		StrCpy $1 "1" ; update
noUpdate:

    ; Remove trailing backslash
    StrCpy $0 $INSTDIR "" -1
    StrCmp $0 "\" 0 +2
    StrCpy $INSTDIR $INSTDIR -1

	DetailPrint $(installer_progress_detail_check)
	
    ; Show warning window, if the UI or EACoreServer is/are running

    Push $1	; push the update value
 	Call CheckForRunningInstances

    ; Kill any running instance
    Call KillRunningCoreServer

    ; check for locked IGO files
    Call CheckForLockedIGOFiles
       
	  ; Uninstall existing Core version
    Call UninstallPreviousEADMVersion

	  ; Uninstall existing EADM 7.x.x.x & 8.0.x.x versions
    Call UninstallEADM_7_8_Version

	; Make destination directory accesible by everyone
	SetOutPath "$TEMP"
	StrCpy $0 "$INSTDIR"
	CreateDirectory $0
	System::Call 'installerdll$tempDLL::GrantEveryoneAccessToFile(w r0)'
	SetOutPath "$INSTDIR"
  
    ; Remove files from destination directory
    ; Nothing happens if the files don't exist (fresh install)
    Delete "$INSTDIR\EACore.dll"
    Delete "$INSTDIR\CoreCmdPortalClient.dll"
    Delete "$INSTDIR\CmdPortalClient.dll"
    Delete "$INSTDIR\${uninstname}"
    Delete "$INSTDIR\EADM.exe"
    Delete "$INSTDIR\EACoreServer.exe"
    Delete "$INSTDIR\Login.exe"
    Delete "$INSTDIR\MessageDlg.exe"
    Delete "$INSTDIR\PatchProgress.exe"
  

    ; This is for older XP SP1? Most systems have winhttp.dll now.
    IfFileExists $SYSDIR\winhttp.dll dontNeedWinHTTP 0
    
    File /a "/oname=$SYSDIR\winhttp.dll" ${CORE_RUNTIMEDIR}\winhttp.dll

dontNeedWinHTTP:

	; special option for test installs on hudson - do not install the crt runtime, because it's so slow
  	${GetParameters} $0
	  ClearErrors
    ${GetOptions} $0 "/hudson" $1
    IfErrors noSpecialFlag 0
    Goto noRuntimeInstall

noSpecialFlag:
	; x64 CRT files are currently not required, because we do not ship IGO 64 bit or any other x64 part	
	; install microsoft crt runtime files
	System::Call "kernel32::GetCurrentProcess() i .s"
	System::Call "kernel32::IsWow64Process(i s, *i .r0)"
	
	SetOutPath "$TEMP"
	
	File WindowsInstaller-KB893803-v2-x86.exe		; required for WinXP<SP3 to install the v2010 runtime files
    ;;;;;;;;;;; Windows XP<SP3 only - Start ;;;;;;;;;;;;;;
	; install the update only on winxp that has no SP3 installed

	Push $0	
	Push $1	
	Push $2	
	Push $3	
	Push $4	

	System::Alloc 156
	Pop $0
	System::Call '*$0(i156)'
	System::Call kernel32::GetVersionExA(ir0)
	System::Call '*$0(i,&i4.r3,&i4.r4,i,i,&t128,&i2.r1,&i2.r2)'
	System::Free $0

	; major version 5
	IntCmp $3 5 0 notXPwithoutSP3 notXPwithoutSP3
	; minor version 1
	IntCmp $4 1 0 notXPwithoutSP3 notXPwithoutSP3
	; wServicePackMajor - 3 SP3
	IntCmp $1 3 0 notXPwithoutSP3 notXPwithoutSP3
	
	ExecWait '"$TEMP\WindowsInstaller-KB893803-v2-x86.exe" /promptrestart /quiet'

notXPwithoutSP3:		
    Pop $4
    Pop $3
    Pop $2
    Pop $1
    Pop $0

    IntCmp $0 0 x86_system	
    ; install VC Redists 2010 (For old OOA entitlements)
	File vcredist_x64.exe
	ExecWait '"$TEMP\vcredist_x64.exe" /q'
	; install VC Redists 2013 (For bootstrap/current version of Origin)
	File vcredist_x64_vs2013.exe
	ExecWait '"$TEMP\vcredist_x64_vs2013.exe" /q'

x86_system:
	;;;;;;;;;;; Windows XP<SP3 only - End ;;;;;;;;;;;;;;
	; install VC Redists 2010 (For old OOA entitlements)
	File vcredist_x86.exe
	ExecWait '"$TEMP\vcredist_x86.exe" /q'
	; install VC Redists 2013 (For bootstrap/current version of Origin)
	File vcredist_x86_vs2013.exe
	ExecWait '"$TEMP\vcredist_x86_vs2013.exe" /q'
    


noRuntimeInstall:

	; install root certificate update
	Call UpdateRootCertificate
	
	; Extract main files
	File /a "${EBISU_RUNTIMEDIR}\${EBISU_FILENAME}.exe"
	
	File /a "${EBISU_RUNTIMEDIR}\${productname}.VisualElementsManifest.xml"

	File /a "/oname=${uninstname}" "$%TEMP%\uninstaller.exe"

	!if ${special} == "integration"
	File /a "/oname=EACore.ini" "${EBISU_RUNTIMEDIR}\EACore_integration.ini"
	!endif

	!if ${special} == "qa"
	File /a "/oname=EACore.ini" "${EBISU_RUNTIMEDIR}\EACore_QA.ini"
	!endif
	
	!if ${special} == "integration"
		; *****************************
		; search for the ITO staging config file
		; *****************************
		
		${GetParameters} $0
		ClearErrors
		${GetOptions} $0 "/ITOStagingFileFound=" $1
		IfErrors 0 doNotUseIntegratedStagingFile
	    File /a "/oname=EACore.ini" "${EBISU_RUNTIMEDIR}\EACore_integration.ini"
	  	goto useIntegratedStagingFile
	  	
	  doNotUseIntegratedStagingFile:
	    IntCmp $runSilent 1 skipMessageBoxes
		IfSilent skipMessageBoxes

		Call ModifyMessageString
		MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONINFORMATION "$string_ebisu_installer_found_ito"

	  skipMessageBoxes:
  	  useIntegratedStagingFile:
	!endif

	!if ${special} == "qa"
		; *****************************
		; search for the ITO staging config file
		; *****************************
		
		${GetParameters} $0
		ClearErrors
		${GetOptions} $0 "/ITOStagingFileFound=" $1
		IfErrors 0 doNotUseIntegratedQAStagingFile
	    File /a "/oname=EACore.ini" "${EBISU_RUNTIMEDIR}\EACore_QA.ini"
	  	goto useIntegratedQAStagingFile
	  	
	  doNotUseIntegratedQAStagingFile:
	    IntCmp $runSilent 1 skipMessageBoxes
		IfSilent skipMessageBoxes

		Call ModifyMessageString
		MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONINFORMATION "$string_ebisu_installer_found_ito"

	  skipMessageBoxes:
	  useIntegratedQAStagingFile:
	!endif
				
	; *****************************
	; search & copy the ITO staging config file
	; *****************************
	
	${GetParameters} $0
	ClearErrors
	${GetOptions} $0 "/ITOStagingFileFound=" $1
	IfErrors doNotCopyITOFile
	IntCmp $runSilent 1 skipITOMessageBox
	IfSilent skipITOMessageBox
	
	Call ModifyMessageString
	MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONINFORMATION "$string_ebisu_installer_copied_ito"
	skipITOMessageBox:
		; copy ITO file source is in $1 destination is in $0
		StrCpy $0 "$INSTDIR\EACore.ini"   ;Path of copy file to
		StrCpy $2 0 ; only 0 or 1, set 0 to overwrite file if it already exists
		System::Call "kernel32::CopyFile(t r1, t r0, b r2) ?e"
		Pop $0 ; pops a bool
	doNotCopyITOFile:
	
	!if ${type} != "thin"
		; Extract the install package
		Push "${version}"
		Push "."
		Call StrTok
		Pop $originMajor
		Push "."
		Call StrTok
		Pop $originMinor
		Push "."
		Call StrTok
		Pop $originPoint
		Pop $originBuild
!ifndef MAVEN
		File /a "/oname=OriginUpdate_$originMajor_$originMinor_$originPoint_$originBuild.zip" "${EBISU_FILENAME}Update*.zip"
!else
		File /a "/oname=OriginUpdate_$originMajor_$originMinor_$originPoint_$originBuild.zip" "${EBISU_TARGETDIR}\${EBISU_FILENAME}Update*.zip"
!endif
	!endif

    ; Modify access rights to allow everyone access to some EADM files/folders
    Call ModifyAccessRights

    ; Migrate cache
    Call MigrateCache

    ; Register agents
    ; Note: Kept for parts but no longer registering agents during installer.
    ;       Now using relative paths and seeding EACore_App.ini with those configurations.
    ; Call RegisterAgentsManually

    SetOutPath "$INSTDIR"
    ; Register EADMLegacyCLI.exe as protocol handler for the ealink schema
    ExecWait '"$INSTDIR\OriginLegacyCLI.exe" -register'
    
    ; Write Core Registry Keys
    WriteRegStr HKLM "SOFTWARE\Electronic Arts\EA Core" "ClientVersion" "7.0.0.1"
    WriteRegStr HKLM "SOFTWARE\Electronic Arts\EA Core" "ClientPath" "$INSTDIR\${legacyCore}\OriginLegacyCLI.exe"
    WriteRegStr HKLM "SOFTWARE\Electronic Arts\EA Core" "ClientAccessDLLPath" "$INSTDIR\${legacyCore}\CmdPortalClient.dll"
    WriteRegStr HKLM "SOFTWARE\Electronic Arts\EA Core" "EADM6Version" "7.0.0.1"
    WriteRegStr HKLM "SOFTWARE\Electronic Arts\EA Core" "EADM6InstallDir" "$INSTDIR"

    ; Write EADM Registry Keys - to prevent over installation of older EADM clients
    WriteRegStr HKLM "SOFTWARE\Electronic Arts\EADM" "ClientVersion" ${version}
    WriteRegStr HKLM "SOFTWARE\Electronic Arts\EADM" "ClientPath" "$INSTDIR\${EBISU_FILENAME}.exe"

    ; Write Ebisu Registry Keys
    WriteRegStr HKLM "SOFTWARE\${productname}" "ClientVersion" ${version}
    WriteRegStr HKLM "SOFTWARE\${productname}" "ClientPath" "$INSTDIR\${EBISU_FILENAME}.exe"

    ;;;;;;;;;;; Windows Vista only - Start ;;;;;;;;;;;;;;
	; create the following registry key only for Windows Vista (major version = 6, minor version = 0)

	Push $0	
	Push $1	
	Push $2	
	Push $3	
	Push $4	

	System::Alloc 156
	Pop $0
	System::Call '*$0(i156)'
	System::Call kernel32::GetVersionExA(ir0)
	System::Call '*$0(i,&i4.r3,&i4.r4,i,i,&t128,&i2.r1,&i2.r2)'
	System::Free $0

	; major version 6
	IntCmp $3 6 0 notVista notVista
	; minor version 0
	IntCmp $4 0 0 notVista notVista

    ${If} ${RunningX64}
        SetRegView 64
    ${EndIf}
	WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Parental Controls\HTTPExemptions" "$INSTDIR\${EBISU_FILENAME}.exe" "$INSTDIR\${EBISU_FILENAME}.exe"
    ${If} ${RunningX64}
        SetRegView 32
    ${EndIf}

notVista:		
    Pop $4
    Pop $3
    Pop $2
    Pop $1
    Pop $0

    ;;;;;;;;;;; Windows Vista only - End ;;;;;;;;;;;;;;

    ; Prevent the tools from appearing in the recent menu and pinned in Win7   
    Push "MessageDlg.exe"
    Call HideAppFromRecentMenu

    Push "PatchProgress.exe"
    Call HideAppFromRecentMenu

    Push "Login.exe"
    Call HideAppFromRecentMenu

		;
		; Create keys to avoid EADM 4.x downgrading us
		;
		Call FakeEADM4Installation

    ;${If} ${RunningX64}
    ;    SetRegView 64
    ;${EndIf}
    
		; Add data for the Add Remove Programs Registry Entry
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayName" "${productname}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "Publisher" "Electronic Arts, Inc."
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${version}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\${uninstname}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\${uninstname}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "http://www.ea.com"
    WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoModify" 1
    WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoRepair" 1
    
    ; Reset to 32 bit registry
    ;${If} ${RunningX64}
    ;    SetRegView 32
    ;${EndIf}

	; check to see if we are being called from InstallThruEbisu
    ${GetParameters} $0
    ClearErrors
    ${GetOptions} $0 "/RunEADM" $1
	ifErrors checkOEM

checkOEM:
	
	; Check to see if we had any command line parameters telling us to add any OEM files
	StrCpy $0 "" ; clear
	${GetParameters} $0
	ClearErrors
	${GetOptions} $0 "/OEM=" $1
	IfErrors checkAutoupdate 0
	
	StrCpy $2 ""
	System::Call "shell32::SHGetFolderPathW(i 0, i ${CSIDL_COMMON_APPDATA}, i 0, i 0, w .r2) i.r0"
	
	StrCmp $2 "" checkAutoupdate
    StrCpy $2 "$2\Origin\OEM"
	CreateDirectory "$2"
	SetOutPath $2
	FileOpen $0 "origin.oem" w
	FileWrite $0 $1
	FileClose $0
	
checkAutoupdate:
	IfSilent noAutoupdate 0
	IntCmp $AutoupdateCheckResult 0 noAutoupdate
	WriteRegStr HKLM "SOFTWARE\${productname}" "Autoupdate" "true"
    WriteRegStr HKLM "SOFTWARE\${productname}" "AutopatchGlobal" "true"
	Goto checkTelemOO
	
noAutoupdate:
	WriteRegStr HKLM "SOFTWARE\${productname}" "Autoupdate" "false"
    WriteRegStr HKLM "SOFTWARE\${productname}" "AutopatchGlobal" "false"
	
checkTelemOO:
	IfSilent noTelemOO 0
	IntCmp $TelemOOCheckResult 0 noTelemOO
	WriteRegStr HKLM "SOFTWARE\${productname}" "TelemOO" "true"
	Goto icons
	
noTelemOO:
	WriteRegStr HKLM "SOFTWARE\${productname}" "TelemOO" "false"
	
icons:
    !insertmacro DebugMessageBox "AdditionalIcons"

    SetShellVarContext all
    
    ; $OUTDIR is used for the working directory of the shortcuts
    SetOutPath "$INSTDIR"
    
    Call GetSystemDefaultLanguageCode
    Pop $0
	
	
	
	IfSilent 0 +3
	StrCpy $StartCheckResult "1"
	StrCpy $DesktopCheckResult "1"
	
	; we need to recreate the shortcut if the lnk. exists or not. We don't want to create it if
	; the user doesn't have it checked.
  IfFileExists "$SMPROGRAMS\${productname}\${productname}.lnk" 0 checkShortcuts
		CreateShortCut "$SMPROGRAMS\${productname}\${productname}.lnk" "$INSTDIR\${EBISU_FILENAME}.exe"

checkShortcuts:		
	IntCmp $StartCheckResult 0 desktopShortcut
    CreateDirectory "$SMPROGRAMS\${productname}"
    CreateShortCut "$SMPROGRAMS\${productname}\$(EADM_MENU_UNINSTALL).lnk" "$INSTDIR\${uninstname}"
    CreateShortCut "$SMPROGRAMS\${productname}\$(EADM_MENU_OER).lnk" "$INSTDIR\OriginER.exe"
		CreateShortCut "$SMPROGRAMS\${productname}\${productname}.lnk" "$INSTDIR\${EBISU_FILENAME}.exe"
    
desktopShortcut:
	IntCmp $DesktopCheckResult 0 done
    CreateShortCut "$DESKTOP\${productname}.lnk" "$INSTDIR\${EBISU_FILENAME}.exe"
	
done:



    ;;;;;;;;;;; Windows 8 and above - only - Start ;;;;;;;;;;;;;;
	; remove the uninstall startmenu shortcut on Windows 8 (major version = 6, minor version = 2) or later, so it will not show up on the users home screen

	Push $0	
	Push $1	
	Push $2	
	Push $3	
	Push $4	

	System::Alloc 156
	Pop $0
	System::Call '*$0(i156)'
	System::Call kernel32::GetVersionExA(ir0)
	System::Call '*$0(i,&i4.r3,&i4.r4,i,i,&t128,&i2.r1,&i2.r2)'
	System::Free $0

	; major version 6
	IntCmp $3 6 0 notWin8 notWin8
	; minor version 2 or higher
	IntCmp $4 2 0 notWin8 0

    Delete "$SMPROGRAMS\${productname}\$(EADM_MENU_UNINSTALL).lnk"

notWin8:		
    Pop $4
    Pop $3
    Pop $2
    Pop $1
    Pop $0

    ;;;;;;;;;;; Windows 8 only - End ;;;;;;;;;;;;;;

    SetShellVarContext current
	
    ; IMPORTANT: MAKE SURE, THIS CALL COMES AT THE END OF THE INSTALLATION!!!
    ; Register EADM.exe as protocol handler for the eadm:// schema
    ExecWait '"$INSTDIR\${EBISU_FILENAME}.exe" /Register'
		
		; remove our helper DLL
		System::Call '${sysGetModuleHandle}("$TEMP\installerdll$tempDLL.dll" ) .r1'	; get the HMODULE handle and free it
		System::Call '${sysFreeLibrary}( r1 ) .r0'
		Delete "$TEMP\installerdll$tempDLL.dll"
	
!endif ;!INNER
SectionEnd

;----------------------------------------
; Run Ebisu after the installer finished
;----------------------------------------
Function RunAfterInstallation
	; the launcher will use this key to launch EADMUI after the installation

    ${GetParameters} $0
    ClearErrors
    ${GetOptions} $0 "/launcherTime=" $1
    IfErrors done 0
    WriteRegStr HKLM "SOFTWARE\${productname}" "Launch" "$1"
done:
FunctionEnd

;---------------------------------------
; Auto Run Ebisu after booting computer
;---------------------------------------
Function RunEADMAfterBootComputer
; Set autostart registry key for Origin
	ExecWait '"$INSTDIR\${EBISU_FILENAME}.exe" /SetAutoStart'
FunctionEnd 

Function DontRunEADMAfterBootComputer
; Remove autostart registry key for Origin
	SetShellVarContext current
	${DeleteRegValueX64} HKCU "SOFTWARE\Microsoft\Windows\CurrentVersion\Run" "EADM"
FunctionEnd 

;--------------------------------
; Uninstaller Section
;--------------------------------
!ifdef INNER
Function un.onInit

		StrCpy $isUiModified "false"
	
    ${GetParameters} $0

		; /S (uppercase is handled by NSIS)
		IfSilent endSilent

    ; /s option (uppercase is handled by NSIS)
    ClearErrors
    ${GetOptions} $0 "/s" $1
    IfErrors endSilent 0
    SetSilent silent

endSilent:
    ; Check that we are the only running instance
		Call un.CheckSingleInstance
    Pop $0
    IntCmp $0 "1" +2
    Abort     

    ; Check the user has admin privileges
    Call un.IsUserAdmin
    Pop $0
    IntCmp $0 1 endCheckAdmin
    
    MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONSTOP $(EADM_UNINSTALL_ADMIN_REQUIRED)
    Abort 

endCheckAdmin:

FunctionEnd

!define IDC_UNINSTWARN      1511

; -------------------------------------------------------------------------------
; un.onShowUinstFinishPage: This callback when the unistall finish page is shown
; -------------------------------------------------------------------------------
Function un.onShowUinstFinishPage
  	Call un.BringWindowToForeground
FunctionEnd

; -------------------------------------------------------------------------------
; In this callback we create a static to warn the user about uninstalling EA Core
; -------------------------------------------------------------------------------
Function un.onShowUninstConfirmPage
    
    ; Get the size of the window.
    System::Call "*${stRECT} .R0"
 
    ; Get the inner window rect and the DirectoryText rect. 
    ; We want to position our STATIC right below the DirectoryText control
    System::Call "user32::GetWindowRect(i $mui.UnConfirmPage, i R0)"
    System::Call "*$R0${stRECT} (.R1, .R2, , )"
    
    System::Call "user32::GetWindowRect(i $mui.UnConfirmPage.DirectoryText, i R0)"
    System::Call "*$R0${stRECT} (.r0, , , .r1)"
    
    ; We need relative to $mui.UnConfirmPage
    IntOp $0 $0 - $R1 
    IntOp $1 $1 - $R2
    
    System::Call "user32::GetClientRect(i $mui.UnConfirmPage, i R0)"
    System::Call "*$R0${stRECT} (, , .R1, .R2)"

    System::Free $R0
    
    ; $0: Left
    ; $1: Top
    ; $R1: Width
    ; $R2: Height   
    IntOp $1 $1 + 35
    IntOp $R1 $R1 - $0
    IntOp $R2 $R2 - $1
    IntOp $R2 $R2 - 5
        
    ; Create bitmap control.
    StrCpy $0 $(EADM_UNINSTALL_WARNING_BODY)
    System::Call 'user32::CreateWindowEx(i 0, t "STATIC", t r0, i ${WS_CHILD}|${WS_VISIBLE}, i r0, i r1, i R1, i R2, i $mui.UnConfirmPage, i ${IDC_UNINSTWARN}, i 0, i 0) i.R1'
    System::Call 'user32::SetWindowPos(i R1, i ${HWND_TOP}, i 0, i 0, i 0, i 0, i ${SWP_NOSIZE}|${SWP_NOMOVE})'

    ; Change text color to red
    SetCtlColors $R1 FF0000 "transparent"

    ; Set the default font and make it bold
    CreateFont $1 "$(^Font)" "$(^FontSize)" "700"
    SendMessage $R1 ${WM_SETFONT} $1 1   
   
    ; Default action is cancel
    StrCpy $0 $mui.Button.Cancel
    System::Call 'user32::SetFocus( i r0 )'
		SendMessage $HWNDPARENT ${WM_NEXTDLGCTL} $mui.Button.Cancel 1
  
		Call un.BringWindowToForeground
   
FunctionEnd

; -------------------------------------------------------------------------------
; This callback is called when the user hits the 'cancel' button after the uninstall 
; has failed (if it used the Abort command or otherwise failed).
; -------------------------------------------------------------------------------
Function un.onUninstFailed
        
FunctionEnd

; -------------------------------------------------------------------------------
; This callback is called when the uninstall was successful, right before the 
; install window closes (which may be after the user clicks 'Close' if SetAutoClose is set to false)..
; -------------------------------------------------------------------------------
Function un.onUninstSuccess

FunctionEnd

; -------------------------------------------------------------------------------
; Delete the cache folder
; Reads the configuration file and if the cache folder is defined (no temp cache) 
; It erases the whole directory.
; -------------------------------------------------------------------------------
Function un.deleteCacheFolder

    ; Read the configuration value
    Push "EACoreSession"
    Push "full_base_cache_dir"
    Call un.GetRuntimePreference
    Pop $0
    StrCmp $0 "" done

    !insertmacro DebugMessageBox "full_base_cache_dir: $0"

    ; If it ends with '\' then remove that (for GetParent to work properly)
    StrCpy $1 $0 "" -1
    StrCmp $1 "\" 0 +2
    StrCpy $0 $0 -1
    
    ; This is the way to check the existance and ensure it is a directory
     IfFileExists "$0\*.*" 0 done

    ; Check that the directory is not root
    StrLen $1 "$0"
    IntCmp $1 3 done done 0

      
		; delete files inside the cache folder, except .file_crcs
    Push $0		
		Call un.DeleteNonCRCFiles
  
    ; Finally remove the folder, if it is empty!!!
    RMDir $0

    ; Delete the parent folders as long as they are empty and named "EA Core" or "cache"
    Push $0
    Call un.GetParent
    Call un.DeleteEADMFolders
    
done:

FunctionEnd

; -------------------------------------------------------------------------------
; Delete the logs
; -------------------------------------------------------------------------------
Function un.deleteLogs

    ; Logs are located at:
    ; ${CSIDL_COMMON_APPDATA}/LogId + something.html
		StrCpy $1 ""
		System::Call "shell32::SHGetFolderPathW(i 0, i ${CSIDL_COMMON_APPDATA}, i 0, i 0, w .r1) i.r0"

    StrCmp $1 "" done

    StrCpy $1 "$1\Electronic Arts\EA Core\logs"
    
    IfFileExists "$1\*.*" 0 done     ; Check that the folder exists
    
    ; Read the configuration value for LogId
    Push "EACore"
    Push "LogId"
    Call un.GetRuntimePreference
    Pop $0
    StrCmp $0 "" done

    ; Iterate and delete
    FindFirst $2 $3 "$1\$0*.html"
    
loop:
    StrCmp $3 "" doneLoop
    Delete "$1\$3"
    FindNext $2 $3
    Goto loop

doneLoop:
    FindClose $2

	; If the only remaining file is EALogReader.html then remove the directory
    FindFirst $2 $3 "$1\*.*"
	
loop2:
	StrCmp $3 "" removeDir 0
    StrCmp $3 "." next 0
	StrCmp $3 ".." next 0
	StrCmp $3 "logReader.html" next 0
	StrCmp $3 "EALogReader.html" next 0
	Goto doneLoop2
	
next:
    FindNext $2 $3
    Goto loop2

doneLoop2:
	; Some other files were found
	FindClose $2
	Goto done

removeDir:
	FindClose $2
	RMDir /r $1
	
    ; Delete the parent folders \Electronic Arts\EA Core\ if empty
    Push $1
    Call un.GetParent
    Pop $1
	
	ClearErrors
	RMDir $1
	IfErrors done
	
	Push $1
    Call un.GetParent
    Pop $1
	RMDir $1
	
done:

FunctionEnd


; -------------------------------------------------------------------------------
; Delete EADM data folders
; -------------------------------------------------------------------------------
Function un.deleteEADMDataFolder
		
		; move out, so that we do not block a folder
		SetOutPath "$TEMP"

    ; Origin default DIP folder
    ; Program Files\Origin Games\
		StrCpy $1 ""
		System::Call "shell32::SHGetFolderPathW(i 0, i ${CSIDL_PROGRAM_FILES}, i 0, i 0, w .r1) i.r0"
    StrCmp $1 "" done1
    StrCpy $0 "$1\Origin Games"
    RMDir $0	; only if it is empty

done1:
		; Origin non default DIP folder
    ; Read the configuration value
    Push "EACore"
    Push "base_install_dir"
    Call un.GetRuntimePreference
    Pop $0
    StrCmp $0 "" done2
    RMDir $0	; only if it is empty
    
done2:    
    ; legacy folders
    ; ${CSIDL_LOCAL_APPDATA}\Electronic Arts
		StrCpy $1 ""
		System::Call "shell32::SHGetFolderPathW(i 0, i ${CSIDL_LOCAL_APPDATA}, i 0, i 0, w .r1) i.r0"
    StrCmp $1 "" done3
    StrCpy $1 "$1\Electronic Arts"
		;remove legacy folders
		RMDir /r "$1\EADM"
		RMDir /r "$1\EA Core"
		RMDir /r "$1\Web Cache"
		RMDir /r "$1\EADMUI"
		; remove the Electronic Arts folder, if it is empty
		RMDir $1

done3:		
    ; legacy folders
    ; ${CSIDL_COMMON_APPDATA}\Electronic Arts
		StrCpy $1 ""
		System::Call "shell32::SHGetFolderPathW(i 0, i ${CSIDL_COMMON_APPDATA}, i 0, i 0, w .r1) i.r0"
    StrCmp $1 "" done4a
    StrCpy $1 "$1\Electronic Arts"
		;remove legacy folders
		RMDir /r "$1\EADM"
		RMDir /r "$1\EADMUI"
		RMDir "$1\EA Core"
		; remove the Electronic Arts folder, if it is empty
		RMDir $1

done4a:		
		; Logs are located at:
    ; ${CSIDL_COMMON_APPDATA}\Origin\Logs
		StrCpy $1 ""
		System::Call "shell32::SHGetFolderPathW(i 0, i ${CSIDL_COMMON_APPDATA}, i 0, i 0, w .r1) i.r0"
    StrCmp $1 "" done4b
    StrCpy $1 "$1\Origin\Logs"   
		RMDir /r $1
done4b:		
		; BoxArt is located at:
    ; ${CSIDL_COMMON_APPDATA}\Origin\BoxArt
		StrCpy $1 ""
		System::Call "shell32::SHGetFolderPathW(i 0, i ${CSIDL_COMMON_APPDATA}, i 0, i 0, w .r1) i.r0"
    StrCmp $1 "" done4c
    StrCpy $1 "$1\Origin\BoxArt"   
		RMDir /r $1
done4c:		
		; AvatarsCache is located at:
    ; ${CSIDL_COMMON_APPDATA}\Origin\AvatarsCache
		StrCpy $1 ""
		System::Call "shell32::SHGetFolderPathW(i 0, i ${CSIDL_COMMON_APPDATA}, i 0, i 0, w .r1) i.r0"
    StrCmp $1 "" done4d
    StrCpy $1 "$1\Origin\AvatarsCache"   
		RMDir /r $1
done4d:		
		; Telemetry is located at:
    ; ${CSIDL_COMMON_APPDATA}\Origin\Telemetry
		StrCpy $1 ""
		System::Call "shell32::SHGetFolderPathW(i 0, i ${CSIDL_COMMON_APPDATA}, i 0, i 0, w .r1) i.r0"
    StrCmp $1 "" done4e
    StrCpy $1 "$1\Origin\Telemetry"   
		RMDir /r $1
done4e:		
		; delete files left over files like origin_app.ini / installedgames.xml:
    ; ${CSIDL_COMMON_APPDATA}\Origin\*.*
		StrCpy $1 ""
		System::Call "shell32::SHGetFolderPathW(i 0, i ${CSIDL_COMMON_APPDATA}, i 0, i 0, w .r1) i.r0"
    StrCmp $1 "" done4f
		Delete "$1\Origin\*.*"

done4f:		
		; delete files inside the cache folder, except .file_crcs
    ; ${CSIDL_COMMON_APPDATA}\Origin\DownloadCache
		StrCpy $1 ""
		System::Call "shell32::SHGetFolderPathW(i 0, i ${CSIDL_COMMON_APPDATA}, i 0, i 0, w .r1) i.r0"
    StrCmp $1 "" done4g
		push "$1\Origin\DownloadCache"
		Call un.DeleteNonCRCFiles

done4g:		
		; Entitlement Cache is located at:
    ; ${CSIDL_COMMON_APPDATA}\Origin\EntitlementCache
		StrCpy $1 ""
		System::Call "shell32::SHGetFolderPathW(i 0, i ${CSIDL_COMMON_APPDATA}, i 0, i 0, w .r1) i.r0"
    StrCmp $1 "" done5
    StrCpy $1 "$1\Origin\EntitlementCache"   
		RMDir /r $1		
		
done5:
    ; web cache is located at:
    ; ${CSIDL_LOCAL_APPDATA}\Origin
		StrCpy $1 ""
		System::Call "shell32::SHGetFolderPathW(i 0, i ${CSIDL_LOCAL_APPDATA}, i 0, i 0, w .r1) i.r0"
    StrCmp $1 "" done6
    StrCpy $1 "$1\Origin"    
		RMDir /r $1

done6:    

    ; conversation logs are located at
    ; ${CSIDL_APPDATA}\Origin\Conversation Logs
		StrCpy $1 ""
		System::Call "shell32::SHGetFolderPathW(i 0, i ${CSIDL_APPDATA}, i 0, i 0, w .r1) i.r0"
    StrCmp $1 "" done7
    StrCpy $1 "$1\Origin\Conversation Logs"    
		RMDir /r $1

done7:

FunctionEnd

; -------------------------------------------------------------------------------
; Delete the logs
; -------------------------------------------------------------------------------
Function un.deleteRuntimePreferences

    ; Delete the runtime preferences file
    Call un.GetRuntimePreferencesFileName
    Pop $0
    StrCmp $0 "" done

    IfFileExists $0 0 +2
    Delete $0

    ; Delete the parent folders "\Electronic Arts\EA Core\prefs\"
    Push $0
    Call un.GetParent
    Pop $0
    
    Push $0
    Call un.DeleteEADMFolders

done:

FunctionEnd

; -------------------------------------------------------------------------------
; Uninstaller main section
; -------------------------------------------------------------------------------
Section "Uninstall"
    
		; generate random part for the dll name	
		StrCpy $0 ""
		System::Call 'kernel32::GetTickCount()i .r0'
		StrCpy $tempDLL $0

		; Extract our plugin dll
    File /nonfatal /a "/oname=$TEMP\installerdll$tempDLL.dll" "${INSTALLER_DLLDIR}\installerdll.dll"

		; See if the parent process is belonging to EADM, then we run as an update and won't show some warnings 
    SetOutPath "$TEMP"
	System::Call 'installerdll$tempDLL::IsUpdate() i.r1'

    ; Remove trailing backslash
    StrCpy $0 $INSTDIR "" -1
    StrCmp $0 "\" 0 +2
    StrCpy $INSTDIR $INSTDIR -1

	SetOutPath "$INSTDIR"

    ; Show warning window, if the UI or EACoreServer is/are running
    Push $1
		Call un.CheckForRunningInstances

    ; Kill any running instance
    Call un.KillRunningCoreServer

    ; check for locked IGO files
    Call un.CheckForLockedIGOFiles
    
unpin:
    !define COINIT_APARTMENTTHREADED 0x2
    !define CLSCTX_INPROC_SERVER 0x1
    !define IID_IShellItem {43826D1E-E718-42EE-BC55-A1E261C37BFE}
    !define CLSID_StartMenuPin {A2A9545D-A0C2-42B4-9708-A0B2BADD77C8}
    !define IID_IStartMenuPinnedList {4CD19ADA-25A5-4A32-B3B7-347BEE5BE36B}
	
    System::Call `ole32::CoInitializeEx(n,i${COINIT_APARTMENTTHREADED})i.r0`
    ${If} $0 >= 0
	#IShellItem *pitem, 
	System::Call `shell32::SHCreateItemFromParsingName(w "$INSTDIR\${EBISU_FILENAME}.exe",n,g"${IID_IShellItem}",*i.R0)i.r0`
	#Do setup work here to remove the link, including the unpinning of the item.
	${If} $0 >= 0
	  #IStartMenuPinnedList *pStartMenuPinnedList;
	  System::Call `ole32::CoCreateInstance(g"${CLSID_StartMenuPin}",n,i${CLSCTX_INPROC_SERVER},g"${IID_IStartMenuPinnedList}",*i.R1)i.r0`
	  ${If} $0 >= 0
	    #`pStartMenuPinnedList->RemoveFromList(pitem)`
	    System::Call `$R1->3(iR0)i.r0`
	    #`pStartMenuPinnedList->Release()`
	    System::Call `$R1->2()`
	  ${EndIf}
	  #`pitem->Release()`
	  System::Call `$R0->2()`
	${EndIf}
    ${EndIf}
    System::Call `ole32::CoUninitialize()`
    
waitFini:
	; create our uninstall telemtry event
    ExecWait '"$INSTDIR\${EBISU_FILENAME}.exe" /noUpdate /Uninstall'

	; just to be sure!!
    ; Kill any running instance
    Call un.KillRunningCoreServer

	; uninstall the Origin client service on any OS (just as a precaution!)
	ExecWait '"$INSTDIR\OriginClientService.exe" /nsisuninstall'
    
    ; uninstall the Origin responder service on any OS (just as a precaution!)
	ExecWait '"$INSTDIR\OriginWebHelperService.exe" /nsisuninstall'
	
	; just to be sure!!
    ; Kill any running instance
    Call un.KillRunningCoreServer
	
    ; Set counter
    IntOp $0 10 + 0
        
    Delete "$INSTDIR\EACore.dll"
    Delete "$INSTDIR\CoreCmdPortalClient.dll"
    Delete "$INSTDIR\CmdPortalClient.dll"
    Delete "$INSTDIR\QtCore4.dll"
		Delete "$INSTDIR\EADM.exe"
    IfFileExists "$INSTDIR\EACore.dll" 0 uninstallAirComponents

    Sleep 500
        
    ; Count down the repeat 
    IntOp $0 $0 - 1
    
    ; Continue if we looped enough already
    IntCmp $0 0 0 0 waitFini

uninstallAirComponents:

		; first try
    ExecWait '"$INSTDIR\EADM6AirComponentsUninstall.exe" /S'
    Delete "$INSTDIR\EADM6AirComponentsUninstall.exe"
		
		; second try - just to be sure
    ; remove the AIR UI compponent before doing a check for an existing installed EADM 6, because most
    ; traces would have been removed by a previous uninstall, except the "EA Download Manager UI"
    ; Get the current uninstaller for the AIR UI component
    Push "EA Download Manager UI"
    Call un.GetUninstallString
    Pop $2
    ; Add silent uninstallation
    StrCpy $2 "$2 /quiet"
    ; Finally, execute the uninstaller
    ExecWait $2 $0
    !insertmacro DebugMessageBox "Previous version uninstalled: $0"
  
deleteFiles:

		; we need the settings.xml to find the file_crcs when a new Origin is installed, so do not delete it !!!
		;SetOutPath "$TEMP"
		;System::Call 'installerdll$tempDLL::CleanUserConfigDir()'
		;SetOutPath "$INSTDIR"
	
    ; Delete the cache folder
    Call un.deleteCacheFolder

    ; Delete EADM data folder
    Call un.deleteEADMDataFolder

    ; Delete the logs
    Call un.deleteLogs

    ; Delete the runtime preferences file
    Call un.deleteRuntimePreferences
    
    ; Delete all installed files
    Delete "$INSTDIR\3RDPARTYLICENSES.HTML"
    Delete "$INSTDIR\3RDPARTYLICENSES_FR.HTML"
	Delete "$INSTDIR\${EBISU_FILENAME}Client.dll"
    Delete "$INSTDIR\${EBISU_FILENAME}.exe"
    Delete "$INSTDIR\OriginER.exe"
    Delete "$INSTDIR\EADM.exe"
    Delete "$INSTDIR\CoreCmdPortalClient.dll"
    Delete "$INSTDIR\CmdPortalClient.dll"   	
    Delete "$INSTDIR\CmdPortalTester.exe" 
    Delete "$INSTDIR\Telemetry.dll"
    Delete "$INSTDIR\EACore.dll"
    Delete "$INSTDIR\EACore.ini"
    Delete "$INSTDIR\EACore.ini.bak"
    Delete "$INSTDIR\EACore_App.ini"
    Delete "$INSTDIR\Origin_App.ini"
    Delete "$INSTDIR\OriginLegacyCLI.exe"
    Delete "$INSTDIR\EACoreServer.exe"
    Delete "$INSTDIR\EAProxyInstaller.exe"
    Delete "$INSTDIR\OriginClientService.exe"
    Delete "$INSTDIR\OriginWebHelperService.exe"
    Delete "$INSTDIR\logDataTemplate.html" 			; No longer used.. remove
    Delete "$INSTDIR\logReader.html"
    Delete "$INSTDIR\EntResp"						; Debug only server response
    Delete "$INSTDIR\Error.log"						; Debug only
	Delete "$INSTDIR\OriginUpdate.json"				; Debug only

    Delete "$INSTDIR\QtCore4.dll"
    Delete "$INSTDIR\QtGui4.dll"
    Delete "$INSTDIR\QtNetwork4.dll"
    Delete "$INSTDIR\QtXml4.dll"
    Delete "$INSTDIR\QtXmlPatterns4.dll"
    Delete "$INSTDIR\QtWebKit4.dll"
    Delete "$INSTDIR\phonon4.dll"
    Delete "$INSTDIR\QtTest4.dll"
	Delete "$INSTDIR\QtMultimediaKit1.dll"
	Delete "$INSTDIR\QtOpenGL4.dll"
	Delete "$INSTDIR\QtSensors1.dll"

	; Qt5 files
    Delete "$INSTDIR\Qt5*.dll"

    Delete "$INSTDIR\winhttp.dll"

    Delete "$INSTDIR\InstallerDLL.dll"
    
    Delete "$INSTDIR\libeay32.dll"
    Delete "$INSTDIR\ssleay32.dll"
    
    Delete "$INSTDIR\Tufao.dll"
    
    Delete "$INSTDIR\icudt51.dll"
    Delete "$INSTDIR\icuin51.dll"
    Delete "$INSTDIR\icuuc51.dll"

    Delete "$INSTDIR\Microsoft.VC80.CRT.manifest"
    Delete "$INSTDIR\msvcp80.dll"
    Delete "$INSTDIR\msvcr80.dll"
    Delete "$INSTDIR\Origin_EULA.rtf"
    
    ; remove core legacy files
    RMDir /r "$INSTDIR\${legacyCore}"

    Delete "$INSTDIR\Login.exe"
    Delete "$INSTDIR\MessageDlg.exe"
    Delete "$INSTDIR\PatchProgress.exe"

    ; Remove launch link
    Delete "$INSTDIR\${productname}.lnk"
    
    ; Delete codecs dir
    Delete "$INSTDIR\codecs\*.dll"
    Delete "$INSTDIR\codecs\*.manifest"
    RMDir "$INSTDIR\codecs"
    
    ; Delete imageformats dir
    Delete "$INSTDIR\imageformats\*.dll"
    Delete "$INSTDIR\imageformats\*.manifest"
    RMDir "$INSTDIR\imageformats"

    ; Delete platforms dir
    Delete "$INSTDIR\platforms\*.dll"
    Delete "$INSTDIR\platforms\*.manifest"
    RMDir "$INSTDIR\platforms"

	; Delete audio plugins dir
    Delete "$INSTDIR\audio\*.dll"
    RMDir "$INSTDIR\audio"
	
    ; Delete printersupport dir
    Delete "$INSTDIR\printsupport\*.dll"
    RMDir "$INSTDIR\printsupport"

    ; Delete mediaservice dir
    Delete "$INSTDIR\mediaservice\*.dll"
    RMDir "$INSTDIR\mediaservice"

	; Delete sounds dir
	Delete "$INSTDIR\sounds\*.wav"
	RMDir "$INSTDIR\sounds"
	
    ; Delete lang dir
    Delete "$INSTDIR\lang\*.xml"
    RMDir "$INSTDIR\lang"

    ; Delete plugins dir
    RMDir /r "$INSTDIR\plugins"
    
    ; Delete x64 dir
    Delete "$INSTDIR\x64\*.*"
    RMDir "$INSTDIR\x64"
    
		; Delete IGO files
		Delete "$INSTDIR\igoproxy.exe"
		Delete "$INSTDIR\igo32.dll"
		Delete "$INSTDIR\igoproxy64.exe"
		Delete "$INSTDIR\igo64.dll"

		; Delete TWITCH files
		Delete "$INSTDIR\twitchsdk_32_Debug.dll"
		Delete "$INSTDIR\twitchsdk_32_Release.dll"
		Delete "$INSTDIR\twitch32.dll"
		Delete "$INSTDIR\avcodec-54.dll"
		Delete "$INSTDIR\avformat-54.dll"
		Delete "$INSTDIR\avutil-51.dll"
		Delete "$INSTDIR\curl-ca-bundle.crt"
		Delete "$INSTDIR\libcurl.dll"
		Delete "$INSTDIR\libmfxsw32.dll"
		Delete "$INSTDIR\libmp3lame-0.dll"
		Delete "$INSTDIR\libmp3lame-ttv.dll"
		Delete "$INSTDIR\swresample-0.dll"
		Delete "$INSTDIR\avutil-ttv-51.dll"
		Delete "$INSTDIR\swresample-ttv-0.dll"
		
		; Delete old IGO files
		Delete "$INSTDIR\APIHook32.dll"
		Delete "$INSTDIR\APIHook64.dll"
		Delete "$INSTDIR\data.dat"
		Delete "$INSTDIR\dbghelp32.dll"
		Delete "$INSTDIR\dbghelp64.dll"
		Delete "$INSTDIR\DirectX10GUIRenderer32.dll"
		Delete "$INSTDIR\DirectX10GUIRenderer64.dll"
		Delete "$INSTDIR\DirectX11GUIRenderer32.dll"
		Delete "$INSTDIR\DirectX11GUIRenderer64.dll"
		Delete "$INSTDIR\DirectX81GUIRenderer32.dll"
		Delete "$INSTDIR\DirectX9GUIRenderer32.dll"
		Delete "$INSTDIR\DirectX9GUIRenderer64.dll"
		Delete "$INSTDIR\EADM_IGO_driver32.sys"
		Delete "$INSTDIR\EADM_IGO_driver64.sys"
		Delete "$INSTDIR\HlxOvrly32.exe"
		Delete "$INSTDIR\HlxOvrly64.exe"
		Delete "$INSTDIR\IGOClient32.dll"
		Delete "$INSTDIR\IGOClient64.dll"
		Delete "$INSTDIR\IGODX10Helper32.dll"
		Delete "$INSTDIR\IGODX10Helper64.dll"
		Delete "$INSTDIR\IGODX11Helper32.dll"
		Delete "$INSTDIR\IGODX11Helper64.dll"
		Delete "$INSTDIR\IGODX8Helper32.dll"
		Delete "$INSTDIR\IGODX9Helper32.dll"
		Delete "$INSTDIR\IGODX9Helper64.dll"
		Delete "$INSTDIR\IGOGUI32.dll"
		Delete "$INSTDIR\IGOGUI64.dll"
		Delete "$INSTDIR\IGOOPENGLHelper32.dll"
		Delete "$INSTDIR\IGOOPENGLHelper64.dll"
		Delete "$INSTDIR\IGO_dx1032.dll"
		Delete "$INSTDIR\IGO_dx1064.dll"
		Delete "$INSTDIR\IGO_dx1132.dll"
		Delete "$INSTDIR\IGO_dx1164.dll"
		Delete "$INSTDIR\IGO_dx832.dll"
		Delete "$INSTDIR\IGO_dx932.dll"
		Delete "$INSTDIR\IGO_dx964.dll"
		Delete "$INSTDIR\IGO_gl32.dll"
		Delete "$INSTDIR\IGO_gl64.dll"
		Delete "$INSTDIR\IGO_QT4Browser.exe"
		Delete "$INSTDIR\IGO_sync32.dll"
		Delete "$INSTDIR\IGO_sync64.dll"
		Delete "$INSTDIR\KillHlxOvrly.exe"
		Delete "$INSTDIR\OpenGLGUIRenderer32.dll"
		Delete "$INSTDIR\OpenGLGUIRenderer64.dll"
		Delete "$INSTDIR\StandaloneMsgWindow32.exe"
		Delete "$INSTDIR\StandaloneMsgWindow64.exe"
		
    ; Delete the Origin Config service .wad files
    Delete "$INSTDIR\*.wad"

	; Delete the update package
	Delete "$INSTDIR\OriginUpdate*.zip"
	
    ; Delete the Origin Crash Reporter
    Delete "$INSTDIR\OriginCrashReporter.exe"

    ; Delete the Origin ER
    Delete "$INSTDIR\OriginER.exe"

    ; Delete the Telemetry Breakpoint Handler
    Delete "$INSTDIR\TelemetryBreakpointHandler.exe"

	; Delete the update tool
	Delete "$INSTDIR\UpdateTool.exe"
	
		;Delete the VisualElementsFiles.
		Delete "$INSTDIR\*.VisualElementsManifest.xml"
			
            
    ; Delete the vc redist executables
    Delete "$INSTDIR\vcredist_*.exe"
    
    ; Delete the uninstaller itself
    Delete "$INSTDIR\${uninstname}"

    ; Move out (sets current directory) and try to remove
    ; the installation directory (will only work if it is empty)
    SetOutPath "$TEMP"
    RMDir "$INSTDIR"
    
    SetShellVarContext all

    ; Remove start menu folder
    Delete "$SMPROGRAMS\${productname}\${productname}.lnk"
    Delete "$SMPROGRAMS\${productname}\$(EADM_MENU_OER).lnk"
    Delete "$SMPROGRAMS\${productname}\$(EADM_MENU_UNINSTALL).lnk"
    Delete "$SMPROGRAMS\${productname}\$(EADM_MENU_HELP).url"

    Push  "$SMPROGRAMS\${productname}"
    Call un.DeleteEmptyFolders

endStartMenu:
    ; Remove EADM exe (undoes HideAppFromRecentMenu)
    Push "${EBISU_FILENAME}.exe"
    Call un.RemoveRegisteredApp
    
    ; Remove our fake key
    Call un.RemoveFakeEADM4Installation

    ; Remove Desktop Shortcut
    Delete "$DESKTOP\${productname}.lnk"

    SetShellVarContext current

    ; Remove the Add/Remove programs entry
    ${un.DeleteRegKeyX64} HKLM "${PRODUCT_UNINST_KEY}"
        
    ; Remove Application registry entries
    ${un.DeleteRegValueX64} HKLM "SOFTWARE\Electronic Arts\EA Core" "ClientVersion"
    ${un.DeleteRegValueX64} HKLM "SOFTWARE\Electronic Arts\EA Core" "ClientPath"
    ${un.DeleteRegValueX64} HKLM "SOFTWARE\Electronic Arts\EA Core" "ClientAccessDLLPath"
    ${un.DeleteRegValueX64} HKLM "SOFTWARE\Electronic Arts\EA Core" "EADM6Version"
    ${un.DeleteRegValueX64} HKLM "SOFTWARE\Electronic Arts\EA Core" "EADM6InstallDir"
    
    ${un.DeleteRegKeyIfEmptyX64} HKLM "SOFTWARE\Electronic Arts\EA Core"
    
    ${un.DeleteRegValueX64} HKLM "SOFTWARE\Electronic Arts\EADM" "ClientVersion"
    ${un.DeleteRegValueX64} HKLM "SOFTWARE\Electronic Arts\EADM" "ClientPath"
    ${un.DeleteRegValueX64} HKLM "SOFTWARE\Electronic Arts\EADM" "Launch"

    ${un.DeleteRegValueX64} HKLM "SOFTWARE\${productname}" "ClientVersion"
    ${un.DeleteRegValueX64} HKLM "SOFTWARE\${productname}" "ClientPath"
    ${un.DeleteRegValueX64} HKLM "SOFTWARE\${productname}" "Launch"
	${un.DeleteRegValueX64} HKLM "SOFTWARE\${productname}" "InstallSuccesfull"
    ${un.DeleteRegValueX64} HKCU "SOFTWARE\${productname}" "Launch"
	${un.DeleteRegValueX64} HKCU "SOFTWARE\${productname}" "InstallSuccesfull"

	; it is safe to try to remove a reg key that does not exist this exists only in Windows Vista
    ${If} ${RunningX64}
        SetRegView 64
    ${EndIf}
    ${un.DeleteRegValueX64} HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Parental Controls\HTTPExemptions" "$INSTDIR\${EBISU_FILENAME}.exe"
    ${If} ${RunningX64}
        SetRegView 32
    ${EndIf}

    ${un.DeleteRegKeyIfEmptyX64} HKLM "SOFTWARE\${productname}"

    ${un.DeleteRegKeyIfEmptyX64} HKLM "SOFTWARE\Electronic Arts"
	
    ${un.DeleteRegKeyX64} HKCR "ebisu"
    ${un.DeleteRegKeyX64} HKCR "eadm"
    ${un.DeleteRegKeyX64} HKCR "ealink"
    ${un.DeleteRegKeyX64} HKCR "origin"
    ${un.DeleteRegKeyX64} HKCR "origin2"
	
    ${un.DeleteRegValueX64} HKCU "SOFTWARE\Microsoft\Windows\CurrentVersion\Run" "EADM"
    ${un.DeleteRegValueX64} HKCU "SOFTWARE\Microsoft\Windows\CurrentVersion\Run" "Origin"
		
    Call un.BringWindowToForeground

		; remove our helper DLL
		System::Call '${sysGetModuleHandle}("$TEMP\installerdll$tempDLL.dll" ) .r1'	; get the HMODULE handle and free it
		System::Call '${sysFreeLibrary}( r1 ) .r0'
		Delete "$TEMP\installerdll$tempDLL.dll"   

SectionEnd
!endif ;INNER

