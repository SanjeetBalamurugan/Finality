project "FinalityPlayer"
    kind "ConsoleApp"
    language "C++"
    files { "src/**.cpp" }
    includedirs { "../FinalityEngine/src", "../vendor/glm", "../vendor/entt/include", "../vendor/spdlog/include", "../vendor/yaml-cpp/include", "../vendor/imgui" }
    links { "FinalityEngine", "ImGui", "yaml-cpp", "Shell32" }
    filter "system:windows"
        defines { "IMGUI_API=__declspec(dllimport)" }
    filter {}
