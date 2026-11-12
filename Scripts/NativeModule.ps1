param(
    [Parameter(Mandatory)][string]$ProjectRoot,
    [Parameter(Mandatory)][string]$EngineRoot,
    [Parameter(Mandatory)][string]$EngineBin,
    [string]$ScriptDirectory = 'Assets/Scripts',
    [switch]$GenerateOnly,
    [ValidateSet('Debug','Release')][string]$Configuration = 'Debug'
)

$ErrorActionPreference = 'Stop'
$resolvedProject = [System.IO.Path]::GetFullPath($ProjectRoot)
$sourceDirectory = [System.IO.Path]::GetFullPath((Join-Path $resolvedProject $ScriptDirectory))
$projectPrefix = $resolvedProject.TrimEnd('\','/') + '\'
if (-not $sourceDirectory.StartsWith($projectPrefix,[System.StringComparison]::OrdinalIgnoreCase)) { throw 'Script source directory must stay inside the project.' }
$locator = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installation = & $locator -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $installation) { throw 'Visual Studio C++ tools were not found.' }
$buildTool = Join-Path $installation 'MSBuild\Current\Bin\MSBuild.exe'
$compiler = Get-ChildItem -LiteralPath (Join-Path $installation 'VC\Tools\MSVC') -Directory | Sort-Object Name -Descending | Select-Object -First 1
$env:Path = (Join-Path $compiler.FullName 'bin\Hostx64\x64') + ';' + $env:Path
$python = $env:FINALITY_PYTHON
$pythonConfiguration = Join-Path $EngineRoot '.tools/reflection-python.txt'
if (-not $python -and (Test-Path -LiteralPath $pythonConfiguration)) { $python = [System.IO.File]::ReadAllText($pythonConfiguration).Trim() }
if (-not $python) { throw 'Configure Python for script reflection with Scripts/SetupScriptReflection.ps1 -PythonExecutable <python.exe>.' }
$sdk = Get-ChildItem -LiteralPath (Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\Include') -Directory | Sort-Object Name -Descending | Select-Object -First 1
$reflected = Join-Path $resolvedProject ".Finality\NativeScripts\Reflected\$Configuration"
& $python (Join-Path $EngineRoot 'Scripts/ReflectScripts.py') '--source' $sourceDirectory '--output' $reflected '--msvc' $compiler.FullName '--sdk' $sdk.FullName '--configuration' $Configuration
if ($LASTEXITCODE -ne 0) { throw 'Native script reflection failed; the previous module remains available.' }
$generator = Join-Path $EngineRoot 'vendor\premake\premake5.exe'
$template = Join-Path $EngineRoot 'Scripts\Templates\ProjectScripts.lua'
& $generator "--file=$template" vs2026 "--engine-root=$EngineRoot" "--engine-bin=$EngineBin" "--project-root=$resolvedProject" "--script-directory=$sourceDirectory" "--reflected-directory=$reflected"
if ($LASTEXITCODE -ne 0) { throw 'Native script project generation failed.' }
if ($GenerateOnly) { return }
& $buildTool (Join-Path $resolvedProject '.Finality\NativeScripts\GameScripts.vcxproj') '/m:1' '/nr:false' "/p:Configuration=$Configuration" '/p:Platform=x64' '/p:PreferredToolArchitecture=x64' '/v:minimal' '/nologo'
if ($LASTEXITCODE -ne 0) { throw 'Native script compilation failed.' }
