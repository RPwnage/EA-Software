;--------------------------------
;General
;--------------------------------
XPStyle on

;Request application privileges for Windows Vista
RequestExecutionLevel user	; never launch this as admin !!!
SetCompressor LZMA
SilentInstall silent

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

;--------------------------------
; Components configuration
;--------------------------------

; Maven has its own directory structure so adjust that
!ifndef MAVEN
    !define EBISU_RUNTIMEDIR "..\runtime"
    !define CORE_RUNTIMEDIR "..\core\CoreService\runtime"
    !define SRC_PATCHPROGRESSDIR "..\PatchProgress"
    !define SRC_ACCESSDIR "..\Access"
    !define SRC_EAACCESSGAMESDIR "..\EAAccessGames"
    !define SRC_AWCDIR "..\AccessDRM\Output"
    !define SRC_EADM6DIR "..\Vault"
    !define INSTALLER_DLLDIR ".\InstallerDLL\Release"
	!define QT_LICDIR "..\..\..\..\Qt\4.7.3\Rebranded EA License"
	!define QT_DLLSDIR "..\..\..\..\Qt\4.7.3\bin"
	!define QT_WEBKITDIR "..\..\..\..\qtwebkit-2.1.0\bin"
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

; Get installation folder from registry if available
InstallDirRegKey HKLM "SOFTWARE\Origin" "ClientPath"
  
;Text that goes grayed at the start of the div line 
;defaults to "Nullsoft Install System v2.xx"
BrandingText "${productname} ${version}"

!ifdef outfile
	OutFile "${outfile}"
!else
	OutFile "Origin-installer_${version}.exe"
	!define outfile "Origin-installer_${version}.exe"
!endif

!define uninstname "OriginUninstall.exe"

;--------------------------------
;Variables
;--------------------------------
Var tempName
Var firstTimeInstall
Var newerVersion
Var runEADMfound
Var forceinstallfound

;--------------------------------
;Interface Settings
;--------------------------------
!define MUI_ICON "resources\App.ico"
!define MUI_UNICON "resources\Uninstall.ico"
!define MUI_ABORTWARNING

!define PRODUCT_UNINST_KEY       "Software\Microsoft\Windows\CurrentVersion\Uninstall\${productname}"
!define PRODUCT_UNINST_ROOT_KEY  "HKLM"
!define PRODUCT_STARTMENU_REGVAL "NSIS:StartMenuDir"


;--------------------------------
; Pages
;--------------------------------
; none

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

;--------------------------------
; Parse the command line for backward compat
;--------------------------------
Function .onInit

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

FunctionEnd

!define ERROR_ALREADY_EXISTS    183

;------------------------------------------------------------------------------
; CheckSingleInstance
; Check that only one installer/uninstaller instance is running at a time
; If another running instance is detected bring it to the front
;------------------------------------------------------------------------------
!macro FUNC_CheckSingleInstance un
Function ${un}CheckSingleInstance

    ; Create Origin mutex
    ; we cannot use the old EADM::Installer mutex, because then we cannot run the old uinstaller!
    System::Call "kernel32::CreateMutex(i 0, i 0, t 'ORIGIN::InstallerLauncher') i .r0 ?e"
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

Section "Main Program" SecMain

	SetSilent normal	; do not set the installer silten per default, because then we miss the dialog boxes!

	; option /s
	StrCpy $0 "" ; clear
	${GetParameters} $0
	ClearErrors
	${GetOptions} $0 "/s" $1
	IfErrors endSilent 0
	SetSilent silent

	; option /S
	StrCpy $0 "" ; clear
	${GetParameters} $0
	ClearErrors
	${GetOptions} $0 "/S" $1
	IfErrors endSilent 0
	SetSilent silent

endSilent:

	; generate random part for the dll name	
	StrCpy $0 ""
	System::Call 'kernel32::GetTickCount()i .r0'
	StrCpy $tempName $0

	; Extract our plugin dll
	File /a "/oname=$TEMP\installerdll$tempName.dll" "${INSTALLER_DLLDIR}\installerdll.dll"

	; *****************************
	; launch the installer
	; *****************************
	StrCpy $firstTimeInstall ""		; clear for saving out first time EADM install
	StrCpy $newerVersion ""				; clear for saving out newer version
	StrCpy $runEADMfound ""				; clear for saving out our "run after install" status of Origin
	StrCpy $forceinstallfound ""	; clear  /forceinstall flags
	
	SetOutPath "$TEMP"
	; check if this is an update (is EAProxyInstaller our parent?)
	System::Call 'installerdll$tempName::IsUpdate() i.r1'
	IntCmp $1 0 noUpdate
	; append command line switch to indicate an update installation(this will silence certain checks in the installer)
	StrCpy $3 "$3 /update"
	
	; move this installer to temp (if it is not already there) and start it from there, in order to not lock download folders
	
	Push $EXEPATH
	Push $TEMP
	Call StrStr
	Pop $1
	StrCmp $1 "" 0 noUpdate

	StrCpy $0 "$EXEPATH" ;src
	StrCpy $1 "$TEMP\OriginLauncher$tempName.exe"   ;dest
	StrCpy $2 0 ; only 0 or 1, set 0 to overwrite file if it already exists
	System::Call "kernel32::CopyFile(t r0, t r1, b r2) ?e"
	Pop $0 ; pops a bool

	System::Call 'installerdll$tempName::RunProcess(w "$TEMP\OriginLauncher$tempName.exe", w "$TEMP", i 0, w r3)'
	
	; remove our helper DLL
	System::Call '${sysGetModuleHandle}("$TEMP\installerdll$tempName.dll" ) .r1'	; get the HMODULE handle and free it
	System::Call '${sysFreeLibrary}( r1 ) .r0'
	Delete "$TEMP\installerdll$tempName.dll"

	Quit
	
