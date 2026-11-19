; LensLinks.iss - Inno Setup script for the LensLinks Windows app.
; Build with:  powershell -ExecutionPolicy Bypass -File .\installer\build-installer.ps1
; Output:      installer\Output\LensLinks-Setup-1.0.0.exe

#define AppName      "LensLinks"
#define AppVersion   "1.0.0"
#define AppPublisher "KiaSteve"
#define AppURL       "https://github.com/KiaSteve/LensLinks"
#define AppExe       "lenslinks.exe"
#define BuildDir     "..\build\windows\x64\runner\Release"
#define CameraDll    "LensLinksNative.dll"

[Setup]
; Never change this GUID: it is how Windows recognises upgrades of the same app.
AppId={{6B2F3A52-9D41-4E0C-8A7B-1C5E4D2F9A10}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion} (Model One)
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}/issues
DefaultDirName={autopf}\LensLinks
DefaultGroupName=LensLinks
DisableProgramGroupPage=yes
OutputDir=Output
OutputBaseFilename=LensLinks-Setup-{#AppVersion}
Compression=lzma2
SolidCompression=yes
; The virtual camera is a COM DLL registered machine-wide, so admin rights are required.
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.22000
UninstallDisplayIcon={app}\{#AppExe}
WizardStyle=modern
CloseApplications=yes
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Shortcuts:"; Flags: unchecked

[Dirs]
Name: "{commonappdata}\LensLinks"

[Files]
; The virtual camera DLL is registered with Windows (regserver) after it is copied.
Source: "{#BuildDir}\{#CameraDll}"; DestDir: "{app}"; Flags: ignoreversion regserver restartreplace uninsrestartdelete
; Everything else from the Flutter release build.
Source: "{#BuildDir}\*"; DestDir: "{app}"; Excludes: "{#CameraDll}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\CHANGELOG.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\LensLinks"; Filename: "{app}\{#AppExe}"
Name: "{group}\Uninstall LensLinks"; Filename: "{uninstallexe}"
Name: "{autodesktop}\LensLinks"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Run]
; Lock down the data folder: SYSTEM and Administrators full, Users modify, LOCAL SERVICE read.
Filename: "{sys}\icacls.exe"; Parameters: """{commonappdata}\LensLinks"" /inheritance:r"; Flags: runhidden; StatusMsg: "Setting folder permissions..."
Filename: "{sys}\icacls.exe"; Parameters: """{commonappdata}\LensLinks"" /grant:r *S-1-5-18:(OI)(CI)F *S-1-5-32-544:(OI)(CI)F *S-1-5-32-545:(OI)(CI)M *S-1-5-19:(OI)(CI)RX"; Flags: runhidden
; Allow the app through Windows Firewall so the phone can be found and connected.
Filename: "{sys}\netsh.exe"; Parameters: "advfirewall firewall delete rule name=""LensLinks"" program=""{app}\{#AppExe}"""; Flags: runhidden
Filename: "{sys}\netsh.exe"; Parameters: "advfirewall firewall add rule name=""LensLinks"" dir=in action=allow program=""{app}\{#AppExe}"" enable=yes profile=any"; Flags: runhidden; StatusMsg: "Allowing LensLinks through the firewall..."
Filename: "{app}\{#AppExe}"; Description: "Start LensLinks"; Flags: nowait postinstall skipifsilent

[UninstallRun]
; Make sure Frame Server has released the camera DLL before it is unregistered and removed.
Filename: "{sys}\taskkill.exe"; Parameters: "/F /IM {#AppExe}"; Flags: runhidden; RunOnceId: "CloseApp"
Filename: "{sys}\net.exe"; Parameters: "stop FrameServer /y"; Flags: runhidden; RunOnceId: "StopFrameServer"
Filename: "{sys}\netsh.exe"; Parameters: "advfirewall firewall delete rule name=""LensLinks"" program=""{app}\{#AppExe}"""; Flags: runhidden; RunOnceId: "RemoveFirewallRule"

[UninstallDelete]
Type: filesandordirs; Name: "{commonappdata}\LensLinks"

[Code]
// Stop Windows Camera Frame Server before files are copied so the DLL is not locked.
procedure StopFrameServer;
var
  ResultCode: Integer;
begin
  Exec(ExpandConstant('{sys}\net.exe'), 'stop FrameServer /y', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
end;

// Close a running copy of the app so its files are not locked.
procedure CloseRunningApp;
var
  ResultCode: Integer;
begin
  Exec(ExpandConstant('{sys}\taskkill.exe'), '/F /IM {#AppExe}', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  CloseRunningApp;
  StopFrameServer;
  Result := '';
end;

// Frame Server must be running again before the app registers the camera.
procedure CurStepChanged(CurStep: TSetupStep);
var
  ResultCode: Integer;
begin
  if CurStep = ssPostInstall then
    Exec(ExpandConstant('{sys}\net.exe'), 'start FrameServer', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
end;

// The pairing key lives in the user's AppData; ask before removing it.
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  Dir: String;
begin
  if CurUninstallStep = usPostUninstall then
  begin
    Dir := ExpandConstant('{userappdata}\LensLinks');
    if DirExists(Dir) then
      if MsgBox('Also remove your saved pairing key and settings?', mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDYES then
        DelTree(Dir, True, True, True);
  end;
end;
