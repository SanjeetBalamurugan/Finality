project "FIBuild"
    kind "ConsoleApp"
    language "C++"
    files { "src/**.cpp", "src/**.h" }
    includedirs { "../vendor/yaml-cpp/include" }
    defines { "YAML_CPP_STATIC_DEFINE" }
    links { "yaml-cpp" }
