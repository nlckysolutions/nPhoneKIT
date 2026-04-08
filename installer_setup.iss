; Inno Setup Script for nPhoneKIT
; This script generates the professional installer.

[Setup]
AppId={{D3A6B481-B1A4-4E8E-8A5F-4A4D2E1FD71A}}
AppName=nPhoneKIT
AppVersion=1.6.2
AppPublisher=NlckySolutions
AppPublisherURL=https://github.com/nlckysolutions/nPhoneKIT
AppSupportURL=https://github.com/nlckysolutions/nPhoneKIT
AppUpdatesURL=https://github.com/nlckysolutions/nPhoneKIT
DefaultDirName={autopf}\nPhoneKIT
DefaultGroupName=nPhoneKIT
AllowNoIcons=yes
LicenseFile=LICENSE
; Uncomment the following line to run in non administrative install mode (install for current user only.)
;PrivilegesRequired=lowest
PrivilegesRequired=admin
OutputDir=dist_installer
OutputBaseFilename=nPhoneKIT_Setup
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "dist\nPhoneKIT\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\nPhoneKIT"; Filename: "{app}\nPhoneKIT.exe"
Name: "{group}\{cm:UninstallProgram,nPhoneKIT}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\nPhoneKIT"; Filename: "{app}\nPhoneKIT.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\nPhoneKIT.exe"; Description: "{cm:LaunchProgram,nPhoneKIT}"; Flags: nowait postinstall skipifsilent
