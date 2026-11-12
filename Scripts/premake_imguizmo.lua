
project "ImGuizmo"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"
    defines { "IMGUI_DEFINE_MATH_OPERATORS" }

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "../vendor/ImGuizmo/src/ImGuizmo.cpp",
        "../vendor/ImGuizmo/src/ImGuizmo.h"
    }

    includedirs
    {
        "../vendor/ImGuizmo/src",
        "../FinalityEngine/src",
        "../vendor/imgui"
    }
    forceincludes { "ImGUI/ImGuizmoApi.h" }

    links {
        "ImGui"
    }

    filter "system:windows"
        defines { "IMGUI_API=__declspec(dllimport)" }
        systemversion "latest"

    filter "system:linux"
        pic "On"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"

    filter {}