noUpdate:

	StrCpy $3 "" ; clear
	${GetParameters} $3 ; get the command line parameters for the installer
	; add our internal launcherTime parameter used for launching the Origin client after the installation
	StrCpy $3 "$3 /launcherTime="
	StrCpy $3 "$3$tempname"

    ; Check that we are the only running instance
		Call CheckSingleInstance
    Pop $0
    IntCmp $0 "1" +2
    Abort     
    
    ; We can only upgrade from EADM6
    Call GetEADMInstalledVersionMajor
    Pop $0
    
    IntCmp $0 0 cont                 ; No version currently installed
    IntCmp $0 4 cont incompat cont   ; Continue only if version is equal or greater that 4
    
incompat:

	MessageBox MB_OK|MB_TOPMOST|MB_SETFOREGROUND $(EADM_INCOMPATIBLE_VERSION_INSTALLED)
    Goto notInstalled

cont:

	; check to see if we are being called from InstallThruEbisu
	StrCpy $0 "" ; clear
	${GetParameters} $0
	ClearErrors
	${GetOptions} $0 "/RunEADM" $1
	IfErrors norunEADM 0
	StrCpy $runEADMfound "1"		; we want to run Origin, because /RunEADM was specified
norunEADM:		

	; check to see if /forceinstall was specified
	StrCpy $0 "" ; clear
	${GetParameters} $0
	ClearErrors
	${GetOptions} $0 "/forceinstall" $1
	IfErrors noforceinstall 0
	StrCpy $forceinstallfound "1"
noforceinstall:

    ; Find a previous version of Origin
    ReadRegStr $0 HKLM "SOFTWARE\${productname}" "ClientVersion"

    ; Check that the key really exists and has a valid value
    IfErrors noprevinstall
    StrCmp $0 "" noprevinstall

    Push $0
    Push ${version}
    Call VersionCompare
    Pop $0

    ; $0 = 0  Versions are equal
    ; $0 = 1  Version1 is newer
    ; $0 = 2  Version2 is older
    IntCmp $0 0 done
    IntCmp $0 1 query_overwrite_newer


	StrCpy $newerVersion "1"		;installing version is newer
    goto install

noprevinstall:
	Strcpy $firstTimeInstall "1"		;first time install
	goto install

query_overwrite_newer:
	
	StrCmp $forceinstallfound "1" allowOldInstallSilently	; check if /forceinstall was specified
	
	; if called via ITE we do not allow to install an older verision
	StrCmp $runEADMfound "1" noInstall 		; check if /runEADM was specified
				
    MessageBox MB_YESNO|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONEXCLAMATION|MB_DEFBUTTON2 $(EADM_NEWER_VERSION_INSTALLED) /SD IDNO IDYES install IDNO noInstall

allowOldInstallSilently:
		
    MessageBox MB_YESNO|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONEXCLAMATION|MB_DEFBUTTON2 $(EADM_NEWER_VERSION_INSTALLED) /SD IDYES IDYES install IDNO noInstall

done:
  
  	; if forceinstall has been found, we allow re-installation
	StrCmp $forceinstallfound "1" reinstall	; check if /forceinstall was specified

  	; if called via ITE we do not allow re-installation
	StrCmp $runEADMfound "1" launch		; check if /runEADM was specified
		
reinstall:
    MessageBox MB_YESNO|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONEXCLAMATION|MB_DEFBUTTON2 $(EADM_REINSTALL_EADM) /SD IDYES IDYES install IDNO noInstall

install:

	; check for pending installations from older(<9.0) Origin/EADM builds
	System::Call 'installerdll$tempName::DoPendingDownloadsExist(), i.r1'
	IntCmp $1 0 skipPendingDownloadsMessage
	MessageBox MB_YESNO|MB_TOPMOST|MB_SETFOREGROUND|MB_ICONINFORMATION|MB_DEFBUTTON1 $(ebisu_installer_detect_pending_downloads) /SD IDYES IDYES skipPendingDownloadsMessage IDNO noInstall

