; ======================================================================
; TianBoard.iss — 安装包脚本 (Inno Setup 6)
;
; 打包对象: 单个可执行 build\bin\TianBoard.exe + 它与 Qt 运行库 + 与 exe 同级的
; 四个运行期目录 etc/ library/ customizations/ Language/。没有插件目录：
; TianBoard 是单进程单可执行，模块在链接期就并进 exe 了。
;
; stage 目录由 packaging\make-installer.ps1 组装（拷 exe + 跑 windeployqt），
; 版本与目录由它用 /D 传进来：
;   powershell -ExecutionPolicy Bypass -F packaging\make-installer.ps1 -Build
; ======================================================================
;{AppVersion}
;#define AppVersion "0.0.0"
;{StageDir}
;#define StageDir "..\build\dist\stage"
;{OutDir}
;#define OutDir "..\build\dist"
;#ifndef AppVersion
;  #define AppVersion "0.0.0"
;#endif
;#ifndef StageDir
;  #define StageDir "..\build\dist\stage"
;#endif
;#ifndef OutDir
;  #define OutDir "..\build\dist"
;#endif

#define AppName "TianBoard"
#define AppDisplayName "TianBoard 交互式白板"
#define Publisher "TianBoard"

[Setup]
AppId={{7C4B1A9E-2F63-4C58-9B0D-5A1E8D3F6B27}
AppName={#AppDisplayName}
AppVersion={#AppVersion}
AppVerName={#AppDisplayName} {#AppVersion}
AppPublisher={#Publisher}
VersionInfoVersion={#AppVersion}
UninstallDisplayName={#AppDisplayName} {#AppVersion}
UninstallDisplayIcon={app}\{#AppName}.exe

DefaultDirName={sd}\{#AppName}
DefaultGroupName={#AppDisplayName}
DisableProgramGroupPage=yes
DisableWelcomePage=no
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0

OutputDir={#OutDir}
OutputBaseFilename={#AppName}-{#AppVersion}-x64-setup
WizardStyle=modern
Compression=lzma2/max
SolidCompression=yes
CloseApplications=yes
RestartApplications=no

[Languages]
Name: "zh"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"

[Tasks]
Name: "desktopicon"; Description: "在桌面创建快捷方式"; GroupDescription: "附加任务:"

[Dirs]
; 没有需要放开写权限的目录：应用把配置、日志、文档库写在 %APPDATA%\TianBoard，
; 安装目录里的 etc/ library/ customizations/ Language/ 只是出厂默认值，只读即可。
; 因此本节故意留空 —— 有 users-modify 的那类写法在这里是多余的。
Name: "{app}"

[Files]
; 整个 stage 目录: exe + Qt 运行库(含 WebEngine 的 Qt6WebEngineCore 与子进程)
; + platforms/imageformats/styles 等 Qt 插件目录 + etc/ library/ customizations/ Language/
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#AppDisplayName}"; Filename: "{app}\{#AppName}.exe"
Name: "{group}\卸载 {#AppDisplayName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppDisplayName}"; Filename: "{app}\{#AppName}.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppName}.exe"; Description: "立即启动 {#AppDisplayName}"; Flags: nowait postinstall skipifsilent

[Code]
function NextButtonClick(CurPageID: Integer): Boolean;
var
  Target: String;
begin
  Result := True;
  if CurPageID <> wpSelectDir then
    Exit;

  Target := Lowercase(WizardDirValue);
  // 默认目录是 C:\TianBoard。装进 Program Files 也能跑（应用不往安装目录写），
  // 但素材库 library/ 体量大、卸载时容易与安全软件的目录保护纠缠，所以建议独立目录。
  if (Pos('\program files', Target) > 0) then
  begin
    Result := MsgBox('建议装到 C:\{#AppName} 这类独立目录：素材库 library/ 体积大，' +
                     '卸载时容易与安全软件的目录保护相互纠缠。' + #13#10#13#10 +
                     '仍然要装到这里吗?',
                     mbConfirmation, MB_YESNO) = IDYES;
  end;
end;
