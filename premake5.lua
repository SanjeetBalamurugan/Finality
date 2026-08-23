workspace "Finality"  
    architecture "x64"
    startproject "FinalityEditor"
    staticruntime "off"
    cppdialect "C++20"

    filter "action:vs*"
        buildoptions { "/utf-8" }

    configurations { "Debug", "Release" }

    outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

    targetdir ("bin/" .. outputdir)
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"
    
    -- Dependencies
    group "Dependencies"
        include "Scripts/premake_glfw.lua" -- GLFW
        include "Scripts/premake_vk.lua" -- VULKAN
        include "Scripts/premake_imgui.lua" -- IMGUI
        include "vendor/stb" -- STB
    group ""
            
    -- Projects
       include "FinalityEngine"
       include "Sandbox"
       include "FinalityEditor"
    
    -- Tools
    group "Tools"
        include "FIShaderCompiler"
    group ""

    -- Scripts
        include "Scripts/premake_actions.lua"