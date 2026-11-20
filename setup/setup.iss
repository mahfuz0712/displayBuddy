; DisplayBuddy installer script for Inno Setup 6.3 or newer  (https://jrsoftware.org/isinfo.php)
;
; 1) In the MSYS2 UCRT64 terminal run:  ./deploy.sh --windows    (creates the "dist" folder)
; 2) Open this file in Inno Setup and press Ctrl+F9 (Build > Compile)
; 3) Your installer appears in:  setup\output\DisplayBuddy-Setup-<version>.exe

#define MyAppName      "DisplayBuddy"
#define MyAppVersion   "1.0.0"
#define MyAppPublisher "Mohammad Mahfuz Rahman"
#define MyAppExeName   "displaybuddy.exe"

[Setup]
; Fixed GUID so a newer setup.exe upgrades the old install. Do not change it.
AppId={{B7C2E4A1-5D3F-4B8E-9A61-2F7D0C1E8A93}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
VersionInfoVersion={#MyAppVersion}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
DisableDirPage=auto
; Machine-wide install for ALL users: needs admin, installs to Program Files,
; shortcuts go to the All Users Start Menu / Desktop, startup entry goes to HKLM.
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
LicenseFile=..\LICENSE
SetupIconFile=..\resources\icons\displaybuddy.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
UninstallDisplayName={#MyAppName}
OutputDir=..\setup\output
OutputBaseFilename=DisplayBuddy-Setup-{#MyAppVersion}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
; Ticked by default: start DisplayBuddy (in the system tray) for every user when they sign in to Windows.
Name: "startup";     Description: "Start {#MyAppName} when Windows starts, for all users (runs in the system tray)"; GroupDescription: "Startup:"
Name: "desktopicon"; Description: "Create a &desktop shortcut for all users"; GroupDescription: "Shortcuts:"; Flags: unchecked

[Files]
Source: "..\dist\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
; {autoprograms} / {autodesktop} resolve to the All Users folders in an admin install.
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}";  Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
; System-wide startup: HKLM Run runs for every user who signs in.
Root: HKLM; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "{#MyAppName}"; ValueData: """{app}\{#MyAppExeName}"" --minimized"; Flags: uninsdeletevalue; Tasks: startup

[Run]
; After installing, DisplayBuddy starts minimized: only its icon appears in the system tray.
Filename: "{app}\{#MyAppExeName}"; Parameters: "--minimized"; Description: "Start {#MyAppName} now (in the system tray)"; Flags: nowait postinstall

[UninstallRun]
Filename: "{sys}\taskkill.exe"; Parameters: "/F /IM {#MyAppExeName}"; Flags: runhidden; RunOnceId: "StopDisplayBuddy"

[Code]
// DisplayBuddy lives in the tray, so it may be running while you install an update. Stop it first.
procedure CurStepChanged(CurStep: TSetupStep);
var
  ResultCode: Integer;
begin
  if CurStep = ssInstall then
    Exec(ExpandConstant('{sys}\taskkill.exe'), '/F /IM {#MyAppExeName}', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
end;
