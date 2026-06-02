; ColorDock RGB — Inno Setup 인스톨러 스크립트
; 빌드: iscc ColorDock_Setup.iss

#define AppName "ColorDock RGB"
#define AppVersion "1.0.0"
#define AppPublisher "ColorDock Team"
#define AppURL "https://github.com/impressionistfisherman/color-dock"
#define AppExeName "ColorDock.exe"
#define BuildDir "..\build"

[Setup]
AppId={{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
AppUpdatesURL={#AppURL}
DefaultDirName={autopf}\ColorDock RGB
DefaultGroupName={#AppName}
AllowNoIcons=yes
LicenseFile=
OutputDir=dist
OutputBaseFilename=ColorDock_Setup_{#AppVersion}
SetupIconFile=
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64os
PrivilegesRequired=admin
UninstallDisplayIcon={app}\{#AppExeName}
; 자동 재시작: 이전 버전 실행 중이면 종료
CloseApplications=yes
CloseApplicationsFilter=ColorDock.exe
RestartApplications=no

[Languages]
Name: "korean"; MessagesFile: "compiler:Languages\Korean.isl"

[Tasks]
Name: "desktopicon";     Description: "바탕화면 바로가기 생성";     GroupDescription: "추가 아이콘:"; Flags: unchecked
Name: "startupitem";     Description: "Windows 시작 시 자동 실행"; GroupDescription: "추가 설정:"; Flags: unchecked

[Files]
; 메인 실행 파일
Source: "{#BuildDir}\{#AppExeName}"; DestDir: "{app}"; Flags: ignoreversion

; Qt DLL
Source: "{#BuildDir}\Qt6Core.dll";    DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\Qt6Gui.dll";     DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\Qt6Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\Qt6Network.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\Qt6Svg.dll";     DestDir: "{app}"; Flags: ignoreversion

; MinGW 런타임
Source: "{#BuildDir}\libgcc_s_seh-1.dll";  DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\libstdc++-6.dll";     DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\libwinpthread-1.dll"; DestDir: "{app}"; Flags: ignoreversion

; OpenGL
Source: "{#BuildDir}\opengl32sw.dll";  DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\D3Dcompiler_47.dll"; DestDir: "{app}"; Flags: ignoreversion

; Qt 플러그인
Source: "{#BuildDir}\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs
Source: "{#BuildDir}\styles\*";    DestDir: "{app}\styles";    Flags: ignoreversion recursesubdirs
Source: "{#BuildDir}\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs
Source: "{#BuildDir}\tls\*";       DestDir: "{app}\tls";       Flags: ignoreversion recursesubdirs
Source: "{#BuildDir}\networkinformation\*"; DestDir: "{app}\networkinformation"; Flags: ignoreversion recursesubdirs

[Icons]
Name: "{group}\{#AppName}";                        Filename: "{app}\{#AppExeName}"
Name: "{group}\{cm:UninstallProgram,{#AppName}}";  Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}";                  Filename: "{app}\{#AppExeName}"; Tasks: desktopicon

[Registry]
; 시작프로그램 등록 (선택)
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "ColorDock"; ValueData: """{app}\{#AppExeName}"""; Flags: uninsdeletevalue; Tasks: startupitem

[Run]
Filename: "{app}\{#AppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(AppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}"

[Code]
// 이전 인스턴스 종료
function InitializeSetup(): Boolean;
var
  ResultCode: Integer;
begin
  Result := True;
end;
