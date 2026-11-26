# Native build tooling

FIBuild handles native project scripts, engine build metadata, IDE file generation
and Assimp dependency builds. It does not depend on the engine DLL, so the engine
can build it first and use it to record its own build information.

From a Visual Studio developer command prompt at the repository root:

```text
git submodule update --init --recursive
vendor\premake\premake5.exe vs2026
MSBuild Finality.slnx /p:Configuration=Release /p:Platform=x64
```

Use `vs2022` instead for Visual Studio 2022. Open the generated Finality solution
in Visual Studio to build there, or run MSBuild as above. The normal solution
build constructs FIBuild before the engine and automatically configures/builds
Assimp, including after deleting bin and bin-int. Install CMake or Visual Studio's
C++ CMake tools. Vulkan SDK and matching Visual Studio
C++ tools/Windows SDK are still build-machine dependencies. LLVM source is pinned
to llvmorg-22.1.8 in the vendor/llvm-project Git submodule. Initialize dependencies
with `git submodule update --init --recursive`. Install a compatible LLVM 22
development package for its import library, runtime and builtin headers. The
standard Windows Program Files/LLVM location is detected automatically; set
LLVM_ROOT before running Premake for another location. Reflection uses the pinned
submodule's C API headers. No LLVM binaries are tracked
in this repository. A working Linux editor/player build remains future work.

The editor output includes glslc.exe, shaderc_shared.dll, and its assets. Running
the built editor does not require a Vulkan SDK installation for shader compilation;
a Vulkan-capable graphics driver is still required. Building the engine from source
does require the SDK and a valid VULKAN_SDK environment variable. Restart Visual
Studio after installing it, then regenerate the solution. A binaries-only release
does not include the engine headers and templates needed for native script builds;
use the matching source checkout or the packaged Dist development SDK for that workflow. Exported players use compiled
shaders and do not need glslc.

Select the Dist configuration and build the Distribution target (or the whole
solution) to produce bin/Dist-windows-x86_64. Dist enables full optimization,
intrinsics, link-time optimization and unused-code folding without unsafe floating
point changes. Its SDK folder contains matching headers, dependency includes,
import libraries and build templates; no engine implementation sources, Git history
or LLVM source checkout are needed on the user's machine. Keep SDK next to the
editor and install the MSVC and Windows SDK versions recorded in FinalitySDK.json.
Generated game solutions link this SDK and use the project's actual Assets folders;
the script vcxproj is at the project root, while generated compilation units remain
in .Finality and do not appear as authored script files.

Game exports embed minimal scene/import/window data inside Game.exe; editor build
settings stay in Settings/Build.yaml. Company, product and version are written into
Windows executable metadata. Player Settings provides a default icon and 16, 24,
32, 48, 64, 128 and 256 pixel overrides; overrides must match their slot dimensions.
The baker generates .Finality/Build/Game.ico and embeds its frames into Game.exe.
Empty slots use the default image or the Finality icon. Game exports include neither
Player.yaml nor shader sources nor the development SDK.

Doxygen is an external developer tool. Install it on Windows with:

```text
winget install --id DimitriVanHeesch.Doxygen --exact --source winget
```

The Documentation target uses Doxygen from PATH or the standard Windows install
location. DOXYGEN_EXECUTABLE can select another installation. Doxygen is not
bundled with the repository, editor or player.

The editor checks installation files before graphics initialization. Missing assets
or a shader compiler block startup with repair guidance. Missing native development
tools or game export tools produce a warning that permits basic editing. The dialog
can retry checks after repairs. `FinalityEditor.exe --check-dependencies` prints the
same report without opening a window; a nonzero exit code means startup is blocked.
`FIBuild.exe check --engine "engine directory" --bin "matching engine binaries"
--configuration Release` validates native prerequisites without building a project.

Project script compilation:

```text
FIBuild.exe native --project "project directory" --name "Project Name" --engine "engine directory" --bin "matching engine binaries" --configuration Release
```

Add `--generate-only` to refresh project membership without parsing unfinished
source or changing the compiled module. `--source` defaults to Assets/Scripts;
an Assets-based source directory enables discovery throughout Assets.
The generated VS Code task invokes FIBuild directly.

The engine's post-build step uses `FIBuild info` to record the DLL/header SHA-256
fingerprints and toolchain versions. Native builds reject mismatched binaries,
headers or configurations and retain the previous DLL on compile failure.

Cleanup:

```text
vendor\premake\premake5.exe clean --clean-dry-run
vendor\premake\premake5.exe clean
```

Clean removes recognized generated engine outputs and project files. It preserves
embedded projects, authored Assets/Settings, exported players, vendored packages,
IDE state and the Assimp dependency cache. It validates resolved paths and skips
links. It does not recursively delete the entire bin directory.
The LLVM source-build cache is also preserved.
In source asset directories it removes compiled shader caches only when a paired
source file exists; standalone compiled shader assets are preserved.
