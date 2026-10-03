# ============================================================================
#  Setup.iss - Inno Setup installer for AudioFX (optional packaging step)
#  Build with Inno Setup 6+:  iscc Installer\Setup.iss
# ============================================================================
#define MyAppName "AudioFX"
#define MyAppVersion "1.0.0"
#define MyAppExeName "AudioFX.exe"

[Setup]
AppId={{8F2A6C4E-9B7D-4E5A-A1C3-D4F5E6A7B8C9}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher=AudioFX
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputBaseFilename=AudioFX-Setup-{#MyAppVersion}
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
WizardStyle=modern
SetupIconFile=..\resources\icon.ico
UninstallDisplayIcon={app}\{#MyAppExeName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"

[Files]
Source: "..\dist\AudioFX.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\Scripts\Install-VBCable.ps1"; DestDir: "{app}\Scripts"; Flags: ignoreversion
Source: "..\Scripts\Set-DefaultAudioDevice.ps1"; DestDir: "{app}\Scripts"; Flags: ignoreversion
Source: "..\Scripts\Register-AudioFX-Autostart.ps1"; DestDir: "{app}\Scripts"; Flags: ignoreversion
Source: "..\Scripts\WinAudio.cs"; DestDir: "{app}\Scripts"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Route audio through VB-Cable"; Filename: "powershell.exe"; \
    Parameters: "-ExecutionPolicy Bypass -File ""{app}\Scripts\Set-DefaultAudioDevice.ps1"""
Name: "{group}\Restore normal audio devices"; Filename: "powershell.exe"; \
    Parameters: "-ExecutionPolicy Bypass -File ""{app}\Scripts\Set-DefaultAudioDevice.ps1"" -Restore"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "powershell.exe"; \
    Parameters: "-ExecutionPolicy Bypass -File ""{app}\Scripts\Install-VBCable.ps1"""; \
    StatusMsg "Installing VB-Cable virtual audio driver..."; \
    Description: "Install VB-Cable driver (recommended, requires reboot)"; \
    Flags: shellexec runascurrentuser postinstall skipifsilent
Filename: "{app}\{#MyAppExeName}"; Description: "Launch AudioFX"; Flags: nowait postinstall skipifsilent
