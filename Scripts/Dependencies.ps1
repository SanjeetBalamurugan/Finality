param([ValidateSet('Debug','Release')][string]$Configuration = 'Debug')

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$source = Join-Path $projectRoot 'vendor\assimp'
$output = Join-Path $projectRoot 'bin-int\Assimp'
if (-not (Test-Path -LiteralPath (Join-Path $source 'CMakeLists.txt'))) { throw 'Initialize the Assimp submodule before building.' }
& cmake -S $source -B $output -G 'Visual Studio 18 2026' -A x64 -DBUILD_SHARED_LIBS=OFF -DASSIMP_BUILD_TESTS=OFF -DASSIMP_BUILD_ASSIMP_TOOLS=OFF -DASSIMP_BUILD_SAMPLES=OFF -DASSIMP_INSTALL=OFF -DASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT=OFF -DASSIMP_BUILD_OBJ_IMPORTER=ON -DASSIMP_BUILD_GLTF_IMPORTER=ON -DASSIMP_BUILD_FBX_IMPORTER=ON -DASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT=OFF -DASSIMP_BUILD_ZLIB=ON -DASSIMP_BUILD_DRACO=OFF -DASSIMP_INJECT_DEBUG_POSTFIX=OFF '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>DLL'
if ($LASTEXITCODE -ne 0) { throw 'Assimp configuration failed.' }
& cmake --build $output --config $Configuration --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Assimp build failed.' }
