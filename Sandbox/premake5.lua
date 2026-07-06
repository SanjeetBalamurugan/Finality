project "Sandbox"
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
        "../FinalityEngine/src"
    }

    links {
        "FinalityEngine"
    }

    filter "system:windows"
        systemversion "latest"