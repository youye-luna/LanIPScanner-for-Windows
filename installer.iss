; LanIPScanner Inno Setup 安装脚本
; 使用方法：
;   1. 先执行发布: dotnet publish -c Release
;   2. 用 Inno Setup 6 打开此脚本编译即可生成安装包

#define MyAppName      "局域网设备扫描工具"
#define MyAppNameEn    "LanIPScanner"
#define MyAppVersion   "1.4"
#define MyAppFileVersion "1.4.0.0"
#define MyAppPublisher "LanIPScanner"
#define MyAppExeName   "LanIPScanner.exe"

[Setup]
AppId={{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppNameEn}
DefaultGroupName={#MyAppName}
OutputBaseFilename=LanIPScanner_Setup_{#MyAppVersion}
OutputDir=installer_output
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
UsePreviousAppDir=yes
DirExistsWarning=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
SetupIconFile=app.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
VersionInfoVersion={#MyAppFileVersion}
VersionInfoDescription={#MyAppName}

[Languages]
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
Name: "english";           MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "bin\Release\net8.0-windows\win-x64\publish\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}";          Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}";    Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
