newoption
{
    trigger = "build-suffix",
    value = "NAME",
    description = "Use an isolated output directory for verification builds"
}
include "Scripts/NoVisualStudioFilters.lua"

if _ACTION and _ACTION:match("^vs") then
    local requiredDependencies =
    {
        "vendor/glfw/include/GLFW/glfw3.h",
        "vendor/imgui/imgui.h",
        "vendor/assimp/CMakeLists.txt",
        "vendor/llvm-project/clang/include/clang-c/Index.h"
    }

    for _, required in ipairs(requiredDependencies) do
        if not os.isfile(required) then
            error("Missing dependency: " .. required .. ". Run git submodule update --init --recursive, then regenerate the solution.")
        end
    end
end

workspace "Finality"
    architecture "x64"
    startproject "FinalityEditor"
    staticruntime "off"
    cppdialect "C++20"

    filter "action:vs*"
        buildoptions { "/utf-8", "/MP4" }

    configurations
    {
        "Debug",
        "Release",
        "Dist"
    }

    outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
    if _OPTIONS["build-suffix"] then
        outputdir = outputdir .. "-" .. _OPTIONS["build-suffix"]
    end

    targetdir ("bin/" .. outputdir)
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"
    filter "configurations:Dist"
        defines { "NDEBUG", "FINALITY_DIST" }
        runtime "Release"
        optimize "Full"
        symbols "Off"
        intrinsics "On"
        linktimeoptimization "On"
    filter { "configurations:Dist", "action:vs*" }
        functionlevellinking "On"
        linkoptions { "/OPT:REF", "/OPT:ICF" }
    filter {}

    -- Dependencies
    group "Dependencies"
        include "Scripts/premake_glfw.lua" -- GLFW
        include "Scripts/premake_vk.lua" -- VULKAN
        include "Scripts/premake_imgui.lua" -- IMGUI
        include "Scripts/premake_yaml.lua" -- YAML-CPP
        include "Scripts/premake_imguizmo.lua" -- IMGUIZMO
        include "vendor/stb" -- STB
    group ""

    -- Projects
       include "FinalityEngine"
       include "Sandbox"
       include "FinalityEditor"
       include "FinalityPlayer"

    -- Tools
    group "Tools"
        include "FIShaderCompiler"
        include "FIScriptReflect"
        include "FIBaker"
        include "FIBuild"
        include "Documentation"
        include "Scripts/Distribution.lua"

    group ""

    -- Scripts
        include "Scripts/premake_actions.lua"
