#define PluginVersion "1.9.2"
[Setup]
AppId={{DC2EC01D-3BF6-48BF-BB66-0B902934F8B4}
AppName=Zócalos para OBS
AppVersion={#PluginVersion}
AppVerName=Zócalos para OBS {#PluginVersion}
DefaultDirName={commonappdata}\obs-studio\plugins\jcp-sur-lower-thirds
DisableDirPage=yes
DisableProgramGroupPage=yes
#ifdef TestBuild
PrivilegesRequired=lowest
Uninstallable=no
#else
PrivilegesRequired=admin
#endif
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\..
#ifdef TestBuild
OutputBaseFilename=Zocalos-OBS-instalador-prueba
#else
OutputBaseFilename=Zocalos-OBS-v{#PluginVersion}-Instalador-Windows-x64
#endif
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
LicenseFile=..\LICENSE
CloseApplications=no
SetupLogging=yes
UninstallDisplayName=Zócalos para OBS
VersionInfoDescription=Instalador de Zócalos para OBS
VersionInfoProductName=Zócalos para OBS
VersionInfoVersion={#PluginVersion}

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Files]
Source: "..\release\jcp-sur-lower-thirds\bin\64bit\jcp-sur-lower-thirds.dll"; DestDir: "{app}\bin\64bit"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\release\jcp-sur-lower-thirds\data\*"; DestDir: "{app}\data"; Flags: ignoreversion recursesubdirs createallsubdirs

[Messages]
FinishedLabel=La instalación terminó. Abrí OBS y activá Paneles > Zócalos. La biblioteca existente se conserva.

[Code]
function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  Locator, Service, Processes: Variant;
begin
  Result := '';
  try
    Locator := CreateOleObject('WbemScripting.SWbemLocator');
    Service := Locator.ConnectServer('.', 'root\cimv2');
    Processes := Service.ExecQuery('SELECT ProcessId FROM Win32_Process WHERE Name = ''obs64.exe''');
    if Processes.Count > 0 then
      Result := 'Cerrá OBS Studio antes de instalar o actualizar el plugin. Después volvé a intentarlo.';
  except
    Result := 'No se pudo comprobar si OBS Studio está abierto. Cerrá OBS y volvé a ejecutar el instalador.';
  end;
end;
