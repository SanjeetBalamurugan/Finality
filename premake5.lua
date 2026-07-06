workspace "Finality"  
    architecture "x64"
    startproject "Sandbox"
    staticruntime "off"

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
    
    -- Projects
       include "FinalityEngine"
       include "Sandbox"

    -- Dependencies
        include "Scripts/premake_glfw.lua" -- GLFW
    
    -- Scripts
        include "Scripts/premake_actions.lua"