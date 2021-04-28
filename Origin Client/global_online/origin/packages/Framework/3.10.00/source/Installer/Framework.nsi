; this .NSI script is designed for NSIS v2.46

!include "MUI2.nsh"
!include "Framework.nsh"

;--------------------------------
;General

  ;Name and file
  Name "${NAME} ${VERSION}"
  OutFile "${OUTDIR}\${NAME}-${VERSION}.exe"

  ;Default installation folder
  InstallDir "C:\packages\"

  ;Get installation folder from registry if available
  InstallDirRegKey HKLM "Software\Electronic Arts\${NAME}" "Path"

  AllowRootDirInstall true

  ;Request application privileges for Windows Vista
  RequestExecutionLevel user

;--------------------------------
;Interface Settings

  !define MUI_ICON "install.ico"
  !define MUI_ABORTWARNING

  !define MUI_PAGE_HEADER_TEXT                "Choose Package Root" 
  !define MUI_PAGE_HEADER_SUBTEXT             "Choose the folder in which to install packages in."
  !define MUI_DIRECTORYPAGE_TEXT_TOP          "All packages including the the ${NAME} package will be installed into the package root folder."
  !define MUI_DIRECTORYPAGE_TEXT_DESTINATION  "Select the directory to install packages in:"

  BrandingText "packages.worldwide.ea.com"
  ShowInstDetails show

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
; Installer sections

Section "Packages Directory" ; (default section)

    !define SOURCE_EXCLUDES "/x obj /x *.obj /x *.opensdf /x *.sdf /x *.suo /x *.log /x *.pidb /x *.user /x *.userprefs"

    ; add files / whatever that need to be installed here.
    DetailPrint "Copying files..."
    SetOutPath "$INSTDIR\${NAME}\${VERSION}\bin"
    File    /x *.vshost.exe "..\..\bin\*.*"
    SetOutPath "$INSTDIR\${NAME}\${VERSION}"
    File /r "..\..\data"
    File /r "..\..\doc"
    File /r  ${SOURCE_EXCLUDES} "..\..\source"
    File    "..\..\Framework.build"
    File    "..\..\Manifest.xml"
    File    "..\..\nant.bat"
    File    "..\..\nant"
    File    "..\..\COPYING.txt"
    File    "..\..\COPYRIGHT.txt"
    File    "..\..\license.html"
    File    "..\..\3RDPARTYLICENSES.TXT"
    File    "..\..\EA_DEV_SOFTWARE_LICENSE.TXT"
    File    "..\..\readme_unix.txt"

    ; create PackageRoot.xml file
    SetOutPath "$INSTDIR"
    File "PackageRoot.xml"

    ; write out uninstaller
    CreateDirectory "$INSTDIR\${NAME}\${VERSION}\bin"
    WriteUninstaller "$INSTDIR\${NAME}\${VERSION}\bin\uninst.exe"
    
    ; write required registry entries
    DetailPrint "Registering files..."
    
    ; write framework registry settings
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Electronic Arts\${NAME}" "" "${VERSION}"
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Electronic Arts\${NAME}" "Path" "$INSTDIR"
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Electronic Arts\${NAME}\${VERSION}" "" ""
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Electronic Arts\${NAME}\${VERSION}" "Path" "$INSTDIR\${NAME}\${VERSION}"
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Electronic Arts\${NAME}\${VERSION}" "PackagePath" "$INSTDIR"
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Electronic Arts\${NAME}\${VERSION}\PackageInstaller" "" ""
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Electronic Arts\${NAME}\${VERSION}\PackageInstaller" "Path" "$INSTDIR\${NAME}\${VERSION}\bin\PackageInstaller.exe"
    
    ; write uninstaller registry settings
    WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}-${VERSION}" "DisplayName" "${NAME}-${VERSION} (remove only)"
    WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}-${VERSION}" "UninstallString" '"$INSTDIR\${NAME}\${VERSION}\bin\uninst.exe"'

    ; run PackageInstaller to register file type
    ExecWait '"$INSTDIR\${NAME}\${VERSION}\bin\PackageInstaller.exe" /install'

    DetailPrint "Creating shortcuts..."
    SetOutPath     "$SMPROGRAMS\${NAME}\${VERSION}"
    WriteINIStr    "$SMPROGRAMS\${NAME}\${VERSION}\EA Package Server.url" "InternetShortcut" "URL" "http://packages.ea.com/"
    CreateShortCut "$SMPROGRAMS\${NAME}\${VERSION}\Package Manager.lnk" '"$INSTDIR\${NAME}\${VERSION}\bin\PackageManager.exe"'
    CreateShortCut "$SMPROGRAMS\${NAME}\${VERSION}\Packages Directory.lnk" "$INSTDIR"
    CreateShortCut "$SMPROGRAMS\${NAME}\${VERSION}\Documentation.lnk" '"$INSTDIR\${NAME}\${VERSION}\doc\Framework3Guide.chm"'
    WriteINIStr    "$SMPROGRAMS\${NAME}\${VERSION}\Framework Home Page.url" "InternetShortcut" "URL" "http://eatech.ea.com/C15/P62/default.aspx"
    CreateShortCut "$SMPROGRAMS\${NAME}\${VERSION}\Uninstall ${NAME}-${VERSION}.lnk" '"$INSTDIR\${NAME}\${VERSION}\bin\uninst.exe"'

    SetOutPath     "$SMPROGRAMS\${NAME}\${VERSION}\Tools"


    ; create shortcut for registering file types
    CreateShortCut "$SMPROGRAMS\${NAME}\${VERSION}\Tools\Register File Types.lnk" '"$INSTDIR\${NAME}\${VERSION}\bin\RegFramework.exe"'
    
    ; create the NAnt command prompt shortcut
    StrCpy $R0 $OUTDIR                                ; save the outdir to restore later
    SetOutPath "$INSTDIR\${NAME}\${VERSION}\bin"    ; copy over nant environment setup file
    File "NAntenv.bat"
    SetOutPath "$INSTDIR"                            ; sets the current working dir of the shourtcut to packages root
    CreateShortCut    '$SMPROGRAMS\${NAME}\${VERSION}\Tools\NAnt Command Prompt.lnk' %comspec% '/k ""$INSTDIR\${NAME}\${VERSION}\bin\NAntenv.bat" "$INSTDIR\${NAME}\${VERSION}\bin""'
    SetOutPath $R0                                    ; restore the outpath

    DetailPrint "${NAME}-${VERSION} installed"
