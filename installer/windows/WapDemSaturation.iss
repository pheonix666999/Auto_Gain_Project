#define SourceDir GetStringParam("SourceDir", "..\..\build\dist\WapDemSaturation-windows")
#define OutputDir GetStringParam("OutputDir", "..\..\build\dist")
#define OutputBaseFilename GetStringParam("OutputBaseFilename", "WapDemSaturation-windows-installer")

[Setup]
AppId={{B7E71911-AC4D-4B19-8C17-7FCE8B56D8A1}
AppName=WapDem Saturation
AppVersion=1.0.0
AppPublisher=Tractone Audio
AppPublisherURL=https://tractoneaudio.com
DefaultDirName={autopf}\Tractone Audio\WapDem Saturation
DefaultGroupName=Tractone Audio\WapDem Saturation
DisableProgramGroupPage=yes
OutputDir={#OutputDir}
OutputBaseFilename={#OutputBaseFilename}
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
UninstallDisplayName=WapDem Saturation

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Dirs]
Name: "{commoncf}\VST3"
Name: "{commoncf}\Avid\Audio\Plug-Ins"

[Files]
Source: "{#SourceDir}\Standalone\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "{#SourceDir}\VST3\*"; DestDir: "{commoncf}\VST3"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "{#SourceDir}\AAX\*"; DestDir: "{commoncf}\Avid\Audio\Plug-Ins"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "{#SourceDir}\README.md"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#SourceDir}\DELIVERY_NOTES.md"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#SourceDir}\checksums.txt"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist

[Icons]
Name: "{group}\WapDem Saturation"; Filename: "{app}\WapDem Saturation.exe"; Check: FileExists(ExpandConstant('{app}\WapDem Saturation.exe'))
Name: "{autodesktop}\WapDem Saturation"; Filename: "{app}\WapDem Saturation.exe"; Tasks: desktopicon; Check: FileExists(ExpandConstant('{app}\WapDem Saturation.exe'))

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Shortcuts:"; Flags: unchecked

[Run]
Filename: "{app}\WapDem Saturation.exe"; Description: "Launch WapDem Saturation"; Flags: nowait postinstall skipifsilent; Check: FileExists(ExpandConstant('{app}\WapDem Saturation.exe'))
