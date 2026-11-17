#define AppVersion "0.1.0"
[Setup]
AppId={{BF57976A-DAB3-4911-9910-4F85D0632B11}
AppName=无人机初步设计工具
AppVersion={#AppVersion}
DefaultDirName={localappdata}\Programs\UCAVDesigner
DefaultGroupName=无人机初步设计工具
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\release
OutputBaseFilename=UCAVDesigner-0.1.0-Windows-x64-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\UCAVDesigner.exe
DisableProgramGroupPage=yes
MinVersion=10.0

[Files]
Source: "..\release\package\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\无人机初步设计工具"; Filename: "{app}\UCAVDesigner.exe"
Name: "{autodesktop}\无人机初步设计工具"; Filename: "{app}\UCAVDesigner.exe"

[Run]
Filename: "{app}\UCAVDesigner.exe"; Description: "启动程序"; Flags: nowait postinstall skipifsilent
