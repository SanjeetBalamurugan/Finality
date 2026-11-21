
project "FinalityEditor"
    kind "WindowedApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "Off"
    dependson { "FIScriptReflect", "FIBuild", "FIBaker", "FinalityPlayer" }

    files {
        "src/**.h",
        "src/**.cpp"
    }

    includedirs {
        "src",
        "../FinalityEngine/src",
        "../vendor/glm",
        "../vendor/entt/include",
        "../vendor/spdlog/include/",
        "../vendor/yaml-cpp/include",
        "../vendor/imgui"
    }

    links {
        "FinalityEngine",
        "ImGui",
        "yaml-cpp", "Shell32", "Ole32", "OleAut32"
    }

    filter "system:windows"
        defines { "IMGUI_API=__declspec(dllimport)" }
        systemversion "latest"
        entrypoint "mainCRTStartup"
        files {
            "assets/windows/Resource.rc",
            "assets/windows/favicon.ico"
        }
    filter {}
