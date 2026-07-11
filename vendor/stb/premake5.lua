project "stb"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "On"

    files
    {
        "stb.cpp",
        "**.h"
    }

    includedirs
    {
        "."
    }