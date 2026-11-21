project "FIScriptReflect"
    kind "ConsoleApp"
    language "C++"
    files { "src/**.cpp", "src/**.h" }
    local llvm = os.getenv("LLVM_ROOT") or path.join(os.getenv("ProgramFiles") or "/usr/local", "LLVM")
    includedirs { "../vendor/llvm-project/clang/include", "../vendor/llvm-project/llvm/include", path.join(llvm, "include") }
    local libraryDirectory = path.join(llvm, "lib")
    local runtimeDirectory = path.join(llvm, "bin")
    libdirs { libraryDirectory }
    links { "libclang" }
    postbuildcommands { '{COPYFILE} "%{prj.location}/licenses/LLVM-LICENSE.txt" "%{cfg.targetdir}/LLVM-LICENSE.txt"' }
    filter "system:windows"
    postbuildcommands { '{COPYFILE} "' .. path.join(runtimeDirectory, 'libclang.dll') .. '" "%{cfg.targetdir}/libclang.dll"' }
    filter "system:linux"
    postbuildcommands { '{COPYFILE} "' .. path.join(runtimeDirectory, 'libclang.so') .. '" "%{cfg.targetdir}/libclang.so"' }
    filter {}
    postbuildcommands { '{COPYDIR} "' .. path.join(llvm, 'lib/clang/22/include') .. '" "%{cfg.targetdir}/ClangResource/include"' }
