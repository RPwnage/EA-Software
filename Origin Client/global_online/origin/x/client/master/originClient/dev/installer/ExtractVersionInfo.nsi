!include "Versions.nsh"

;--------------------------------
;General
;--------------------------------
XPStyle on

;Request application privileges for Windows Vista
RequestExecutionLevel admin
SilentInstall silent
SetCompressor LZMA


;--------------------------------
; Include Modern UI and functions
;--------------------------------
!include "MUI2.nsh"
!include "WordFunc.nsh"
!include "x64.nsh"
!include "FileFunc.nsh"
!include "StrFunc.nsh"
!include "Utils.nsh"

; Maven has its own directory structure so adjust that
!ifndef MAVEN
    !define EBISU_RUNTIMEDIR "..\runtime"
    !define CORE_RUNTIMEDIR "..\..\..\..\core\Main\CoreService\runtime"
    !define SRC_PATCHPROGRESSDIR "..\PatchProgress"
    !define SRC_ACCESSDIR "..\Access"
    !define SRC_EAACCESSGAMESDIR "..\EAAccessGames"
    !define SRC_AWCDIR "..\AccessDRM\Output"
    !define SRC_EADM6DIR "..\Vault"
    !define INSTALLER_DLLDIR ".\InstallerDLL\Release"
	!define QT_DLLSDIR "..\common\packages\QT\4.6.3_webkit_4.7.0\lib"
	!define QT_PLUGIN_DIR "..\common\packages\QT\4.6.3_webkit_4.7.0\plugins"
!else
    !define EBISU_RUNTIMEDIR "..\temp"
    !define CORE_RUNTIMEDIR "..\temp"
    !define SRC_PATCHPROGRESSDIR "..\temp"
    !define SRC_ACCESSDIR "..\temp"
    !define SRC_EAACCESSGAMESDIR "..\temp"
    !define SRC_AWCDIR "..\temp"
    !define SRC_EADM6DIR "..\temp"
    !define INSTALLER_DLLDIR "..\temp"
	!define QT_DLLSDIR "${QT_DIR}\lib"
	!define QT_PLUGIN_DIR "${QT_DIR}\plugins"
!endif

!define File "${EBISU_RUNTIMEDIR}\${EBISU_FILENAME}.exe"

OutFile "ExtractVersionInfo.exe"

Section

 ## Get file version
 GetDllVersion "${File}" $R0 $R1
  IntOp $R2 $R0 / 0x00010000
  IntOp $R3 $R0 & 0x0000FFFF
  IntOp $R4 $R1 / 0x00010000
  IntOp $R5 $R1 & 0x0000FFFF
  StrCpy $R1 "$R2.$R3.$R4.$R5"

 ## Write it to a !define for use in main script
 FileOpen $R0 "appversion.txt" w
  FileWrite $R0 '!define version "$R1"'
 FileClose $R0

SectionEnd
