
Unicode true
ManifestDPIAware true

ManifestSupportedOS all

!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "LogicLib.nsh"
!include "WinVer.nsh"
!include "x64.nsh"


!define PRODUCT_NAME      "OpenUaSim"
!define PRODUCT_PUBLISHER "Ananev Alexandr"
!define PRODUCT_WEB_SITE  "https://github.com/sanny32/OpenUaSim"
!define EXE_NAME          "ouasim.exe"

!define REG_APP_KEY       "Software\${PRODUCT_NAME}"
!define REG_UNINST_KEY    "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenUaSim"


!ifndef SRC_DIR
    !define SRC_DIR "${__FILEDIR__}\..\.."
!endif

!ifndef STAGE_DIR
    !error "STAGE_DIR is not defined: pass /DSTAGE_DIR=<staged install tree>"
!endif

!ifndef APP_VERSION
    !error "APP_VERSION is not defined: pass /DAPP_VERSION=<display version, e.g. 1.0.0>"
!endif

!ifndef FILE_VERSION
    !error "FILE_VERSION is not defined: pass /DFILE_VERSION=<numeric version, e.g. 1.0.0.0>"
!endif

!ifndef OUT_FILE
    !define OUT_FILE "ouasim-${APP_VERSION}-win64-setup.exe"
!endif

Name "${PRODUCT_NAME} ${APP_VERSION}"
OutFile "${OUT_FILE}"
InstallDir "$PROGRAMFILES64\${PRODUCT_NAME}"
InstallDirRegKey HKLM "${REG_APP_KEY}" "InstallDir"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
BrandingText "${PRODUCT_NAME} ${APP_VERSION}"


VIProductVersion "${FILE_VERSION}"
VIAddVersionKey "ProductName"     "${PRODUCT_NAME}"
VIAddVersionKey "ProductVersion"  "${APP_VERSION}"
VIAddVersionKey "FileVersion"     "${FILE_VERSION}"
VIAddVersionKey "FileDescription" "${PRODUCT_NAME} Setup"
VIAddVersionKey "CompanyName"     "${PRODUCT_PUBLISHER}"
VIAddVersionKey "LegalCopyright"  "Copyright (C) ${PRODUCT_PUBLISHER}"


!define MUI_ABORTWARNING
!define MUI_ICON   "${SRC_DIR}\src\app\res\windows\ouasim.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP "${SRC_DIR}\.github\windows\installer-sidebar.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "${SRC_DIR}\.github\windows\installer-sidebar.bmp"


!define MUI_FINISHPAGE_RUN "$INSTDIR\${EXE_NAME}"
!define MUI_FINISHPAGE_RUN_TEXT "Run ${PRODUCT_NAME}"
!define MUI_FINISHPAGE_LINK "Visit the project on GitHub"
!define MUI_FINISHPAGE_LINK_LOCATION "${PRODUCT_WEB_SITE}"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${SRC_DIR}\LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!define MUI_UNCONFIRMPAGE_TEXT_TOP "${PRODUCT_NAME} will be removed from the folder below."
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"


Function .onInit
    ${IfNot} ${AtLeastWin10}
        MessageBox MB_OK|MB_ICONSTOP "${PRODUCT_NAME} requires Windows 10 or later."
        Abort
    ${EndIf}

    ${IfNot} ${RunningX64}
        MessageBox MB_OK|MB_ICONSTOP "${PRODUCT_NAME} requires a 64-bit version of Windows."
        Abort
    ${EndIf}
    SetRegView 64

    ReadRegStr $R0 HKLM "${REG_UNINST_KEY}" "UninstallString"
    ${If} $R0 != ""
        ReadRegStr $R1 HKLM "${REG_UNINST_KEY}" "DisplayVersion"
        MessageBox MB_YESNO|MB_ICONQUESTION \
            "${PRODUCT_NAME} $R1 is already installed.$\n$\nUninstall it before continuing?" \
            IDNO +2
        ExecWait '$R0 /S _?=$INSTDIR'
    ${EndIf}
FunctionEnd

Section "-Visual C++ runtime"
    InitPluginsDir
    File /oname=$PLUGINSDIR\vc_redist.x64.exe "${STAGE_DIR}\vc_redist.x64.exe"
    ExecWait '"$PLUGINSDIR\vc_redist.x64.exe" /install /quiet /norestart' $0
    ${If} $0 == 3010
        SetRebootFlag true
    ${ElseIf} $0 != 0
    ${AndIf} $0 != 1638
        MessageBox MB_OK|MB_ICONSTOP "Visual C++ runtime installation failed ($0)."
        Abort
    ${EndIf}
SectionEnd

Section "${PRODUCT_NAME}" SEC_APP
    SectionIn RO
    SetShellVarContext all
    SetOutPath "$INSTDIR"

    File /r "${STAGE_DIR}\*.*"

    WriteUninstaller "$INSTDIR\uninstall.exe"
    WriteRegStr HKLM "${REG_APP_KEY}" "InstallDir" "$INSTDIR"

    CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk" "$INSTDIR\${EXE_NAME}"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall ${PRODUCT_NAME}.lnk" "$INSTDIR\uninstall.exe"

    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    WriteRegStr   HKLM "${REG_UNINST_KEY}" "DisplayName"     "${PRODUCT_NAME}"
    WriteRegStr   HKLM "${REG_UNINST_KEY}" "DisplayVersion"  "${APP_VERSION}"
    WriteRegStr   HKLM "${REG_UNINST_KEY}" "DisplayIcon"     "$INSTDIR\${EXE_NAME}"
    WriteRegStr   HKLM "${REG_UNINST_KEY}" "Publisher"       "${PRODUCT_PUBLISHER}"
    WriteRegStr   HKLM "${REG_UNINST_KEY}" "URLInfoAbout"    "${PRODUCT_WEB_SITE}"
    WriteRegStr   HKLM "${REG_UNINST_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr   HKLM "${REG_UNINST_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegStr   HKLM "${REG_UNINST_KEY}" "QuietUninstallString" '"$INSTDIR\uninstall.exe" /S'
    WriteRegDWORD HKLM "${REG_UNINST_KEY}" "EstimatedSize"   "$0"
    WriteRegDWORD HKLM "${REG_UNINST_KEY}" "NoModify"        1
    WriteRegDWORD HKLM "${REG_UNINST_KEY}" "NoRepair"        1
SectionEnd

Section "Desktop shortcut" SEC_DESKTOP
    SetShellVarContext all
    CreateShortCut "$DESKTOP\${PRODUCT_NAME}.lnk" "$INSTDIR\${EXE_NAME}"
SectionEnd


!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_APP} "${PRODUCT_NAME} and the Qt runtime it needs."
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_DESKTOP} "Place a shortcut on the desktop."
!insertmacro MUI_FUNCTION_DESCRIPTION_END


Function un.onInit
    SetRegView 64
FunctionEnd

Section "Uninstall"
    SetShellVarContext all

    Delete "$DESKTOP\${PRODUCT_NAME}.lnk"
    Delete "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk"
    Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall ${PRODUCT_NAME}.lnk"
    RMDir "$SMPROGRAMS\${PRODUCT_NAME}"

    Delete "$INSTDIR\uninstall.exe"
    RMDir /r "$INSTDIR\plugins"
    RMDir /r "$INSTDIR"

    DeleteRegKey HKLM "${REG_UNINST_KEY}"
    DeleteRegKey HKLM "${REG_APP_KEY}"

    System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, i 0, i 0)'
SectionEnd
