newoption { trigger = "engine-root", value = "PATH", description = "Finality engine checkout" }
newoption { trigger = "engine-bin", value = "PATH", description = "Engine binaries matching the editor configuration" }
newoption { trigger = "project-root", value = "PATH", description = "Directory containing the Finality project" }
newoption { trigger = "script-directory", value = "PATH", description = "Directory containing native script sources" }
newoption { trigger = "reflected-directory", value = "PATH", description = "Generated script translation units" }
local engine = assert(_OPTIONS["engine-root"])
local root = assert(_OPTIONS["project-root"])
workspace "FinalityProjectScripts"
    architecture "x64"
    configurations { "Debug", "Release" }
    location (root .. "/.Finality/NativeScripts")
project "GameScripts"
    kind "SharedLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "Off"
    files { _OPTIONS["reflected-directory"] .. "/**.cpp", _OPTIONS["script-directory"] .. "/**.h", _OPTIONS["script-directory"] .. "/**.hpp" }
    targetdir (root .. "/.Finality/NativeScripts/bin/%{cfg.buildcfg}")
    objdir (root .. "/.Finality/NativeScripts/obj/%{cfg.buildcfg}")
    includedirs { engine .. "/FinalityEngine/src", engine .. "/vendor/glm", engine .. "/vendor/entt/include", engine .. "/vendor/spdlog/include", engine .. "/vendor/yaml-cpp/include", engine .. "/vendor/imgui" }
    libdirs { _OPTIONS["engine-bin"] }
    links { "FinalityEngine" }
    defines { "GLM_FORCE_PURE", "GLM_FORCE_UNRESTRICTED_FLOAT", "IMGUI_API=__declspec(dllimport)" }
    buildoptions { "/utf-8", "/MP4", "/FS", "/Zc:__cplusplus" }
    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
        defines { "DEBUG" }
    filter "configurations:Release"
        runtime "Release"
        optimize "On"
        defines { "NDEBUG" }
    filter {}
project "Project Sources"
    kind "Utility"
    files { _OPTIONS["script-directory"] .. "/**.cpp", _OPTIONS["script-directory"] .. "/**.h", _OPTIONS["script-directory"] .. "/**.hpp" }
    includedirs { engine .. "/FinalityEngine/src", engine .. "/vendor/glm", engine .. "/vendor/entt/include", engine .. "/vendor/spdlog/include", engine .. "/vendor/imgui" }
project "Finality Engine Sources"
    kind "Utility"
    files { engine .. "/FinalityEngine/src/**.h", engine .. "/FinalityEngine/src/**.cpp" }
