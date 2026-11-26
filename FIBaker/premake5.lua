project "FIBaker"
    kind "ConsoleApp"
    language "C++"
    defines { "YAML_CPP_STATIC_DEFINE" }
    files { "src/**.cpp" }
    includedirs { "../FinalityEngine/src", "../vendor/yaml-cpp/include" }
    links { "FinalityEngine", "yaml-cpp" }
    filter "system:windows"
        links { "Ole32", "Windowscodecs" }
    filter {}
