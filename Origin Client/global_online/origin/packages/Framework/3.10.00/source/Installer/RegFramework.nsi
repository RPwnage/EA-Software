; this .NSI script is designed for NSIS v2.0.a7

;!include "${NSISDIR}\ModernUI.nsh"
;!include "${NSISDIR}\WinMessages.nsh"

;--------------------------------
;Configuration

    ; executable file
    Name                "$RegFramework"
    OutFile             "${OUTDIR}/RegFramework.exe"

    ; ui
    SetFont             Tahoma 8
    XPStyle             On
    InstallColors       /windows
    InstProgressFlags   "smooth"
    ShowInstDetails     show
    WindowIcon          off
    Icon                "install.ico"

    ; language
    BrandingText        "packages.ea.com"
    Caption                "${NAME} Registration"
    CompletedText        "You are now using version ${VERSION} of the ${NAME}."

    
;--------------------------------
;Installer Sections

Section "setup.exe" 
    
    DetailPrint "Registering Package Installer..."
    ExecWait '"$EXEDIR\PackageInstaller.exe" /install'
    ifErrors Error 0

    
    DetailPrint " "
    Goto End

Error:
    DetailPrint " "
    DetailPrint "Could not find version ${VERSION} of ${NAME}."
    DetailPrint "Looked for keys under HKEY_LOCAL_MACHINE\"
    DetailPrint "SOFTWARE\Electronic Arts\${NAME}\${VERSION}"
    SetErrors
    Abort
End:
SectionEnd