SectionEnd


;--------------------------------
; Installer Functions

Function .onInit
    Call CheckForWindowsVersion    ; verifies operating system
    Call CheckForDotNet            ; verifies .NET installation
FunctionEnd


;--------------------------------
; Uninstaller settings

UninstallIcon "uninstall.ico"
UninstallCaption "${NAME}-${VERSION} Uninstall"
UninstallText "Only the ${NAME}-${VERSION} package will be removed from your system.  No other packages will be modified."
ShowUninstDetails show

;--------------------------------
; Uninstaller sections


Section Uninstall

    ; if we are the associated version $R0 will contain the version otherwise empty string
    Call un.CheckVersion
    Pop $R0

    DetailPrint "Removing registry entries..."
    DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Electronic Arts\${NAME}\${VERSION}"
    DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}-${VERSION}"
    
    ; Remove registery entries for file type associations if we own them
    StrCmp $R0 "" EndStrCmp1 0

        ; get the latest version of framework
        Call un.GetLatestVersion
        Pop $R1        ; version
        Pop $R2        ; path
        Pop $R3        ; package path
        
        DeleteRegKey HKEY_CLASSES_ROOT "EA.Framework.PackageInstaller.1"
        DeleteRegKey HKEY_CLASSES_ROOT ".eap"
        DeleteRegKey HKEY_CLASSES_ROOT ".build"

        ; run PackageInstaller to deregister file type
        ExecWait '"$INSTDIR\PackageInstaller.exe" /uninstall'

        ; setup next version of framework
        StrCmp $R1 "" EndStrCmp2 0
            ; get the next framework version and run file associations for it
            ExecWait '"$R2\bin\PackageInstaller.exe" /install'
            
            ; set the registry to point to new framework
            WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Electronic Arts\${NAME}" "" "$R1"
            WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Electronic Arts\${NAME}" "Path" "$R3"
            
        EndStrCmp2:

    EndStrCmp1:

    ; Remove any shortcuts from the Start Menu
    DetailPrint "Removing shortcuts..."
    RMDir /r "$SMPROGRAMS\${NAME}\${VERSION}"

    ; delete the Framework start menu directory if it is empty
    RMDir "$SMPROGRAMS\${NAME}"
    
    ; delete the Framework registry key if it is empty
    DeleteRegKey /ifempty HKEY_LOCAL_MACHINE "SOFTWARE\Electronic Arts\${NAME}"

    ; delete Framework package directory
    GetFullPathName $R0 "$INSTDIR\..\..\.."
    DetailPrint "Removing ${NAME}\${VERSION} directory..."
    RMDir /r "$R0\${NAME}\${VERSION}"

    ; delete the Framework and packages directories if they are empty
    Strcpy $0 "$R0\${NAME}" 
    call un.IsDirEmpty 
    ifErrors 0 PackagesNotEmpty
    RMDir "$0"

    ; delete the Package directory if only PackageRoot.xml file inside
    Strcpy $0 "$R0" 
    call un.IsPackageRootDirEmpty 
    ifErrors 0 PackagesNotEmpty
    RMDir /r "$0"
    Goto End

PackagesNotEmpty:
    DetailPrint "NOTE: If you wish to remove all packages installed on your system you should"
    DetailPrint "delete the $R0 directory."

End:
    ;!insertmacro MUI_FINISHHEADER un.SetHeader

    ;MessageBox MB_OK "If you wish to remove all packages installed on your system you should delete the $R0 directory."
    DetailPrint "${NAME}-${VERSION} uninstalled"
SectionEnd ; end of uninstall section
