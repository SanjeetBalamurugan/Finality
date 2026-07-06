project "FinalityEngine"
    kind "SharedLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "Off"

    files {
        "src/**.h",
        "src/**.cpp",
        "src/**.c"
    }

    includedirs {
        "src",
        "../vendor/glfw/include/"
    }

    defines {
        "FINALITY_BUILD_DLL"
    }

    links {
        "GLFW"
    }

    filter "system:windows"
        systemversion "latest"

        defines {
            "_WINDLL"
        }
        defines { "GLFW_INCLUDE_NONE" }