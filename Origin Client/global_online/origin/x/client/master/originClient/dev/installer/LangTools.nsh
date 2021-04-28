;--------------------------------
; EA Language utilities
;--------------------------------

!include "LogicLib.nsh"

var myInstallLangLocale
var myLocale
var myBuild

!macro DebugMessageBox1 text
!if ${build} == "debug"
    MessageBox MB_OK "${text}"
    Nop
!endif
!macroend


!macro FUNC_GetLocaleIdFromLocale un
Function ${un}GetLocaleIdFromLocale

Pop $0

${Switch} $0
	${Case} "cs_CZ"
		Push "1029"
		${Break}
	${Case} "da_DK"
		Push "1030"
		${Break}
	${Case} "de_DE"
		Push "1031"
		${Break}
	${Case} "el_GR"
		Push "1032"
		${Break}
	${Case} "en_GB"
		Push "1033"
		${Break}
	${Case} "en_US"
		Push "1033"
		${Break}
	${Case} "es_ES"
		Push "1034"
		${Break}
	${Case} "es_MX"
		Push "2058"
		${Break}
	${Case} "fi_FI"
		Push "1035"
		${Break}
	${Case} "fr_FR"
		Push "1036"
		${Break}
	${Case} "hu_HU"
		Push "1038"
		${Break}
	${Case} "it_IT"
		Push "1040"
		${Break}
	${Case} "ja_JP"
		Push "1041"
		${Break}
	${Case} "ko_KR"
		Push "1042"
		${Break}
	${Case} "nl_NL"
		Push "1043"
		${Break}
	${Case} "nb_NO"
		Push "1044"
		${Break}
	${Case} "no_NO"
		Push "1044"
		${Break}
	${Case} "pl_PL"
		Push "1045"
		${Break}
	${Case} "pt_BR"
		Push "1046"
		${Break}
	${Case} "pt_PT"
		Push "2070"
		${Break}
	${Case} "ru_RU"
		Push "1049"
		${Break}
	${Case} "sv_SE"
		Push "1053"
		${Break}
	${Case} "th_TH"
		Push "1054"
		${Break}
	${Case} "zh_CN"
		Push "2052"
		${Break}
	${Case} "zh_TW"
		Push "1028"
		${Break}
	${Default}
		Push ""
		${Break}
${EndSwitch}	

FunctionEnd
!macroend

; Insert function as an installer and uninstaller function.
!insertmacro FUNC_GetLocaleIdFromLocale ""
!insertmacro FUNC_GetLocaleIdFromLocale "un."


;==================================================
; LANGUAGE SELECTION
; If the display language is supported, use it
; else, if the display language is not supported, test the default locale
; if the default locale is supported, use it
; else, default to English
;
Function LocaleLanguageSelection
	
; 1. get display language locale code. If it is supported, continue	
	Call GetLocaleName
    Pop $R0   ; at this point $R0 is whatever the display language is
	StrCpy $myInstallLangLocale $R0 ; exchange this $R0 for "es_MX" or anything like it for testing purposes
		!insertmacro DebugMessageBox1 "STEP 1.1: : GetLocaleName. the display language is:  $myInstallLangLocale"
	; Check to see if this language is supported
	Push $myInstallLangLocale
	Call GetLocaleIdFromLocale
	Pop $myLocale
		!insertmacro DebugMessageBox1 "STEP 1.2: : GetLocaleIdFromLocale. the locale id for the display language is:  $myLocale"
				
	StrCpy $R0 $myLocale
	; Test to see if the display language is supported
	StrCmp $R0 "" notSupportedLang supportedLang

notSupportedLang:	
	
; 2. If it is not supported, get default locale. If it is supported, continue
	
	Call GetSystemDefaultLanguageCode
	Pop $myInstallLangLocale  ; $myInstallLangLocale now contains whetever it is in the format xx_XX	
		!insertmacro DebugMessageBox1 "STEP 2.1: : GetSystemDefaultLanguageCode:  $myInstallLangLocale"
		
	; Check to see if this language is supported
	Push $myInstallLangLocale
	Call GetLocaleIdFromLocale
	Pop $myLocale
		!insertmacro DebugMessageBox1 "STEP 2.3: : GetLocaleIdFromLocale. the locale id for the display language is:  $myLocale"
				
	StrCpy $R0 $myLocale
	; Test to see if the display language is supported
	StrCmp $R0 "" fallBackLanguage supportedLang

fallBackLanguage:
	Push $LANGUAGE
	Goto langDone

supportedLang:
	Push $R0
	Goto langDone
		
langDone:
FunctionEnd



