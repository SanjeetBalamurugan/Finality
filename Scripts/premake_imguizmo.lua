
project "ImGuizmo"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "../vendor/ImGuizmo/src/GraphEditor.cpp",
        "../vendor/ImGuizmo/src/GraphEditor.h",
        "../vendor/ImGuizmo/src/ImCurveEdit.cpp",
        "../vendor/ImGuizmo/src/ImCurveEdit.h",
        "../vendor/ImGuizmo/src/ImGradient.cpp",
        "../vendor/ImGuizmo/src/ImGradient.h",
        "../vendor/ImGuizmo/src/ImGuizmo.cpp",
        "../vendor/ImGuizmo/src/ImGuizmo.h",
        "../vendor/ImGuizmo/src/ImLightRig.h",
        "../vendor/ImGuizmo/src/ImSequencer.cpp",
        "../vendor/ImGuizmo/src/ImSequencer.h",
        "../vendor/ImGuizmo/src/ImVectorEditor.cpp",
        "../vendor/ImGuizmo/src/ImVectorEditor.h",
        "../vendor/ImGuizmo/src/ImZoomSlider.h"
    }

    includedirs
    {
        "../vendor/ImGuizmo",
        "../vendor/imgui"
    }

    links {
        "ImGui"
    }

    filter "system:windows"
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
