project "FIBaker"
    kind "ConsoleApp"
    language "C++"
    files { "src/**.cpp" }
    includedirs { "../FinalityEngine/src", "../vendor/yaml-cpp/include" }
    links { "FinalityEngine", "yaml-cpp" }
