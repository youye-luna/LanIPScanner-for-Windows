; ===================================================================
; 局域网扫描工具 Inno Setup 安装脚本
; 用法: ISCC.exe LanIPScanner.iss
; 安装内容: release 全部内容 -> J:\LanIPScanner（不含 C 盘）
; 安装完成后再引导安装 Npcap 驱动（nmap 扫描所需）
; ===================================================================

#define MyAppName "局域网扫描工具"
#define MyAppVersion "1.5-beta2"
#define MyAppExeName "LanIPScanner.exe"

[Setup]
AppId={{8B2D9E4C-3A71-4C6E-9F5B-52A61D9C4E01}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher=youye-luna
DefaultDirName={code:GetDefaultDir}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=..\LICENSE
OutputDir=.
OutputBaseFilename=LanIPScanner-{#MyAppVersion}-setup
VersionInfoVersion=1.5.0.0
VersionInfoCompany=youye-luna
VersionInfoCopyright=Copyright (C) 2026 youye-luna
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern zircon
PrivilegesRequired=admin
UninstallDisplayIcon={app}\{#MyAppExeName}
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

[Languages]
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
; 主程序与全部运行库（Qt/MinGW 运行库 + plugins + nmap 全套）
Source: "..\release\*"; DestDir: "{app}"; Excludes: "settings.json,ScanHistory\*"; Flags: ignoreversion recursesubdirs createallsubdirs
; GPL3 许可证
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
; Npcap 驱动安装包（安装完成后引导安装）
Source: "J:\nmap-dist\npcap-1.88.exe"; DestDir: "{app}\nmap"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
; 先装主软件（上面 Files 阶段），装完后在完成页引导安装 Npcap 驱动
Filename: "{app}\nmap\npcap-1.88.exe"; Description: "安装 Npcap 驱动（nmap 扫描所需，扫描更快并可获取 MAC）"; Flags: postinstall shellexec skipifsilent; Check: not IsNpcapInstalled

[Code]
function GetDefaultDir(Param: string): string;
begin
  if DirExists('D:\') then
    Result := 'D:\LanIPScanner'
  else
    Result := 'C:\LanIPScanner';
end;

function IsNpcapInstalled(): Boolean;
begin
  Result := FileExists(ExpandConstant('{sys}\Npcap\npcap.sys')) or
            FileExists(ExpandConstant('{sys}\wpcap.dll'));
end;
