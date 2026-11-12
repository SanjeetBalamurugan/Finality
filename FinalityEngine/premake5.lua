
project "FinalityEngine"
    kind "SharedLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "Off"

    files {
        "src/**.h",
        "src/**.cpp",
        "src/**.c",
        "src/**.inl",
        "../vendor/imgui/backends/imgui_impl_glfw.cpp",
        "../vendor/imgui/backends/imgui_impl_vulkan.cpp"
    }

    includedirs {
        "src",
        "../vendor/glfw/include/",
        "../vendor/imgui",
        "../vendor/ImGuizmo/src",
        "../vendor/spdlog/include/",
        "../vendor/glm",
        "../vendor/entt/include",
        "../vendor/stb/include",
        "../vendor/gli",
        "../vendor/yaml-cpp/include",
        "../vendor/assimp/include",
        "../bin-int/Assimp/include",
        "../vendor/assimp/contrib/rapidjson/include",
        "../vendor/tomlplusplus"
    }

    links {
        "GLFW",
        "stb",
        "ImGuizmo",
        "ImGui",
        "yaml-cpp"
    }
    libdirs { "../bin-int/Assimp/lib/%{cfg.buildcfg}" }
    links { "assimp-vc145-mt" }
    filter "configurations:Debug"
        libdirs { "../bin-int/Assimp/contrib/zlib/Debug" }
        links { "zlibstaticd" }
        links { "spirv-cross-cored" }
    filter "configurations:Release"
        libdirs { "../bin-int/Assimp/contrib/zlib/Release" }
        links { "zlibstatic" }
        links { "spirv-cross-core" }
    filter {}

    defines {
        "FINALITY_BUILD_DLL",
        "GLM_FORCE_PURE",
        "GLM_FORCE_UNRESTRICTED_FLOAT"
    }

    dependson
    {
        "FIShaderCompiler"
    } -- Move this to editor project if i actually make it

    -- Importing Vulkan
    local vulkan = GetVulkanSDK()
    if vulkan then
        print("Vulkan SDK detected at: " .. vulkan.root)

        includedirs { vulkan.include }
        libdirs     { vulkan.lib }
        defines     { "FINALITY_INCLUDE_VULKAN" }

        filter "system:windows"
            links { "vulkan-1" }
        filter "system:linux or macosx"
            links { "vulkan" }
        filter {}
    else
        print("Vulkan SDK not found.")
    end

    filter "system:windows"
        defines { "IMGUI_API=__declspec(dllimport)", "IMGUI_IMPL_API=" }
        systemversion "latest"

        defines {
            "_WINDLL"
        }
        defines { "GLFW_INCLUDE_NONE" }

        buildoptions { "/Zc:__cplusplus" }