skipPendingDownloadsMessage:


	; this progress bar is visible always, even for "silent" installs	!!!
	; show pogress bar while extracting the real installer
	IfSilent +2 0
	System::Call 'installerdll$tempName::ShowProgressWindow(w "$(EADM_EXTRACT_INSTALLER)", i 1)'

	File /a "/oname=$TEMP\Setup.exe" "${infile}"	; extract our real installer

	; close the progress bar window
	IfSilent +2 0
	System::Call 'installerdll$tempName::ShowProgressWindow(w "$(EADM_EXTRACT_INSTALLER)", i 0)'

	; *****************************
	; search for the ITO staging config file
	; *****************************

  StrCpy $1 "$DESKTOP\Origin config for ItO.ini"
  IfFileExists $1 0 skipFlagITOConfigFile
	StrCpy $3 "$3 /ITOStagingFileFound=$1"	; add a command line flag for the ITO config file
skipFlagITOConfigFile:
	System::Call 'installerdll$tempName::RunProcess(w "$TEMP\Setup.exe", w "$TEMP", i 1, w r3)'

noInstall:

launch:
	
	; Check if we want to not launch Origin afterwards
	StrCpy $0 "" ; clear
	${GetParameters} $0
	ClearErrors
	${GetOptions} $0 "/dontLaunch" $1
	IfErrors 0 dontRun

	StrCmp $runEADMfound "1" skipLaunchFlag 0		; check if /runEADM was specified
	IfSilent skipLaunchFlag 										; on silent installations, we do not get the "Launch" flag,
																							; this may be a bug in NSIS, anyway we have set the default to "run after install"...
																							; and there is no way for the user to change that in silent mode
	
	; Check to see if the "run after the installation" checkbox was checked
	ClearErrors
	ReadRegStr $0 HKLM "SOFTWARE\${productname}" "Launch"	; run checkbox state of our installer
	IfErrors dontRun
	StrCmp $0 $tempName 0 dontRun	; we use the random number in $tempName, because we cannot delete the launch flag from an unelevated process
																; and do not want to run the next time, the installer is excuted, but not actually installed

skipLaunchFlag:
	
	; Check to see if the isntallation was a success before running the client
	ClearErrors
	ReadRegStr $0 HKLM "SOFTWARE\${productname}" "InstallSuccesfull"
	IfErrors dontRun
	StrCmp $0 "true" 0 dontRun

	; get the EADM installation directory
	ClearErrors
	ReadRegStr $1 HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation"
	IfErrors notInstalled
	IfFileExists $1 0 notInstalled  
	StrCpy $INSTDIR $1
	
	; Check if we want to tell Origin to set its autoupdate setting
	StrCpy $3 "" ; clear
	${GetParameters} $3 ; get the command line parameters from the installer to pass them to EADMUI
	ClearErrors
	ReadRegStr $2 HKLM "SOFTWARE\${productname}" "Autoupdate"
	IfErrors autoPatch
	StrCmp $2 "true" 0 autoPatch
	StrCpy $3 "$3 /AutoUpdate"
    
	
autoPatch:
	; Check if we want to tell Origin that it should auto update
	ReadRegStr $2 HKLM "SOFTWARE\${productname}" "AutopatchGlobal"
	IfErrors telemOO
	StrCmp $2 "true" 0 telemOO
	StrCpy $3 "$3 /AutoPatchGlobal"
	
telemOO:
	; Check if we want to tell Origin to set its Telemetry opt out setting
	ReadRegStr $2 HKLM "SOFTWARE\${productname}" "TelemOO"
	IfErrors continue
	StrCmp $2 "true" 0 continue
	StrCpy $3 "$3 /TelemOO"

continue:
    ; If dialog containing user adjustable options was shown,
    ; then use user selected settings
	ReadRegStr $4 HKLM "SOFTWARE\${productname}" "ShowDirPage"

    StrCmp $4 "true" addInstallingFlag updating
updating:
	; If not installing, then we're updating
	StrCpy $3 "$3 /Updating"
	Goto run

addInstallingFlag:
	; Add '/Installing' flag to let Origin know that it is Installing (as opposed to Updating or Repairing)
	StrCpy $3 "$3 /Installing"
	
run:
	SetOutPath "$TEMP"
	System::Call 'installerdll$tempName::RunProcess(w "$INSTDIR\${EBISU_FILENAME}.exe", w "$INSTDIR", i 0, w r3)'

dontRun:
notInstalled:

	; remove our helper DLL
	System::Call '${sysGetModuleHandle}("$TEMP\installerdll$tempName.dll" ) .r1'	; get the HMODULE handle and free it
	System::Call '${sysFreeLibrary}( r1 ) .r0'
	Delete "$TEMP\installerdll$tempName.dll"

SectionEnd


