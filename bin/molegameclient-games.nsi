; 该脚本使用 HM VNISEdit 脚本编辑器向导产生

; 安装程序初始定义常量
!define PRODUCT_NAME "mole开放游戏平台"
!define PRODUCT_VERSION "1.0"
!define PRODUCT_PUBLISHER "mole软件工作室"
!define PRODUCT_WEB_SITE "http://www.mole2d.com"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\MolGameClient.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

SetCompressor lzma

; ------ MUI 现代界面定义 (1.67 版本以上兼容) ------
!include "MUI.nsh"

; MUI 预定义常量
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; 欢迎页面
!insertmacro MUI_PAGE_WELCOME
; 许可协议页面
!insertmacro MUI_PAGE_LICENSE "LICENSE.TXT"
; 安装目录选择页面
!insertmacro MUI_PAGE_DIRECTORY
; 安装过程页面
!insertmacro MUI_PAGE_INSTFILES
; 安装完成页面
!define MUI_FINISHPAGE_RUN "$INSTDIR\MolGameClient.exe"
!insertmacro MUI_PAGE_FINISH

; 安装卸载过程页面
!insertmacro MUI_UNPAGE_INSTFILES

; 安装界面包含的语言设置
!insertmacro MUI_LANGUAGE "SimpChinese"

; 安装预释放文件
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS
; ------ MUI 现代界面定义结束 ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "MoleGameSetup.exe"
InstallDir "$PROGRAMFILES\mole开放游戏平台"
InstallDirRegKey HKLM "${PRODUCT_UNINST_KEY}" "UninstallString"
ShowInstDetails show
ShowUnInstDetails show

Section "MainSection" SEC01
  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer
  File "MolGameClient.exe"
  CreateDirectory "$SMPROGRAMS\mole开放游戏平台"
  CreateShortCut "$SMPROGRAMS\mole开放游戏平台\mole开放游戏平台.lnk" "$INSTDIR\MolGameClient.exe"
  CreateShortCut "$DESKTOP\mole开放游戏平台.lnk" "$INSTDIR\MolGameClient.exe"
  File "Mole2d.dll"
  File "MolCrashRptSender.exe"
  File "MolCrashRpt.dll"
  File "dbghelp.dll"
  File "clientconfig.ini"
  File "bass.dll"
  File "LICENSE.TXT"
  File "MFC71.dll"
  File "msvcp71.dll"
  File "msvcr71.dll"  
  File /r "gameroomconfig"
  File /r "Avatars"
  File /r "300002"
SectionEnd

Section -AdditionalIcons
  WriteIniStr "$INSTDIR\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
  CreateShortCut "$SMPROGRAMS\mole开放游戏平台\Website.lnk" "$INSTDIR\${PRODUCT_NAME}.url"
  CreateShortCut "$SMPROGRAMS\mole开放游戏平台\Uninstall.lnk" "$INSTDIR\uninst.exe"
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\MolGameClient.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\MolGameClient.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd

/******************************
 *  以下是安装程序的卸载部分  *
 ******************************/

Section Uninstall
  Delete "$INSTDIR\${PRODUCT_NAME}.url"
  Delete "$INSTDIR\uninst.exe"
  Delete "$INSTDIR\MolGameClient.exe"
  Delete "$INSTDIR\bass.dll"
  Delete "$INSTDIR\dbghelp.dll"
  Delete "$INSTDIR\LICENSE.TXT"
  Delete "$INSTDIR\MFC71.dll"
  Delete "$INSTDIR\MolCrashRpt.dll"
  Delete "$INSTDIR\clientconfig.ini"  
  Delete "$INSTDIR\mole2d.log"  
  Delete "$INSTDIR\MolCrashRptSender.exe"
  Delete "$INSTDIR\Mole2d.dll"
  Delete "$INSTDIR\msvcp71.dll"
  Delete "$INSTDIR\msvcr71.dll"

  Delete "$SMPROGRAMS\mole开放游戏平台\Uninstall.lnk"
  Delete "$SMPROGRAMS\mole开放游戏平台\Website.lnk"
  Delete "$DESKTOP\mole开放游戏平台.lnk"
  Delete "$SMPROGRAMS\mole开放游戏平台\mole开放游戏平台.lnk"
  
  RMDir "$SMPROGRAMS\mole开放游戏平台"

  RMDir /r "$INSTDIR\Avatars"
  RMDir /r "$INSTDIR\gameroomconfig"
  RMDir /r "$INSTDIR\300002"  

  RMDir "$INSTDIR"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true
SectionEnd

#-- 根据 NSIS 脚本编辑规则，所有 Function 区段必须放置在 Section 区段之后编写，以避免安装程序出现未可预知的问题。--#

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "你确实要完全移除 $(^Name) ，及其所有的组件？" IDYES +2
  Abort
FunctionEnd

Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) 已成功地从你的计算机移除。"
FunctionEnd
