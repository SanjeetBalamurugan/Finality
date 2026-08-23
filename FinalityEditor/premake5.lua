project "FinalityEditor"
    kind "WindowedApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "Off"

    files {
        "src/**.h",
        "src/**.cpp"
    }

    includedirs {
        "src",
        "../FinalityEngine/src",
        "../vendor/glm",
        "../vendor/entt/include",
        "../vendor/spdlog/include/"
    }

    links {
        "FinalityEngine"
    }

    filter "system:windows"
        systemversion "latest"
        entrypoint "mainCRTStartup" 
        files { 
            "assets/windows/Resource.rc", 
            "assets/windows/favicon.ico" 
        }
    filter {} 