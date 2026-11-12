param([Parameter(Mandatory)][string]$PythonExecutable)

$ErrorActionPreference = 'Stop'
$engine = Split-Path -Parent $PSScriptRoot
$python = (Get-Item -LiteralPath $PythonExecutable).FullName
$destination = Join-Path $engine '.tools/reflection'
& $python -m pip install --upgrade --target $destination 'libclang-ng==22.1.4.2'
if ($LASTEXITCODE -ne 0) { throw 'Could not install the native script reflection parser.' }
[System.IO.Directory]::CreateDirectory((Join-Path $engine '.tools')) | Out-Null
[System.IO.File]::WriteAllText((Join-Path $engine '.tools/reflection-python.txt'), $python)
