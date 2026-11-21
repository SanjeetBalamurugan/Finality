# Native build tooling

FIBuild handles native project scripts, engine build metadata, IDE file generation
and Assimp dependency builds. It does not depend on the engine DLL, so the engine
can build it first and use it to record its own build information.

From a Visual Studio developer command prompt at the repository root:

```text
vendor\premake\premake5.exe vs2026 --build-suffix=milestone
MSBuild FIBuild\FIBuild.vcxproj /p:Configuration=Release /p:Platform=x64
bin\Release-windows-x86_64-milestone\FIBuild.exe dependencies --engine . --configuration Release
MSBuild Finality.slnx /p:Configuration=Release /p:Platform=x64
```

The dependency command is needed for a fresh Assimp cache. The normal solution
build constructs FIBuild before the engine. Vulkan SDK and matching Visual Studio
C++ tools/Windows SDK are still build-machine dependencies. LLVM source is pinned
to llvmorg-22.1.8 in the vendor/llvm-project Git submodule. Initialize dependencies
with `git submodule update --init --recursive`. Install a compatible LLVM 22
development package for its import library, runtime and builtin headers. The
standard Windows Program Files/LLVM location is detected automatically; set
LLVM_ROOT before running Premake for another location. Reflection uses the pinned
submodule's C API headers. No LLVM binaries are tracked
in this repository. A working Linux editor/player build remains future work.

Doxygen is an external developer tool. Install it on Windows with:

```text
winget install --id DimitriVanHeesch.Doxygen --exact --source winget
```

The Documentation target uses Doxygen from PATH or the standard Windows install
location. DOXYGEN_EXECUTABLE can select another installation. Doxygen is not
bundled with the repository, editor or player.

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
