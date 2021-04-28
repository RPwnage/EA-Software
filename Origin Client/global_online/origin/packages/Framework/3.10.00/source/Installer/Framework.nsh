;
; Utility functions for Framework installer
;

;--------------------------------
; Installer Functions

!include "winver.nsh"


Function CheckForDotNet
    ; check for C:\windows\Microsoft.NET\Framework\v4.0*
    ; if not present prompt to download the .NET Framework via a weblink
    DetailPrint "Checking for Microsoft.NET..."
    IfFileExists "$WINDIR\Microsoft.NET\Framework\v4.0*" DotNetInstalled
        DetailPrint "Not Installed.  Install cannot continue."
        MessageBox MB_OK "You need to install Microsoft.Net 4.0 before you can install the Framework."
        Quit
    DotNetInstalled:
FunctionEnd


Function CheckForWindowsVersion
    
    ${If} ${AtLeastWinXP}
     
    ${Else}
    MessageBox MB_OK "The Framework requires Microsoft Windows 2000, XP or VISTA."
    Quit
    ${EndIf}

FunctionEnd


;--------------------------------
; Uninstaller Functions


;Dir = $0 
;Error Flag 1=empty or not existent 
Function un.IsDirEmpty 
    push $R1 
    push $R2 

    ClearErrors 
    IfFileExists "$0" 0 not_exists 
    FindFirst $R1 $R2 "$0\*.*" 
    IfErrors no_files 
    FindNext $R1 $R2 
    IfErrors no_files 
    FindNext $R1 $R2 
    IfErrors no_files 
    FindClose $R1 
    ClearErrors 
    goto end 

no_files: 
    FindClose $R1 
    SetErrors 
    goto end 


not_exists: 
    SetErrors 

end: 
    pop $R2 
    pop $R1 

FunctionEnd

;Dir = $0 
;Error Flag 1=empty or not existent 
Function un.IsPackageRootDirEmpty 
    push $R1 
    push $R2 

    ClearErrors 
    IfFileExists "$0" 0 not_empty
    FindFirst $R1 $R2 "$0\*.*" 
    ;MessageBox MB_OK "$R2" ; .
    IfErrors not_empty
    FindNext $R1 $R2  
    ;MessageBox MB_OK "$R2" ; ..
    IfErrors empty 
    FindNext $R1 $R2  
    ;MessageBox MB_OK "$R2" ; PackageRoot.xml
    IfErrors empty 
    StrCmp $R2 "PackageRoot.xml" 0 not_empty
    FindNext $R1 $R2
    ;MessageBox MB_OK "$R2" ; <empty>
    IfErrors empty 
    goto not_empty

empty: 
    ;MessageBox MB_OK "empty"
    FindClose $R1 
    SetErrors 
    goto end 

not_empty:
    ;MessageBox MB_OK "not empty"
    FindClose $R1 
    ClearErrors 
    goto end 

end: 
    pop $R2 
    pop $R1 

FunctionEnd


; scan the registry for the path to the newest version of framework
Function un.GetLatestVersion
    StrCpy $R0 0            ; index variable
    StrCpy $R1 ""            ; return version
    StrCpy $R2 ""            ; return path
    StrCpy $R3 ""            ; return package path
    RegKey:
        EnumRegKey $R4 HKLM "SOFTWARE\Electronic Arts\${NAME}" $R0
        StrCmp $R4 "" EndRegKey            ; nothing found

        StrCpy $R3 $R4                                                                                ; copy version
        ReadRegStr $R2 HKEY_LOCAL_MACHINE "SOFTWARE\Electronic Arts\${NAME}\$R3" "Path"                ; copy path
        ReadRegStr $R1 HKEY_LOCAL_MACHINE "SOFTWARE\Electronic Arts\${NAME}\$R3" "PackagePath"        ; copy package path

        IntOp $R0 $R0 + 1                ; increment index
    Goto RegKey                            ; loop
    EndRegKey:
    Push $R1                            ; push package path 
    Push $R2                            ; push path
    Push $R3                            ; push version 
FunctionEnd


; push version onto stack if this version is the currently associated version otherwise empty string
Function un.CheckVersion

    ReadRegStr $R0 HKEY_LOCAL_MACHINE "SOFTWARE\Electronic Arts\${NAME}" ""
    StrCpy $R1 ""
    StrCmp $R0 "" EndStrCmp                            ; nothing found error
        StrCmp $R0 "${VERSION}" 0 EndStrCmp            ; matching version
            StrCpy $R1 $R0
    EndStrCmp:
    Push $R1

FunctionEnd
