project "FinalityEditor"
    kind "ConsoleApp"
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